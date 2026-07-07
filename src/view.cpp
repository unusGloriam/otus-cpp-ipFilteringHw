#include "editor/view.hpp"

#include <iostream>

namespace editor {

void ConsoleView::Render(const Document& document) {
    std::cout << "Document: " << document.GetName() << '\n';
    std::cout << "Primitives: " << document.GetPrimitives().size() << '\n';

    for (const auto& record : document.GetPrimitives()) {
        std::cout << "  #" << record.id << " type="
                  << (record.primitive->GetType() == PrimitiveType::Line ? "line" : "unknown")
                  << '\n';
    }
}

void ConsoleView::ShowMessage(const std::string& message) {
    std::cout << message << '\n';
}

void ConsoleView::ShowError(const std::string& message) {
    std::cerr << message << '\n';
}

}  // namespace editor
