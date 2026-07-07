#include "editor/controller.hpp"
#include "editor/model.hpp"
#include "editor/serialization.hpp"
#include "editor/view.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

namespace {

class FakeView final : public editor::EditorView {
public:
    void Render(const editor::Document&) override {
        ++render_count;
    }

    void ShowMessage(const std::string&) override {
        ++message_count;
    }

    void ShowError(const std::string&) override {
        ++error_count;
    }

    int render_count = 0;
    int message_count = 0;
    int error_count = 0;
};

}  // namespace

TEST(EditorArchitecture, HeadersAreSelfContained) {
    SUCCEED();
}

TEST(Document, AddsAndRemovesPrimitives) {
    editor::Document document("test");
    const editor::PrimitiveFactory factory;

    const auto id = document.AddPrimitive(
        factory.CreateLine(editor::Point{.x = 1.0, .y = 2.0}, editor::Point{.x = 3.0, .y = 4.0}));

    EXPECT_NE(id, 0);
    ASSERT_EQ(document.GetPrimitives().size(), 1U);
    EXPECT_EQ(document.GetPrimitives().front().id, id);

    EXPECT_TRUE(document.RemovePrimitive(id));
    EXPECT_TRUE(document.Empty());
    EXPECT_FALSE(document.RemovePrimitive(id));
}

TEST(Controller, CreatesAndDeletesLine) {
    FakeView view;
    editor::EditorController controller(view);

    controller.NewDocument("controller-test");
    const auto id = controller.CreateLine(
        editor::Point{.x = 0.0, .y = 0.0},
        editor::Point{.x = 1.0, .y = 1.0});

    EXPECT_NE(id, 0);
    ASSERT_EQ(controller.GetDocument().GetPrimitives().size(), 1U);
    EXPECT_TRUE(controller.DeletePrimitive(id));
    EXPECT_TRUE(controller.GetDocument().Empty());
    EXPECT_EQ(view.error_count, 0);
}

TEST(Serializer, RoundTripsLineDocument) {
    editor::Document document("round-trip");
    const editor::PrimitiveFactory factory;
    document.AddPrimitive(factory.CreateLine(
        editor::Point{.x = 2.0, .y = 3.0},
        editor::Point{.x = 5.0, .y = 7.0},
        editor::PrimitiveStyle{
            .stroke_color = {.red = 10, .green = 20, .blue = 30},
            .stroke_width = 2.5,
        }));

    editor::DocumentSerializer serializer;
    std::stringstream stream;

    serializer.Export(document, stream);
    auto imported = serializer.Import(stream);

    EXPECT_EQ(imported.GetName(), "round-trip");
    ASSERT_EQ(imported.GetPrimitives().size(), 1U);
    EXPECT_EQ(imported.GetPrimitives().front().primitive->GetType(), editor::PrimitiveType::Line);
}
