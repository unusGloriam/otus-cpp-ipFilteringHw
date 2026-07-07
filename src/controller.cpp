#include "controller.hpp"

#include <exception>
#include <utility>

namespace editor {
    EditorController::EditorController(
        EditorView &view,
        DocumentSerializer serializer,
        PrimitiveFactory factory)
        : view_(view),
          serializer_(std::move(serializer)),
          factory_(std::move(factory)),
          document_("untitled") {
    }

    void EditorController::NewDocument(std::string name) {
        document_ = Document(std::move(name));
        view_.ShowMessage("New document created");
        RenderCurrentDocument();
    }

    void EditorController::ImportDocument(const std::filesystem::path &path) {
        try {
            document_ = serializer_.ImportFromFile(path);
            view_.ShowMessage("Document imported");
            RenderCurrentDocument();
        } catch (const std::exception &exception) {
            view_.ShowError(exception.what());
        }
    }

    void EditorController::ExportDocument(const std::filesystem::path &path) const {
        try {
            serializer_.ExportToFile(document_, path);
            view_.ShowMessage("Document exported");
        } catch (const std::exception &exception) {
            view_.ShowError(exception.what());
        }
    }

    PrimitiveId EditorController::CreateLine(Point start, Point end, PrimitiveStyle style) {
        try {
            const auto id = document_.AddPrimitive(factory_.CreateLine(start, end, style));
            RenderCurrentDocument();
            return id;
        } catch (const std::exception &exception) {
            view_.ShowError(exception.what());
            return 0;
        }
    }

    bool EditorController::DeletePrimitive(PrimitiveId id) {
        const auto removed = document_.RemovePrimitive(id);
        if (removed) {
            view_.ShowMessage("Primitive deleted");
            RenderCurrentDocument();
        } else {
            view_.ShowError("Primitive not found");
        }

        return removed;
    }

    const Document &EditorController::GetDocument() const {
        return document_;
    }

    void EditorController::RenderCurrentDocument() {
        view_.Render(document_);
    }
} // namespace editor
