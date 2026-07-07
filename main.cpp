#include "controller.hpp"
#include "view.hpp"

#include <filesystem>

using namespace editor;

void OnNewDocument(EditorController &controller) {
    controller.NewDocument("demo");
}

void OnImportDocument(EditorController &controller, const std::filesystem::path &path) {
    controller.ImportDocument(path);
}

void OnExportDocument(const EditorController &controller, const std::filesystem::path &path) {
    controller.ExportDocument(path);
}

PrimitiveId OnCreateLine(EditorController &controller, Point start, Point end) {
    return controller.CreateLine(start, end);
}

void OnDeletePrimitive(EditorController &controller, PrimitiveId id) {
    controller.DeletePrimitive(id);
}

int main() {
    ConsoleView view;
    EditorController controller(view);

    const auto demo_file = std::filesystem::temp_directory_path() / "editor_demo.vec";

    OnNewDocument(controller);
    const auto line_id = OnCreateLine(controller, Point{.x = 0.0, .y = 0.0}, Point{.x = 10.0, .y = 5.0});
    OnExportDocument(controller, demo_file);
    OnDeletePrimitive(controller, line_id);
    OnImportDocument(controller, demo_file);

    return 0;
}
