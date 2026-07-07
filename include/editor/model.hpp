#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace editor {

/**
 * @brief Two-dimensional point in document coordinates.
 */
struct Point {
    double x = 0.0;
    double y = 0.0;
};

/**
 * @brief Axis-aligned bounds of a graphic primitive.
 */
struct BoundingBox {
    Point min;
    Point max;
};

/**
 * @brief RGB color used by primitive styles.
 */
struct Color {
    unsigned char red = 0;
    unsigned char green = 0;
    unsigned char blue = 0;
};

/**
 * @brief Drawing style shared by vector primitives.
 */
struct PrimitiveStyle {
    Color stroke_color;
    double stroke_width = 1.0;
};

class LinePrimitive;

/**
 * @brief Visitor interface for type-specific primitive operations.
 */
class PrimitiveVisitor {
public:
    virtual ~PrimitiveVisitor() = default;

    virtual void Visit(const LinePrimitive& line) = 0;
};

/**
 * @brief Runtime primitive kind.
 */
enum class PrimitiveType {
    Line
};

/**
 * @brief Polymorphic base class for drawable model primitives.
 */
class GraphicPrimitive {
public:
    virtual ~GraphicPrimitive() = default;

    [[nodiscard]] virtual PrimitiveType GetType() const = 0;
    [[nodiscard]] virtual BoundingBox GetBounds() const = 0;
    [[nodiscard]] virtual const PrimitiveStyle& GetStyle() const = 0;
    [[nodiscard]] virtual std::unique_ptr<GraphicPrimitive> Clone() const = 0;
    virtual void Accept(PrimitiveVisitor& visitor) const = 0;
};

/**
 * @brief Straight line segment primitive.
 */
class LinePrimitive final : public GraphicPrimitive {
public:
    LinePrimitive(Point start, Point end, PrimitiveStyle style = {});

    [[nodiscard]] Point GetStart() const;
    [[nodiscard]] Point GetEnd() const;

    [[nodiscard]] PrimitiveType GetType() const override;
    [[nodiscard]] BoundingBox GetBounds() const override;
    [[nodiscard]] const PrimitiveStyle& GetStyle() const override;
    [[nodiscard]] std::unique_ptr<GraphicPrimitive> Clone() const override;
    void Accept(PrimitiveVisitor& visitor) const override;

private:
    Point start_;
    Point end_;
    PrimitiveStyle style_;
};

using PrimitiveId = std::uint64_t;

/**
 * @brief Document-owned primitive with its stable document id.
 */
struct PrimitiveRecord {
    PrimitiveId id = 0;
    std::unique_ptr<GraphicPrimitive> primitive;
};

/**
 * @brief Vector editor document that owns all graphic primitives.
 */
class Document {
public:
    explicit Document(std::string name = {});

    [[nodiscard]] const std::string& GetName() const;
    void SetName(std::string name);

    PrimitiveId AddPrimitive(std::unique_ptr<GraphicPrimitive> primitive);
    bool RemovePrimitive(PrimitiveId id);
    [[nodiscard]] const std::vector<PrimitiveRecord>& GetPrimitives() const;
    void Clear();
    [[nodiscard]] bool Empty() const;

private:
    std::string name_;
    PrimitiveId next_id_ = 1;
    std::vector<PrimitiveRecord> primitives_;
};

/**
 * @brief Factory for creating model primitives without exposing concrete ownership details.
 */
class PrimitiveFactory {
public:
    [[nodiscard]] std::unique_ptr<GraphicPrimitive> CreateLine(
        Point start,
        Point end,
        PrimitiveStyle style = {}) const;
};

}  // namespace editor
