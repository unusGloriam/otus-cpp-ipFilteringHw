#include "view.hpp"

#include <iostream>

namespace editor {
    void ConsoleView::Render(const Document &document) {
        std::cout << "Document: " << document.GetName() << '\n';
        std::cout << "Primitives: " << document.GetPrimitives().size() << '\n';

        for (const auto &[id, primitive]: document.GetPrimitives()) {
            std::cout << "  #" << id << " type="
                    << (primitive->GetType() == PrimitiveType::Line ? "line" : "unknown")
                    << '\n';
        }
    }

    void ConsoleView::ShowMessage(const std::string &message) {
        std::cout << message << '\n';
    }

    void ConsoleView::ShowError(const std::string &message) {
        std::cerr << message << '\n';
    }
} // namespace editor
