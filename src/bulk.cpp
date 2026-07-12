#include "bulk.hpp"

#include <charconv>
#include <exception>
#include <fstream>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <utility>

namespace bulk {
    ParsedLine LineParser::parse(std::string line) {
        // std::getline removes '\n', while a CRLF stream can leave the '\r'.
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line == "{") {
            return OpenDynamicBlock{};
        }
        if (line == "}") {
            return CloseDynamicBlock{};
        }
        return Command{std::move(line)};
    }

    std::string format_bulk(const CommandBulk &command_bulk) {
        std::string result{"bulk: "};
        for (std::size_t index = 0; index < command_bulk.commands.size(); ++index) {
            if (index != 0) {
                result += ", ";
            }
            result += command_bulk.commands[index];
        }
        return result;
    }

    BulkAccumulator::BulkAccumulator(const std::size_t static_batch_size)
        : static_batch_size_{static_batch_size} {
        if (static_batch_size_ == 0) {
            throw std::invalid_argument{"batch size must be greater than zero"};
        }
    }

    std::optional<CommandBulk> BulkAccumulator::accept(ParsedLine line,
                                                       const TimePoint received_at) {
        /// Про } при отсутствующем { не сказано в задании, считать ли ошибкой. Пусть это воспринимается как команда.
        if (std::holds_alternative<CloseDynamicBlock>(line) && dynamic_depth_ == 0) {
            line = Command{"}"};
        }

        if (std::holds_alternative<Command>(line)) {
            if (commands_.empty()) {
                started_at_ = received_at;
            }
            commands_.push_back(std::move(std::get<Command>(line).text));

            if (dynamic_depth_ == 0 && commands_.size() == static_batch_size_) {
                return release();
            }
            return std::nullopt;
        }

        if (std::holds_alternative<OpenDynamicBlock>(line)) {
            auto pending_bulk = dynamic_depth_ == 0 ? release() : std::nullopt;
            ++dynamic_depth_;
            return pending_bulk;
        }

        if (std::holds_alternative<CloseDynamicBlock>(line)) {
            --dynamic_depth_;
            if (dynamic_depth_ == 0) {
                return release();
            }
        }
        return std::nullopt;
    }

    std::optional<CommandBulk> BulkAccumulator::finish() {
        if (dynamic_depth_ != 0) {
            dynamic_depth_ = 0;
            discard();
            return std::nullopt;
        }
        return release();
    }

    std::optional<CommandBulk> BulkAccumulator::release() {
        if (commands_.empty()) {
            return std::nullopt;
        }
        if (!started_at_) {
            throw std::logic_error{"a non-empty bulk has no start time"};
        }

        CommandBulk command_bulk{std::move(commands_), *started_at_};
        commands_.clear();
        started_at_.reset();
        return command_bulk;
    }

    void BulkAccumulator::discard() noexcept {
        commands_.clear();
        started_at_.reset();
    }

    ConsoleBulkSink::ConsoleBulkSink(std::ostream &output) noexcept : output_{output} {
    }

    void ConsoleBulkSink::write(const CommandBulk &command_bulk) {
        output_ << format_bulk(command_bulk) << '\n';
        output_.flush();
        if (!output_) {
            throw std::runtime_error{"failed to write a bulk to the console"};
        }
    }

    FileBulkSink::FileBulkSink(std::filesystem::path output_directory)
        : output_directory_{
            output_directory.empty()
                ? std::filesystem::path{"."}
                : std::move(output_directory)
        } {
    }

    void FileBulkSink::write(const CommandBulk &command_bulk) {
        std::filesystem::create_directories(output_directory_);
        const auto log_path = file_path(command_bulk.started_at);

        std::ofstream output{log_path, std::ios::out | std::ios::trunc};
        if (!output) {
            throw std::runtime_error{"failed to open log file: " + log_path.string()};
        }

        output << format_bulk(command_bulk) << '\n';
        output.close();
        if (!output) {
            throw std::runtime_error{"failed to write log file: " + log_path.string()};
        }
    }

    std::filesystem::path FileBulkSink::file_path(const TimePoint started_at) const {
        const auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
            started_at.time_since_epoch()).count();
        const std::string basename = "bulk" + std::to_string(timestamp);
        return output_directory_ / (basename + ".log");
    }

    CompositeBulkSink::CompositeBulkSink(std::vector<std::reference_wrapper<IBulkSink> > sinks)
        : sinks_{std::move(sinks)} {
        if (sinks_.empty()) {
            throw std::invalid_argument{"at least one bulk sink is required"};
        }
    }

    void CompositeBulkSink::write(const CommandBulk &command_bulk) {
        for (const auto &sink: sinks_) {
            try {
                sink.get().write(command_bulk);
            } catch (const std::exception &) {
                throw;
            }
        }
    }

    BulkController::BulkController(const std::size_t static_batch_size, IBulkSink &sink)
        : accumulator_{static_batch_size}, sink_{sink} {
    }

    void BulkController::process_line(std::string line, const TimePoint received_at) {
        if (finished_) {
            throw std::logic_error{"cannot process a line after end of input"};
        }

        if (const auto command_bulk =
                accumulator_.accept(LineParser::parse(std::move(line)), received_at)) {
            sink_.write(*command_bulk);
        }
    }

    void BulkController::finish() {
        if (finished_) {
            return;
        }

        if (const auto command_bulk = accumulator_.finish()) {
            sink_.write(*command_bulk);
        }
        finished_ = true;
    }

    std::optional<std::size_t> parse_batch_size(const std::string_view value) noexcept {
        if (value.empty()) {
            return std::nullopt;
        }

        std::size_t batch_size{};
        const char *const begin = value.data();
        const char *const end = begin + value.size();
        if (const auto [position, error] = std::from_chars(begin, end, batch_size);
            error != std::errc{} || position != end || batch_size == 0) {
            return std::nullopt;
        }
        return batch_size;
    }

    void process_stream(std::istream &input, BulkController &controller, Clock clock) {
        if (!clock) {
            clock = [] { return std::chrono::system_clock::now(); };
        }

        std::string line;
        while (std::getline(input, line)) {
            controller.process_line(line, clock());
        }

        if (input.bad() || !input.eof()) {
            throw std::runtime_error{"failed to read a command from the input"};
        }
        controller.finish();
    }
} // namespace bulk
