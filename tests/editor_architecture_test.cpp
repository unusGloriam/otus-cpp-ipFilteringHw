#include "controller.hpp"
#include "model.hpp"
#include "serialization.hpp"
#include "view.hpp"

#include <gtest/gtest.h>

#include <sstream>
#include <string>

namespace {
    class FakeView final : public editor::EditorView {
    public:
        void Render(const editor::Document &) override {
            ++render_count;
        }

        void ShowMessage(const std::string &) override {
            ++message_count;
        }

        void ShowError(const std::string &) override {
            ++error_count;
        }

        int render_count = 0;
        int message_count = 0;
        int error_count = 0;
    };
} // namespace

TEST(EditorArchitecture, HeadersAreSelfContained) {
    SUCCEED();
}

TEST(Document, AddsAndRemovesPrimitives) {
    using namespace editor;
    Document document("test");

    const auto id = document.AddPrimitive(
        PrimitiveFactory::CreateLine(Point{.x = 1.0, .y = 2.0}, Point{.x = 3.0, .y = 4.0}));

    EXPECT_NE(id, 0);
    ASSERT_EQ(document.GetPrimitives().size(), 1LU);
    EXPECT_EQ(document.GetPrimitives().front().id, id);

    EXPECT_TRUE(document.RemovePrimitive(id));
    EXPECT_TRUE(document.Empty());
    EXPECT_FALSE(document.RemovePrimitive(id));
}

TEST(Controller, CreatesAndDeletesLine) {
    using namespace editor;
    FakeView view;
    EditorController controller(view);

    controller.NewDocument("controller-test");
    const auto id = controller.CreateLine(Point{.x = 0.0, .y = 0.0}, Point{.x = 1.0, .y = 1.0});

    EXPECT_NE(id, 0);
    ASSERT_EQ(controller.GetDocument().GetPrimitives().size(), 1LU);
    EXPECT_TRUE(controller.DeletePrimitive(id));
    EXPECT_TRUE(controller.GetDocument().Empty());
    EXPECT_EQ(view.error_count, 0);
}

TEST(Serializer, RoundTripsLineDocument) {
    using namespace editor;
    Document document("round-trip");
    document.AddPrimitive(PrimitiveFactory::CreateLine(Point{.x = 2.0, .y = 3.0}, Point{.x = 5.0, .y = 7.0},
                                                       PrimitiveStyle{
                                                           .stroke_color = {
                                                               .red = static_cast<std::byte>(10),
                                                               .green = static_cast<std::byte>(20),
                                                               .blue = static_cast<std::byte>(30)
                                                           },
                                                           .stroke_width = 2.5,
                                                       }));

    std::stringstream stream;

    DocumentSerializer::Export(document, stream);
    const auto imported = DocumentSerializer::Import(stream);

    EXPECT_EQ(imported.GetName(), "round-trip");
    ASSERT_EQ(imported.GetPrimitives().size(), 1U);
    EXPECT_EQ(imported.GetPrimitives().front().primitive->GetType(), editor::PrimitiveType::Line);
}
