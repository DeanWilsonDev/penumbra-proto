#include "Penumbra/Widgets/ScrollablePanel.h"

#include <algorithm>
#include <utility>

namespace Penumbra::Widgets {

namespace {

float NonNegative(float Value) { return Value > 0.0f ? Value : 0.0f; }

float Clamp(float Value, float Low, float High) {
    return std::min(std::max(Value, Low), High);
}

bool PointInRect(SDL_FPoint Point, SDL_FRect Rect) {
    return Point.x >= Rect.x && Point.x < Rect.x + Rect.w &&
           Point.y >= Rect.y && Point.y < Rect.y + Rect.h;
}

} // namespace

SDL_FPoint ScrollablePanel::Measure(SDL_FPoint AvailableSizeLogical) {
    const SDL_FPoint Frame = FrameSize();
    const SDL_FPoint ContentAvailable{NonNegative(AvailableSizeLogical.x - Frame.x),
                                      NonNegative(AvailableSizeLogical.y - Frame.y)};

    float Total = 0.0f;
    float MaxWidth = 0.0f;
    for (std::size_t Index = 0; Index < Children.size(); ++Index) {
        WidgetBase* Child = Children[Index].get();
        const EdgeInsets Margin = Child->GetMarginLogical();
        const SDL_FPoint ChildAvailable{NonNegative(ContentAvailable.x - Margin.Left - Margin.Right),
                                        NonNegative(ContentAvailable.y - Margin.Top - Margin.Bottom)};
        const SDL_FPoint Desired = Child->Measure(ChildAvailable);

        if (Index > 0) {
            Total += ChildGap;
        }
        Total += Desired.y + Margin.Top + Margin.Bottom;
        MaxWidth = std::max(MaxWidth, Desired.x + Margin.Left + Margin.Right);
    }

    ContentHeight = Total;

    // The panel fills the height it is offered (it is a viewport, not content-sized).
    return {MaxWidth + Frame.x, AvailableSizeLogical.y};
}

void ScrollablePanel::Arrange(SDL_FRect FinalRectLogical) {
    ArrangedRect = FinalRectLogical;

    const SDL_FRect Content = ContentRectFrom(FinalRectLogical);
    const float MaxScroll = NonNegative(ContentHeight - Content.h);
    ScrollOffsetY = Clamp(ScrollOffsetY, 0.0f, MaxScroll);

    // Lay children out as a vertical column, shifted up by the scroll offset.
    // Positions may fall outside the viewport; the clip in Draw hides them.
    float CursorY = Content.y - ScrollOffsetY;
    const std::size_t Count = Children.size();
    for (std::size_t Index = 0; Index < Count; ++Index) {
        WidgetBase* Child = Children[Index].get();
        const EdgeInsets Margin = Child->GetMarginLogical();

        const SDL_FPoint ChildAvailable{NonNegative(Content.w - Margin.Left - Margin.Right),
                                        NonNegative(Content.h - Margin.Top - Margin.Bottom)};
        const SDL_FPoint Desired = Child->Measure(ChildAvailable);

        const float AvailableCross = NonNegative(Content.w - Margin.Left - Margin.Right);
        float ChildX = Content.x + Margin.Left;
        float ChildWidth = Desired.x;
        switch (CrossAlignment) {
        case CrossAlign::Start:
            break;
        case CrossAlign::Center:
            ChildX = Content.x + Margin.Left + (AvailableCross - Desired.x) / 2.0f;
            break;
        case CrossAlign::End:
            ChildX = Content.x + Content.w - Margin.Right - Desired.x;
            break;
        case CrossAlign::Stretch:
            ChildWidth = AvailableCross;
            break;
        }

        const float ChildY = CursorY + Margin.Top;
        Child->Arrange({ChildX, ChildY, ChildWidth, Desired.y});

        CursorY = ChildY + Desired.y + Margin.Bottom;
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
    const SDL_FRect Content = ContentRectFrom(ArrangedRect);
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
        const float MaxScroll = NonNegative(ContentHeight - Content.h);
        ScrollOffsetY = Clamp(ScrollOffsetY - Input.MouseWheelDelta * WheelStepLogical, 0.0f, MaxScroll);
        Consumed = true;
    }

    // The panel is opaque: being over it consumes input regardless.
    return Consumed || OverPanel;
}

void ScrollablePanel::Draw(Render::Renderer& Renderer) {
    if (Style.ColorBackground.a != 0) {
        Renderer.DrawFilledRect(ArrangedRect, Style.ColorBackground);
    }
    if (Style.BorderWidth > 0.0f && Style.ColorBorder.a != 0) {
        Renderer.DrawRectOutline(ArrangedRect, Style.ColorBorder, Style.BorderWidth);
    }

    const SDL_FRect Content = ContentRectFrom(ArrangedRect);
    Renderer.PushClipRect(Content);
    for (auto& Child : Children) {
        Child->Draw(Renderer);
    }
    Renderer.PopClipRect();
}

} // namespace Penumbra::Widgets
