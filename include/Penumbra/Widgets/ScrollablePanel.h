#pragma once

#include "Penumbra/Widgets/Box.h"

namespace Penumbra::Widgets {

// A Box that clips its children to its own content rect, stacks them vertically,
// and offsets them by a wheel-driven scroll position along whichever axis Direction
// names. Children are always laid out as a vertical column (main-axis stacking never
// changes) -- Direction instead picks which axis is the bounded, clipped/scrolled
// *viewport* and which is left sized-to-content:
//   Vertical (the default): Y is the viewport (bounded to whatever the parent offers,
//     scrolled by wheel), X is content-sized (reports its widest child's width, same
//     "auto" sizing an ordinary Box gives its cross axis). This is every existing use
//     of this widget (modal-body, swimlane columns).
//   Horizontal: X is the viewport instead (bounded, scrolled), Y is content-sized (the
//     sum of its stacked children's own heights, no vertical scroll at all in this
//     mode) -- for a block of text lines that must never wrap (white-space: nowrap)
//     but still needs to fit inside a fixed-width container, panned horizontally
//     rather than reflowed (give-code-blocks-their-own-horizontally-scrollable-non-
//     wrapping-frame-in-the-card-modal, Cairn's own Card Modal code blocks).
class ScrollablePanel : public Box {
public:
    enum class ScrollDirection { Vertical, Horizontal };

    ScrollDirection Direction{ScrollDirection::Vertical};

    // Logical pixels scrolled per wheel notch, one per axis -- only the field matching
    // Direction has any effect. Zero (the default) means no opinion; the embedding host
    // supplies the value, keeping it out of Penumbra.
    float WheelStepLogical{0.0f};           // Direction == Vertical
    float HorizontalWheelStepLogical{0.0f}; // Direction == Horizontal

    float GetScrollOffset() const { return ScrollOffsetY; }
    float GetScrollOffsetX() const { return ScrollOffsetX; }

    Point Measure(Point AvailableSizeLogical) override;
    void  Arrange(Rect FinalRectLogical) override;
    bool  UpdateInteractionState(const Platform::InputState&) override;
    void  Draw(Render::Renderer&) override;

    bool ConsumedWheelThisFrame() const override { return WheelHandledThisFrame; }

private:
    float ScrollOffsetY{0.0f}; // how far the content is scrolled up, in logical px
    float ScrollOffsetX{0.0f}; // how far the content is scrolled left, in logical px
    float ContentHeight{0.0f}; // total stacked height of children, from Measure
    float ContentWidth{0.0f};  // widest child, from Measure -- Direction == Horizontal's
                                // own scroll-clamp bound, the ContentHeight counterpart
    bool  WheelHandledThisFrame{false};
};

} // namespace Penumbra::Widgets
