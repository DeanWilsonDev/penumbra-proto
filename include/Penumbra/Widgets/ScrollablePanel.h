#pragma once

#include "Penumbra/Widgets/Box.h"

namespace Penumbra::Widgets {

// A Box that clips its children to its own content rect, stacks them vertically,
// and offsets them by a wheel-driven scroll position. Its own size is the viewport
// (bounded by what its parent gives it), not the full height of its children.
class ScrollablePanel : public Box {
public:
    // Logical pixels scrolled per wheel notch. Zero (the default) means no opinion;
    // the demo supplies the value, keeping it out of Penumbra.
    float WheelStepLogical{0.0f};

    float GetScrollOffset() const { return ScrollOffsetY; }

    Point Measure(Point AvailableSizeLogical) override;
    void  Arrange(Rect FinalRectLogical) override;
    bool  UpdateInteractionState(const Platform::InputState&) override;
    void  Draw(Render::Renderer&) override;

private:
    float ScrollOffsetY{0.0f}; // how far the content is scrolled up, in logical px
    float ContentHeight{0.0f}; // total stacked height of children, from Measure
};

} // namespace Penumbra::Widgets
