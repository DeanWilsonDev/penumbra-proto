#pragma once

#include "Penumbra/Widgets/Box.h"

namespace Penumbra::Widgets {

enum class ChevronDirection { Right, Down };

// A theme-neutral, intrinsic two-segment chevron glyph.
class Chevron : public Box {
public:
    ChevronDirection Direction{ChevronDirection::Right};
    Render::Color    Color{0, 0, 0, 0};
    float            RadiusLogical{0.0f};
    float            ThicknessLogical{0.0f};
    Point            ContentSizeLogical{0.0f, 0.0f};

protected:
    Point MeasureContent(Point AvailableContentSize) override;
    void  DrawContent(Render::Renderer&, Rect ContentRect) override;
};

} // namespace Penumbra::Widgets
