#include "model.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace editor {
    LinePrimitive::LinePrimitive(Point start, Point end, PrimitiveStyle style)
        : start_(start),
          end_(end),
          style_(style) {
    }

    Point LinePrimitive::GetStart() const {
        return start_;
    }

    Point LinePrimitive::GetEnd() const {
        return end_;
    }

    PrimitiveType LinePrimitive::GetType() const {
        return PrimitiveType::Line;
    }

    BoundingBox LinePrimitive::GetBounds() const {
        return {
            .min = {.x = std::min(start_.x, end_.x), .y = std::min(start_.y, end_.y)},
            .max = {.x = std::max(start_.x, end_.x), .y = std::max(start_.y, end_.y)},
        };
    }

    const PrimitiveStyle &LinePrimitive::GetStyle() const {
        return style_;
    }

    std::unique_ptr<GraphicPrimitive> LinePrimitive::Clone() const {
        return std::make_unique<LinePrimitive>(*this);
    }

    void LinePrimitive::Accept(PrimitiveVisitor &visitor) const {
        visitor.Visit(*this);
    }

    Document::Document(std::string name)
        : name_(std::move(name)) {
    }

    const std::string &Document::GetName() const {
        return name_;
    }

    void Document::SetName(std::string name) {
        name_ = std::move(name);
    }

    PrimitiveId Document::AddPrimitive(std::unique_ptr<GraphicPrimitive> primitive) {
        if (!primitive) {
            throw std::invalid_argument("primitive must not be null");
        }

        const auto id = next_id_++;
        primitives_.emplace_back(id, std::move(primitive));
        return id;
    }

    bool Document::RemovePrimitive(PrimitiveId id) {
        return std::erase_if(primitives_, [id](const PrimitiveRecord &record) {
            return record.id == id;
        }) > 0;
    }

    const std::vector<PrimitiveRecord> &Document::GetPrimitives() const {
        return primitives_;
    }

    void Document::Clear() {
        primitives_.clear();
    }

    bool Document::Empty() const {
        return primitives_.empty();
    }

    std::unique_ptr<GraphicPrimitive> PrimitiveFactory::CreateLine(
        Point start,
        Point end,
        PrimitiveStyle style) {
        return std::make_unique<LinePrimitive>(start, end, style);
    }
} // namespace editor
