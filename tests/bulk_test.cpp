#include "bulk.hpp"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {
    bulk::TimePoint at_second(const std::int64_t seconds) {
        return bulk::TimePoint{std::chrono::seconds{seconds}};
    }

    class RecordingSink final : public bulk::IBulkSink {
    public:
        void write(const bulk::CommandBulk &command_bulk) override {
            bulks.push_back(command_bulk);
        }

        std::vector<bulk::CommandBulk> bulks;
    };

    TEST(BatchSizeParser, AcceptsOnlyAPositiveInteger) {
        EXPECT_EQ(bulk::parse_batch_size("1"), 1U);
        EXPECT_EQ(bulk::parse_batch_size("003"), 3U);
        EXPECT_EQ(bulk::parse_batch_size(
                      std::to_string(std::numeric_limits<std::size_t>::max())),
                  std::numeric_limits<std::size_t>::max());

        EXPECT_FALSE(bulk::parse_batch_size(""));
        EXPECT_FALSE(bulk::parse_batch_size("0"));
        EXPECT_FALSE(bulk::parse_batch_size("-1"));
        EXPECT_FALSE(bulk::parse_batch_size("+3"));
        EXPECT_FALSE(bulk::parse_batch_size(" 3"));
        EXPECT_FALSE(bulk::parse_batch_size("3 "));
        EXPECT_FALSE(bulk::parse_batch_size("3x"));
        EXPECT_FALSE(bulk::parse_batch_size("184467440737095516160"));
    }

    TEST(LineParser, RecognizesOnlyExactControlLinesAndNormalizesCrlf) {
        constexpr bulk::LineParser parser;

        EXPECT_TRUE(std::holds_alternative<bulk::OpenDynamicBlock>(parser.parse("{")));
        EXPECT_TRUE(
            std::holds_alternative<bulk::CloseDynamicBlock>(parser.parse("}\r")));

        const auto spaced = bulk::LineParser::parse(" { ");
        ASSERT_TRUE(std::holds_alternative<bulk::Command>(spaced));
        EXPECT_EQ(std::get<bulk::Command>(spaced).text, " { ");

        const auto command = bulk::LineParser::parse("command\r");
        ASSERT_TRUE(std::holds_alternative<bulk::Command>(command));
        EXPECT_EQ(std::get<bulk::Command>(command).text, "command");
    }

    TEST(BulkController, EmitsStaticBatchesAndAPartialBatchAtEndOfInput) {
        RecordingSink sink;
        bulk::BulkController controller{3, sink};

        controller.process_line("cmd1", at_second(100));
        controller.process_line("cmd2", at_second(101));
        EXPECT_TRUE(sink.bulks.empty());
        controller.process_line("cmd3", at_second(102));
        controller.process_line("cmd4", at_second(103));
        controller.process_line("cmd5", at_second(104));
        controller.finish();

        ASSERT_EQ(sink.bulks.size(), 2U);
        EXPECT_EQ(sink.bulks[0].commands,
                  (std::vector<std::string>{"cmd1", "cmd2", "cmd3"}));
        EXPECT_EQ(sink.bulks[0].started_at, at_second(100));
        EXPECT_EQ(sink.bulks[1].commands,
                  (std::vector<std::string>{"cmd4", "cmd5"}));
        EXPECT_EQ(sink.bulks[1].started_at, at_second(103));
    }

    TEST(BulkController, ImplementsTheCompleteDynamicBlockExample) {
        RecordingSink sink;
        bulk::BulkController controller{3, sink};
        std::int64_t time = 100;
        const auto send = [&](const std::string &line) {
            controller.process_line(line, at_second(time++));
        };

        send("cmd1");
        send("cmd2");
        send("{");
        send("cmd3");
        send("cmd4");
        send("}");
        send("{");
        send("cmd5");
        send("cmd6");
        send("{");
        send("cmd7");
        send("cmd8");
        send("}");
        send("cmd9");
        send("}");
        send("{");
        send("cmd10");
        send("cmd11");
        controller.finish();

        ASSERT_EQ(sink.bulks.size(), 3U);
        EXPECT_EQ(sink.bulks[0].commands,
                  (std::vector<std::string>{"cmd1", "cmd2"}));
        EXPECT_EQ(sink.bulks[1].commands,
                  (std::vector<std::string>{"cmd3", "cmd4"}));
        EXPECT_EQ(sink.bulks[2].commands,
                  (std::vector<std::string>{"cmd5", "cmd6", "cmd7", "cmd8",
                      "cmd9"}));
        EXPECT_EQ(sink.bulks[0].started_at, at_second(100));
        EXPECT_EQ(sink.bulks[1].started_at, at_second(103));
        EXPECT_EQ(sink.bulks[2].started_at, at_second(107));
    }

    TEST(BulkController, DoesNotSplitADynamicBlockAtTheStaticLimit) {
        RecordingSink sink;
        bulk::BulkController controller{2, sink};

        controller.process_line("{", at_second(1));
        controller.process_line("a", at_second(2));
        controller.process_line("{", at_second(3));
        controller.process_line("b", at_second(4));
        controller.process_line("}", at_second(5));
        controller.process_line("c", at_second(6));
        EXPECT_TRUE(sink.bulks.empty());
        controller.process_line("}", at_second(7));

        ASSERT_EQ(sink.bulks.size(), 1U);
        EXPECT_EQ(sink.bulks[0].commands,
                  (std::vector<std::string>{"a", "b", "c"}));
        EXPECT_EQ(sink.bulks[0].started_at, at_second(2));
    }

    TEST(BulkController, ResumesStaticBatchingAfterAClosedDynamicBlock) {
        RecordingSink sink;
        bulk::BulkController controller{3, sink};

        controller.process_line("{", at_second(1));
        controller.process_line("dynamic", at_second(2));
        controller.process_line("}", at_second(3));
        controller.process_line("static1", at_second(4));
        controller.process_line("static2", at_second(5));
        controller.finish();

        ASSERT_EQ(sink.bulks.size(), 2U);
        EXPECT_EQ(sink.bulks[0].commands,
                  (std::vector<std::string>{"dynamic"}));
        EXPECT_EQ(sink.bulks[1].commands,
                  (std::vector<std::string>{"static1", "static2"}));
        EXPECT_EQ(sink.bulks[1].started_at, at_second(4));
    }

    TEST(BulkController, DiscardsAnUnfinishedDynamicBlockAtEndOfInput) {
        RecordingSink sink;
        bulk::BulkController controller{3, sink};

        controller.process_line("static", at_second(10));
        controller.process_line("{", at_second(11));
        controller.process_line("dynamic1", at_second(12));
        controller.process_line("dynamic2", at_second(13));
        controller.process_line("{", at_second(14));
        controller.process_line("dynamic3", at_second(15));
        controller.process_line("}", at_second(16));
        controller.finish();

        ASSERT_EQ(sink.bulks.size(), 1U);
        EXPECT_EQ(sink.bulks[0].commands,
                  (std::vector<std::string>{"static"}));
    }

    TEST(BulkController, EmptyLinesAreCommandsAndEmptyBlocksProduceNothing) {
        RecordingSink sink;
        bulk::BulkController controller{1, sink};

        controller.process_line("{", at_second(1));
        controller.process_line("}", at_second(2));
        controller.process_line("", at_second(3));
        controller.finish();

        ASSERT_EQ(sink.bulks.size(), 1U);
        EXPECT_EQ(sink.bulks[0].commands, (std::vector<std::string>{""}));
        EXPECT_EQ(sink.bulks[0].started_at, at_second(3));
    }

    TEST(BulkController, TreatsAnUnmatchedClosingBraceAsACommand) {
        RecordingSink sink;
        bulk::BulkController controller{3, sink};

        controller.process_line("}", at_second(1));
        controller.process_line("{{", at_second(2));
        controller.process_line("}x", at_second(3));

        ASSERT_EQ(sink.bulks.size(), 1U);
        EXPECT_EQ(sink.bulks[0].commands,
                  (std::vector<std::string>{"}", "{{", "}x"}));
        EXPECT_EQ(sink.bulks[0].started_at, at_second(1));
    }

    TEST(BulkController, FinishIsIdempotentAndFurtherInputIsRejected) {
        RecordingSink sink;
        bulk::BulkController controller{3, sink};
        controller.process_line("cmd", at_second(1));

        controller.finish();
        controller.finish();

        ASSERT_EQ(sink.bulks.size(), 1U);
        EXPECT_THROW(controller.process_line("late", at_second(2)), std::logic_error);
    }

    TEST(BulkFormatting, UsesTheRequiredRepresentation) {
        EXPECT_EQ(bulk::format_bulk(
                      bulk::CommandBulk{{"cmd1", "cmd2", ""}, at_second(1)}),
                  "bulk: cmd1, cmd2, ");
    }

    TEST(BulkSinks, ConsoleAndCompositeSinksReceiveTheSameBulk) {
        std::ostringstream output;
        bulk::ConsoleBulkSink console{output};
        RecordingSink recording;
        bulk::CompositeBulkSink composite{
            std::vector<std::reference_wrapper<bulk::IBulkSink> >{console, recording}
        };
        const bulk::CommandBulk command_bulk{{"one", "two"}, at_second(50)};

        composite.write(command_bulk);

        EXPECT_EQ(output.str(), "bulk: one, two\n");
        ASSERT_EQ(recording.bulks.size(), 1U);
        EXPECT_EQ(recording.bulks[0].commands, command_bulk.commands);
    }

    TEST(ProcessStream, TreatsAReadFailureWithoutEofAsAnError) {
        std::istringstream input{"unread"};
        input.setstate(std::ios::failbit);
        RecordingSink sink;
        bulk::BulkController controller{3, sink};
        controller.process_line("pending", at_second(1));

        EXPECT_THROW(bulk::process_stream(input, controller), std::runtime_error);
        EXPECT_TRUE(sink.bulks.empty());

        controller.finish();
        ASSERT_EQ(sink.bulks.size(), 1U);
        EXPECT_EQ(sink.bulks[0].commands, std::vector<std::string>{"pending"});
    }

    TEST(Configuration, RejectsInvalidDependenciesAndBatchSize) {
        RecordingSink sink;
        EXPECT_THROW((bulk::BulkController{0, sink}), std::invalid_argument);
        EXPECT_THROW((bulk::CompositeBulkSink{
                         std::vector<std::reference_wrapper<bulk::IBulkSink>>{}}),
                     std::invalid_argument);
    }
} // namespace
