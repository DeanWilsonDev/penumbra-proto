#pragma once

#include "Penumbra/Widgets/WidgetBase.h"

#include <memory>
#include <vector>

namespace Penumbra::Widgets {

// The universal primitive — the "div". Everything is a Box. A Box owns the box
// model, an optional list of children, and a layout mode for arranging them.
// "Container-ness" is just a Box that has children.
class Box : public WidgetBase {
public:
    BoxStyle                                 Style{};
    std::vector<std::unique_ptr<WidgetBase>> Children;
    LayoutMode                               Layout{LayoutMode::None};
    float                                    ChildGap{0.0f};
    CrossAlign                               CrossAlignment{CrossAlign::Start};

    WidgetBase* AddChild(std::unique_ptr<WidgetBase> Child);

    Point Measure(Point AvailableSizeLogical) override;
    void  Arrange(Rect FinalRectLogical) override;
    bool  UpdateInteractionState(const Platform::InputState&) override;
    void  Draw(Render::Renderer&) override;

    EdgeInsets GetMarginLogical() const override { return Style.Margin; }

protected:
    // Subclasses override these for their own intrinsic visuals (text, glyphs).
    virtual Point MeasureContent(Point AvailableContentSize) { return {0.0f, 0.0f}; }
    virtual void  DrawContent(Render::Renderer&, Rect ContentRect) {}

    // The box-model overhead (border + padding) on each axis, and the content rect
    // an outer rect collapses to once border and padding are removed.
    Point FrameSize() const;
    Rect  ContentRectFrom(Rect OuterRect) const;
};

} // namespace Penumbra::Widgets
