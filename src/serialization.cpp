#include "serialization.hpp"

#include <fstream>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

namespace editor {
    namespace {
        Color ReadColor(int red, int green, int blue) {
            if (constexpr int max_channel = 255;
                red < 0 || red > max_channel || green < 0 || green > max_channel || blue < 0 ||
                blue > max_channel) {
                throw std::runtime_error("invalid color channel");
            }

            return Color{
                .red = static_cast<std::byte>(red),
                .green = static_cast<std::byte>(green),
                .blue = static_cast<std::byte>(blue),
            };
        }

        class ExportVisitor final : public PrimitiveVisitor {
        public:
            explicit ExportVisitor(std::ostream &output)
                : output_(output) {
            }

            void Visit(const LinePrimitive &line) override {
                const auto [stroke_color, stroke_width] = line.GetStyle();
                const auto [x_0, y_0] = line.GetStart();
                const auto [x_1, y_1] = line.GetEnd();

                output_ << "line " << x_0 << ' ' << y_0 << ' ' << x_1 << ' ' << y_1 << ' '
                        << stroke_width << ' ' << std::to_integer<int>(stroke_color.red) << ' '
                        << std::to_integer<int>(stroke_color.green) << ' '
                        << std::to_integer<int>(stroke_color.blue) << '\n';
            }

        private:
            std::ostream &output_;
        };
    } // namespace

    Document DocumentSerializer::ImportFromFile(const std::filesystem::path &path) {
        std::ifstream input(path);
        if (!input.is_open()) {
            throw std::runtime_error("cannot open input file");
        }

        return Import(input, path.stem().string());
    }

    void DocumentSerializer::ExportToFile(
        const Document &document,
        const std::filesystem::path &path) {
        std::ofstream output(path);
        if (!output.is_open()) {
            throw std::runtime_error("cannot open output file");
        }

        Export(document, output);
    }

    Document DocumentSerializer::Import(std::istream &input, std::string name) {
        Document document(std::move(name));
        std::string line;

        while (std::getline(input, line)) {
            if (line.empty()) {
                continue;
            }

            std::istringstream parser(line);
            std::string record_type;
            parser >> record_type;

            if (record_type == "document") {
                std::string document_name;
                std::getline(parser >> std::ws, document_name);
                document.SetName(std::move(document_name));
                continue;
            }

            if (record_type == "line") {
                Point start;
                Point end;
                PrimitiveStyle style;
                int red = 0;
                int green = 0;
                int blue = 0;

                if (!(parser >> start.x >> start.y >> end.x >> end.y >> style.stroke_width >> red >>
                      green >> blue)) {
                    throw std::runtime_error("invalid line record");
                }

                style.stroke_color = ReadColor(red, green, blue);
                document.AddPrimitive(PrimitiveFactory::CreateLine(start, end, style));
                continue;
            }

            throw std::runtime_error("unknown record type");
        }

        if (!input.eof()) {
            throw std::runtime_error("cannot read input");
        }

        return document;
    }

    void DocumentSerializer::Export(const Document &document, std::ostream &output) {
        output << "document " << document.GetName() << '\n';

        ExportVisitor visitor(output);
        for (const auto &[id, primitive]: document.GetPrimitives()) {
            primitive->Accept(visitor);
        }
    }
} // namespace editor
