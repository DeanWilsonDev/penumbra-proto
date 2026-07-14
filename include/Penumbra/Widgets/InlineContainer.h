#pragma once

#include "Penumbra/Widgets/Box.h"

#include <vector>

namespace Penumbra::Widgets {

// An inline-flow container: children are placed left to right and wrap to a new
// line when the next one would overflow the available content width — the run/
// paragraph layout Box's VerticalStack/HorizontalStack modes don't do. Not a text
// renderer (Label owns that) and not a LayoutMode on Box: wrapping needs a
// width-dependent line-breaking pass Box's single-pass stack layout never has to
// do, different enough to earn its own class instead of another enum value.
//
// Reuses Box wholesale for the box model, child storage/mutation, first-refusal
// input dispatch, and drawing (background/border/children) — only the geometry
// of where each child ends up changes, so only Measure/Arrange are overridden.
class InlineContainer : public Box {
public:
    // Vertical spacing between wrapped lines. Box's inherited ChildGap is reused
    // as the horizontal spacing between items on the same line — same "gap
    // between things" idea Box already exposes, just the other axis. Inherited
    // CrossAlignment governs how a line's items align within that line's height
    // (top/center/bottom/stretch), the same role it plays across Box's stacks.
    float LineGapLogical{0.0f};

    Point Measure(Point AvailableSizeLogical) override;
    void  Arrange(Rect FinalRectLogical) override;

private:
    // One child's already-measured footprint within a line.
    struct LineItem {
        WidgetBase* Child;
        Point       Desired;
        EdgeInsets  Margin;
    };

    // Measures visible children against ContentAvailable and greedily wraps them
    // into lines by width. Shared by Measure (to size the container) and Arrange
    // (to commit final rects) so the two passes can never disagree about where
    // the wraps fall for a given available width.
    std::vector<std::vector<LineItem>> BuildLines(Point ContentAvailable) const;
};

} // namespace Penumbra::Widgets
