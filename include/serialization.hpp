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
        [[nodiscard]] Document ImportFromFile(const std::filesystem::path &path) const;

        void ExportToFile(const Document &document, const std::filesystem::path &path) const;

        [[nodiscard]] Document Import(std::istream &input, std::string name = {}) const;

        void Export(const Document &document, std::ostream &output) const;
    };
} // namespace editor
