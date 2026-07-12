#ifndef BULK_HPP
#define BULK_HPP

/**
 * @file bulk.hpp
 * @brief Разбор входных строк и накопление блоков команд.
 */

#include <chrono>
#include <cstddef>
#include <filesystem>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace bulk {
    /** @brief Момент времени, используемый для имени файла пачки команд. */
    using TimePoint = std::chrono::system_clock::time_point;

    /** @brief Функция, возвращающая время получения очередной строки. */
    using Clock = std::function<TimePoint()>;

    /** @brief Обычная строковая команда. Пустой текст также является командой. */
    struct Command final {
        /** @brief Текст команды без символа перевода строки. */
        std::string text;
    };

    /** @brief Событие начала динамического блока (`{`). */
    struct OpenDynamicBlock final {
    };

    /** @brief Событие завершения динамического блока (`}`). */
    struct CloseDynamicBlock final {
    };

    /** @brief Результат разбора одной входной строки. */
    using ParsedLine = std::variant<Command, OpenDynamicBlock, CloseDynamicBlock>;

    /**
     * @brief Разбирает входные строки в команды и управляющие события.
     *
     * Класс не хранит состояние. Управляющими считаются только строки, которые
     * в точности равны `{` или `}`.
     */
    class LineParser final {
    public:
        /**
         * @brief Разбирает одну входную строку.
         * @param line Строка без `\n`; завершающий `\r` от CRLF удаляется.
         * @return Команда либо событие открытия/закрытия динамического блока.
         */
        static ParsedLine parse(std::string line);
    };

    /** @brief Готовая к выводу пачка команд. */
    struct CommandBulk final {
        /** @brief Команды в порядке получения. */
        std::vector<std::string> commands;

        /** @brief Время получения первой команды пачки. */
        TimePoint started_at;
    };

    /**
     * @brief Накапливает команды и реализует правила статических и динамических пачек.
     *
     * Класс ничего не выводит: при завершении пачки он передаёт владение её
     * командами вызывающему коду через `std::optional<CommandBulk>`.
     */
    class BulkAccumulator final {
    public:
        /**
         * @brief Создаёт накопитель статических пачек заданного размера.
         * @param static_batch_size Число команд в статической пачке.
         * @throws std::invalid_argument Если размер равен нулю.
         */
        explicit BulkAccumulator(std::size_t static_batch_size);

        /**
         * @brief Принимает очередное разобранное событие.
         *
         * Открытие внешнего динамического блока завершает накопленную статическую
         * пачку. Вложенные скобки меняют только глубину блока. Закрытие внешнего
         * блока завершает динамическую пачку, а лишняя `}` считается командой.
         *
         * @param line Команда или управляющее событие; значение поглощается методом.
         * @param received_at Время получения этой входной строки.
         * @return Готовая пачка, если событие её завершило, иначе `std::nullopt`.
         */
        [[nodiscard]] std::optional<CommandBulk> accept(ParsedLine line,
                                                        TimePoint received_at);

        /**
         * @brief Завершает обработку входа.
         * @return Неполная статическая пачка или `std::nullopt`, если команд нет.
         * @note Незавершённая динамическая пачка отбрасывается целиком.
         */
        [[nodiscard]] std::optional<CommandBulk> finish();

    private:
        /** @brief Извлекает накопленные команды как готовую пачку. */
        [[nodiscard]] std::optional<CommandBulk> release();

        /** @brief Удаляет накопленные команды без формирования пачки. */
        void discard() noexcept;
    };

    /**
     * @brief Интерфейс получателя готовых пачек.
     *
     * Накопление команд не зависит от конкретного способа вывода: консоль,
     * файл и их композиция реализуют один и тот же интерфейс.
     */
    class IBulkSink {
    public:
        /** @brief Обеспечивает корректное полиморфное уничтожение получателя. */
        virtual ~IBulkSink() = default;

        /**
         * @brief Записывает одну готовую пачку.
         * @param command_bulk Пачка, доступная только на время вызова.
         * @throws std::exception Реализация может сообщить об ошибке вывода.
         */
        virtual void write(const CommandBulk &command_bulk) = 0;
    };

    /** @brief Выводит каждую пачку команд в заданный поток и немедленно сбрасывает его буфер. */
    class ConsoleBulkSink final : public IBulkSink {
    public:
        /**
         * @brief Создаёт консольный получатель.
         * @param output Поток вывода; должен существовать дольше объекта. По умолчанию std::cout
         * @note Класс хранит ссылку и не владеет потоком.
         */
        explicit ConsoleBulkSink(std::ostream &output = std::cout) noexcept;

        /**
         * @brief Записывает форматированную пачку и перевод строки.
         * @param command_bulk Пачка для вывода.
         * @throws std::runtime_error Если запись в поток завершилась ошибкой.
         */
        void write(const CommandBulk &command_bulk) override;
    };

    /** @brief Сохраняет каждую пачку команд в отдельный log-файл. */
    class FileBulkSink final : public IBulkSink {
    public:
        /**
         * @brief Создаёт файловый получатель.
         * @param output_directory Каталог для log-файлов; пустой путь означает `.`.
         * @note Каталог создаётся при первой записи, а не в конструкторе.
         */
        explicit FileBulkSink(std::filesystem::path output_directory = ".");

        /**
         * @brief Записывает пачку в файл `bulk<timestamp>.log`.
         * @param command_bulk Пачка для записи; timestamp берётся из `started_at`.
         * @throws std::filesystem::filesystem_error При ошибке работы с каталогом.
         * @throws std::runtime_error При ошибке открытия или записи файла.
         */
        void write(const CommandBulk &command_bulk) override;

    private:
        /** @brief Формирует имя файла для пачки команд. */
        [[nodiscard]] std::filesystem::path file_path(TimePoint started_at) const;
    };

    /**
     * @brief Передаёт одну пачку нескольким получателям в заданном порядке.
     *
     * Класс не владеет получателями. `std::reference_wrapper` нужен потому, что
     * стандартный контейнер не может хранить обычные ссылки. Он хранит только
     * ненулевую ссылку и не копирует объект `IBulkSink`; все получатели должны
     * существовать дольше композиции.
     */
    class CompositeBulkSink final : public IBulkSink {
    public:
        /**
         * @brief Создаёт композицию получателей.
         * @param sinks Непустой набор не владеющих ссылок на получателей.
         * @throws std::invalid_argument Если набор пуст.
         */
        explicit CompositeBulkSink(std::vector<std::reference_wrapper<IBulkSink> > sinks);

        /**
         * @brief Передаёт пачку каждому получателю.
         * @param command_bulk Пачка для вывода.
         * @throws std::exception Если возникает ошибка при записи в получателе.
         */
        void write(const CommandBulk &command_bulk) override;
    };

    /**
     * @brief Связывает разбор строки, накопление пачки и её вывод.
     *
     * Контроллер владеет накопителем, но не владеет получателем `IBulkSink`.
     */
    class BulkController final {
    public:
        /**
         * @brief Создаёт контроллер пакетной обработки.
         * @param static_batch_size Размер статической пачки, больший нуля.
         * @param sink Получатель пачек; должен существовать дольше контроллера.
         * @throws std::invalid_argument Если размер пачки равен нулю.
         */
        BulkController(std::size_t static_batch_size, IBulkSink &sink);

        /**
         * @brief Обрабатывает одну исходную строку и при необходимости выводит пачку.
         * @param line Строка без символа перевода строки.
         * @param received_at Время получения строки.
         * @throws std::logic_error Если обработка входа уже завершена методом `finish`.
         * @throws std::exception Исключения накопителя и получателя передаются наружу.
         */
        void process_line(std::string line, TimePoint received_at);

        /**
         * @brief Сообщает о EOF конце ввода.
         *
         * Неполная статическая пачка выводится, незакрытая динамическая — удаляется.
         * Повторный успешный вызов ничего не делает.
         *
         * @throws std::exception Исключения получателя передаются наружу.
         */
        void finish();
    };
} // namespace bulk

#endif
