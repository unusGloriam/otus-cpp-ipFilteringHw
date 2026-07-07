#pragma once

#include "model.hpp"

#include <filesystem>
#include <iosfwd>
#include <string>

namespace editor {
    /**
     * @brief Imports and exports editor documents without coupling the model to file formats.
     */
    class DocumentSerializer {
    public:
        static Document ImportFromFile(const std::filesystem::path &path);

        static void ExportToFile(const Document &document, const std::filesystem::path &path);

        static Document Import(std::istream &input, std::string name = {});

        static void Export(const Document &document, std::ostream &output);
    };
} // namespace editor
