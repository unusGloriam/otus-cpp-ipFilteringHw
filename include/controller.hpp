#pragma once

#include "model.hpp"
#include "serialization.hpp"
#include "view.hpp"

#include <filesystem>
#include <string>

namespace editor {
    /**
     * @brief MVC controller that handles editor use cases.
     */
    class EditorController {
    public:
        EditorController(
            EditorView &view,
            DocumentSerializer serializer = {},
            PrimitiveFactory factory = {});

        void NewDocument(std::string name = {});

        void ImportDocument(const std::filesystem::path &path);

        void ExportDocument(const std::filesystem::path &path) const;

        PrimitiveId CreateLine(Point start, Point end, PrimitiveStyle style = {});

        bool DeletePrimitive(PrimitiveId id);

        [[nodiscard]] const Document &GetDocument() const;

    private:
        EditorView &view_;
        DocumentSerializer serializer_;
        PrimitiveFactory factory_;
        Document document_;

        void RenderCurrentDocument();
    };
} // namespace editor
