#ifndef BULK_HPP
#define BULK_HPP

/**
 * @file bulk.hpp
 * @brief Разбор входных строк и накопление блоков команд.
 */

#include <chrono>
#include <cstddef>
#include <functional>
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
} // namespace bulk

#endif
