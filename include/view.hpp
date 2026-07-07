#pragma once

#include "model.hpp"

#include <string>

namespace editor {
    /**
     * @brief Abstract view in the editor MVC layout.
     */
    class EditorView {
    public:
        virtual ~EditorView() = default;

        virtual void Render(const Document &document) = 0;

        virtual void ShowMessage(const std::string &message) = 0;

        virtual void ShowError(const std::string &message) = 0;
    };

    /**
     * @brief Minimal console implementation of the editor view.
     */
    class ConsoleView final : public EditorView {
    public:
        void Render(const Document &document) override;

        void ShowMessage(const std::string &message) override;

        void ShowError(const std::string &message) override;
    };
} // namespace editor
