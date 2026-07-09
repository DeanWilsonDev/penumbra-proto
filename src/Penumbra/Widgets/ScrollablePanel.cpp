#include "Penumbra/Widgets/ScrollablePanel.h"

#include <algorithm>
#include <utility>

namespace Penumbra::Widgets {

namespace {

float NonNegative(float Value) { return Value > 0.0f ? Value : 0.0f; }

float Clamp(float Value, float Low, float High) {
    return std::min(std::max(Value, Low), High);
}

bool PointInRect(Point Point, Rect Rect) {
    return Point.X >= Rect.X && Point.X < Rect.X + Rect.W &&
           Point.Y >= Rect.Y && Point.Y < Rect.Y + Rect.H;
}

} // namespace

Point ScrollablePanel::Measure(Point AvailableSizeLogical) {
    const Point Frame = FrameSize();
    const Point ContentAvailable{NonNegative(AvailableSizeLogical.X - Frame.X),
                                 NonNegative(AvailableSizeLogical.Y - Frame.Y)};

    float Total = 0.0f;
    float MaxWidth = 0.0f;
    for (std::size_t Index = 0; Index < Children.size(); ++Index) {
        WidgetBase* Child = Children[Index].get();
        const EdgeInsets Margin = Child->GetMarginLogical();
        const Point ChildAvailable{NonNegative(ContentAvailable.X - Margin.Left - Margin.Right),
                                   NonNegative(ContentAvailable.Y - Margin.Top - Margin.Bottom)};
        const Point Desired = Child->Measure(ChildAvailable);

        if (Index > 0) {
            Total += ChildGap;
        }
        Total += Desired.Y + Margin.Top + Margin.Bottom;
        MaxWidth = std::max(MaxWidth, Desired.X + Margin.Left + Margin.Right);
    }

    ContentHeight = Total;

    // The panel fills the height it is offered (it is a viewport, not content-sized).
    return {MaxWidth + Frame.X, AvailableSizeLogical.Y};
}

void ScrollablePanel::Arrange(Rect FinalRectLogical) {
    ArrangedRect = FinalRectLogical;

    const Rect Content = ContentRectFrom(FinalRectLogical);
    const float MaxScroll = NonNegative(ContentHeight - Content.H);
    ScrollOffsetY = Clamp(ScrollOffsetY, 0.0f, MaxScroll);

    // Lay children out as a vertical column, shifted up by the scroll offset.
    // Positions may fall outside the viewport; the clip in Draw hides them.
    float CursorY = Content.Y - ScrollOffsetY;
    const std::size_t Count = Children.size();
    for (std::size_t Index = 0; Index < Count; ++Index) {
        WidgetBase* Child = Children[Index].get();
        const EdgeInsets Margin = Child->GetMarginLogical();

        const Point ChildAvailable{NonNegative(Content.W - Margin.Left - Margin.Right),
                                   NonNegative(Content.H - Margin.Top - Margin.Bottom)};
        const Point Desired = Child->Measure(ChildAvailable);

        const float AvailableCross = NonNegative(Content.W - Margin.Left - Margin.Right);
        float ChildX = Content.X + Margin.Left;
        float ChildWidth = Desired.X;
        switch (CrossAlignment) {
        case CrossAlign::Start:
            break;
        case CrossAlign::Center:
            ChildX = Content.X + Margin.Left + (AvailableCross - Desired.X) / 2.0f;
            break;
        case CrossAlign::End:
            ChildX = Content.X + Content.W - Margin.Right - Desired.X;
            break;
        case CrossAlign::Stretch:
            ChildWidth = AvailableCross;
            break;
        }

        const float ChildY = CursorY + Margin.Top;
        Child->Arrange({ChildX, ChildY, ChildWidth, Desired.Y});

        CursorY = ChildY + Desired.Y + Margin.Bottom;
        if (Index + 1 < Count) {
            CursorY += ChildGap;
        }
    }
}

bool ScrollablePanel::UpdateInteractionState(const Platform::InputState& Input) {
    const bool OverPanel = PointInRect(Input.MousePosition, ArrangedRect);
    if (!OverPanel) {
        return false;
    }

    // Children's hit region is restricted to the clipped content rect, so widgets
    // scrolled out of view cannot be interacted with.
    const Rect Content = ContentRectFrom(ArrangedRect);
    bool Consumed = false;
    if (PointInRect(Input.MousePosition, Content)) {
        for (auto Iterator = Children.rbegin(); Iterator != Children.rend(); ++Iterator) {
            if ((*Iterator)->UpdateInteractionState(Input)) {
                Consumed = true;
                break;
            }
        }
    }

    if (WheelStepLogical != 0.0f && Input.MouseWheelDelta != 0.0f) {
        const float MaxScroll = NonNegative(ContentHeight - Content.H);
        ScrollOffsetY = Clamp(ScrollOffsetY - Input.MouseWheelDelta * WheelStepLogical, 0.0f, MaxScroll);
        Consumed = true;
    }

    // The panel is opaque: being over it consumes input regardless.
    return Consumed || OverPanel;
}

void ScrollablePanel::Draw(Render::Renderer& Renderer) {
    if (Style.ColorBackground.A != 0) {
        Renderer.DrawFilledRect(ArrangedRect, Style.ColorBackground, Style.BorderRadius);
    }
    if (Style.BorderWidth > 0.0f && Style.ColorBorder.A != 0) {
        Renderer.DrawRectOutline(ArrangedRect, Style.ColorBorder, Style.BorderWidth, Style.BorderRadius);
    }

    const Rect Content = ContentRectFrom(ArrangedRect);
    Renderer.PushClipRect(Content);
    for (auto& Child : Children) {
        Child->Draw(Renderer);
    }
    Renderer.PopClipRect();
}

} // namespace Penumbra::Widgets
