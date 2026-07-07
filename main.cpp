#include "editor/controller.hpp"
#include "editor/view.hpp"

#include <filesystem>

void OnNewDocument(editor::EditorController& controller) {
    controller.NewDocument("demo");
}

void OnImportDocument(
    editor::EditorController& controller,
    const std::filesystem::path& path) {
    controller.ImportDocument(path);
}

void OnExportDocument(
    const editor::EditorController& controller,
    const std::filesystem::path& path) {
    controller.ExportDocument(path);
}

editor::PrimitiveId OnCreateLine(
    editor::EditorController& controller,
    editor::Point start,
    editor::Point end) {
    return controller.CreateLine(start, end);
}

void OnDeletePrimitive(editor::EditorController& controller, editor::PrimitiveId id) {
    controller.DeletePrimitive(id);
}

int main() {
    editor::ConsoleView view;
    editor::EditorController controller(view);

    const auto demo_file = std::filesystem::temp_directory_path() / "editor_demo.vec";

    OnNewDocument(controller);
    const auto line_id = OnCreateLine(
        controller,
        editor::Point{.x = 0.0, .y = 0.0},
        editor::Point{.x = 10.0, .y = 5.0});
    OnExportDocument(controller, demo_file);
    OnDeletePrimitive(controller, line_id);
    OnImportDocument(controller, demo_file);

    return 0;
}
