#include "PenumbraIcons/StandardIconBackend.h"

#include "Penumbra/Render/Renderer.h"

#include <algorithm>
#include <array>
#include <initializer_list>

namespace PenumbraIcons {

namespace {

using Penumbra::Point;
using Penumbra::Rect;
using Penumbra::Render::Color;
using Penumbra::Render::Renderer;

constexpr float GridSize = 24.0f;

class Pen {
public:
    Pen(Renderer& InTarget, Rect Bounds, Color InTint, float InThickness)
        : Target(InTarget), Tint(InTint), Thickness(InThickness) {
        const float Side = std::min(Bounds.W, Bounds.H);
        Origin = {Bounds.X + (Bounds.W - Side) * 0.5f, Bounds.Y + (Bounds.H - Side) * 0.5f};
        Scale  = Side / GridSize;
    }

    void Stroke(std::initializer_list<Point> GridPoints) const {
        const Point* Previous = nullptr;
        for (const Point& GridPoint : GridPoints) {
            if (Previous != nullptr) {
                Target.DrawLine(At(*Previous), At(GridPoint), Tint, Thickness);
            }
            Previous = &GridPoint;
        }
        for (const Point& GridPoint : GridPoints) {
            Disc(At(GridPoint), Thickness * 0.5f);
        }
    }

    void Dot(Point GridCenter, float GridRadius) const { Disc(At(GridCenter), GridRadius * Scale + Thickness * 0.5f); }

private:
    Point At(Point GridPoint) const { return {Origin.X + GridPoint.X * Scale, Origin.Y + GridPoint.Y * Scale}; }

    void Disc(Point Center, float Radius) const {
        Target.DrawFilledRect({Center.X - Radius, Center.Y - Radius, Radius * 2.0f, Radius * 2.0f}, Tint, Radius);
    }

    Renderer& Target;
    Color     Tint;
    float     Thickness;
    Point     Origin{};
    float     Scale{1.0f};
};

struct Glyph {
    std::string_view Name;
    void (*Draw)(const Pen&);
};

constexpr std::array Glyphs{
    Glyph{"close",
          [](const Pen& P) {
              P.Stroke({{6, 6}, {18, 18}});
              P.Stroke({{18, 6}, {6, 18}});
          }},
    Glyph{"chevron-left", [](const Pen& P) { P.Stroke({{15, 6}, {9, 12}, {15, 18}}); }},
    Glyph{"chevron-right", [](const Pen& P) { P.Stroke({{9, 6}, {15, 12}, {9, 18}}); }},
    Glyph{"chevron-up", [](const Pen& P) { P.Stroke({{6, 15}, {12, 9}, {18, 15}}); }},
    Glyph{"chevron-down", [](const Pen& P) { P.Stroke({{6, 9}, {12, 15}, {18, 9}}); }},
    Glyph{"hamburger",
          [](const Pen& P) {
              P.Stroke({{4, 6}, {20, 6}});
              P.Stroke({{4, 12}, {20, 12}});
              P.Stroke({{4, 18}, {20, 18}});
          }},
    Glyph{"kebab",
          [](const Pen& P) {
              P.Dot({12, 5}, 1.0f);
              P.Dot({12, 12}, 1.0f);
              P.Dot({12, 19}, 1.0f);
          }},
    Glyph{"plus",
          [](const Pen& P) {
              P.Stroke({{12, 5}, {12, 19}});
              P.Stroke({{5, 12}, {19, 12}});
          }},
    Glyph{"minus", [](const Pen& P) { P.Stroke({{5, 12}, {19, 12}}); }},
    Glyph{"check", [](const Pen& P) { P.Stroke({{4, 12}, {9, 17}, {20, 6}}); }},
    Glyph{"document",
          [](const Pen& P) {
              P.Stroke({{5, 3}, {14, 3}, {19, 8}, {19, 21}, {5, 21}, {5, 3}});
              P.Stroke({{14, 3}, {14, 8}, {19, 8}});
          }},
    Glyph{"folder",
          [](const Pen& P) {
              P.Stroke({{3, 7}, {3, 4}, {11, 4}, {11, 7}});
              P.Stroke({{3, 7}, {21, 7}, {21, 20}, {3, 20}, {3, 7}});
          }},
    Glyph{"tag",
          [](const Pen& P) {
              P.Stroke({{3, 6}, {15, 6}, {21, 12}, {15, 18}, {3, 18}, {3, 6}});
              P.Dot({7, 12}, 0.5f);
          }},
    Glyph{"list",
          [](const Pen& P) {
              for (const float Y : {6.0f, 12.0f, 18.0f}) {
                  P.Dot({4, Y}, 0.5f);
                  P.Stroke({{8, Y}, {20, Y}});
              }
          }},
};

} // namespace

StandardIconBackend::StandardIconBackend(float InStrokeThicknessLogical, Penumbra::Render::Color InDefaultColor)
    : StrokeThicknessLogical(InStrokeThicknessLogical), DefaultColor(InDefaultColor) {}

void StandardIconBackend::DrawIcon(Renderer& Renderer, std::string_view IconName, Rect BoundsLogical,
                                   Color IconColor) {
    const auto Found = std::ranges::find(Glyphs, IconName, &Glyph::Name);
    if (Found == Glyphs.end()) {
        return;
    }
    Found->Draw(Pen(Renderer, BoundsLogical, IconColor.A != 0 ? IconColor : DefaultColor, StrokeThicknessLogical));
}

std::span<const std::string_view> StandardIconBackend::Names() {
    static const auto All = [] {
        std::array<std::string_view, Glyphs.size()> Result{};
        std::ranges::transform(Glyphs, Result.begin(), &Glyph::Name);
        return Result;
    }();
    return All;
}

} // namespace PenumbraIcons
