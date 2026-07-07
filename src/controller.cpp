#include "controller.hpp"
#include "serialization.hpp"

#include <exception>
#include <utility>

namespace editor {
    EditorController::EditorController(
        EditorView &view)
        : view_(view),
          document_("untitled") {
    }

    void EditorController::NewDocument(std::string name) {
        document_ = Document(std::move(name));
        view_.ShowMessage("New document created");
        RenderCurrentDocument();
    }

    void EditorController::ImportDocument(const std::filesystem::path &path) {
        try {
            document_ = DocumentSerializer::ImportFromFile(path);
            view_.ShowMessage("Document imported");
            RenderCurrentDocument();
        } catch (const std::exception &exception) {
            view_.ShowError(exception.what());
        }
    }

    void EditorController::ExportDocument(const std::filesystem::path &path) const {
        try {
            DocumentSerializer::ExportToFile(document_, path);
            view_.ShowMessage("Document exported");
        } catch (const std::exception &exception) {
            view_.ShowError(exception.what());
        }
    }

    PrimitiveId EditorController::CreateLine(Point start, Point end, PrimitiveStyle style) {
        try {
            const auto id = document_.AddPrimitive(PrimitiveFactory::CreateLine(start, end, style));
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

    void EditorController::RenderCurrentDocument() const {
        view_.Render(document_);
    }
} // namespace editor
