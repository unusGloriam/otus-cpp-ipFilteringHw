#include "bulk.hpp"

#include <iostream>

namespace {
    void print_usage() {
        std::cerr << "Usage: bulk <positive batch size>\n";
    }
} // namespace

int main(const int argc, char *argv[]) {
    if (argc != 2) {
        print_usage();
        return EXIT_FAILURE;
    }

    const auto batch_size = bulk::parse_batch_size(argv[1]);
    if (!batch_size) {
        print_usage();
        return EXIT_FAILURE;
    }

    try {
        bulk::ConsoleBulkSink console_sink{};
        bulk::FileBulkSink file_sink;
        bulk::CompositeBulkSink output{
            std::vector<std::reference_wrapper<bulk::IBulkSink> >{console_sink, file_sink}
        };
        bulk::BulkController controller{*batch_size, output};
        bulk::process_stream(std::cin, controller);
    } catch (const std::exception &error) {
        std::cerr << "bulk: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
