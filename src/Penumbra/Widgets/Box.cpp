#include "Penumbra/Widgets/Box.h"

#include <algorithm>
#include <utility>

namespace Penumbra::Widgets {

namespace {
float NonNegative(float Value) { return Value > 0.0f ? Value : 0.0f; }
} // namespace

WidgetBase* Box::AddChild(std::unique_ptr<WidgetBase> Child) {
    WidgetBase* Raw = Child.get();
    Children.push_back(std::move(Child));
    return Raw;
}

SDL_FPoint Box::FrameSize() const {
    const float Horizontal = Style.Padding.Left + Style.Padding.Right + 2.0f * Style.BorderWidth;
    const float Vertical   = Style.Padding.Top + Style.Padding.Bottom + 2.0f * Style.BorderWidth;
    return {Horizontal, Vertical};
}

SDL_FRect Box::ContentRectFrom(SDL_FRect OuterRect) const {
    const float B = Style.BorderWidth;
    return {OuterRect.x + B + Style.Padding.Left,
            OuterRect.y + B + Style.Padding.Top,
            NonNegative(OuterRect.w - 2.0f * B - Style.Padding.Left - Style.Padding.Right),
            NonNegative(OuterRect.h - 2.0f * B - Style.Padding.Top - Style.Padding.Bottom)};
}

SDL_FPoint Box::Measure(SDL_FPoint AvailableSizeLogical) {
    const SDL_FPoint Frame = FrameSize();
    const SDL_FPoint ContentAvailable{NonNegative(AvailableSizeLogical.x - Frame.x),
                                      NonNegative(AvailableSizeLogical.y - Frame.y)};

    SDL_FPoint ContentDesired{0.0f, 0.0f};

    if (Layout == LayoutMode::None) {
        ContentDesired = MeasureContent(ContentAvailable);
    } else {
        const bool Vertical = (Layout == LayoutMode::VerticalStack);
        float MainTotal = 0.0f;
        float CrossMax  = 0.0f;

        for (std::size_t Index = 0; Index < Children.size(); ++Index) {
            WidgetBase* Child = Children[Index].get();
            const EdgeInsets Margin = Child->GetMarginLogical();

            const SDL_FPoint ChildAvailable{NonNegative(ContentAvailable.x - Margin.Left - Margin.Right),
                                            NonNegative(ContentAvailable.y - Margin.Top - Margin.Bottom)};
            const SDL_FPoint Desired = Child->Measure(ChildAvailable);

            const float ChildMain   = Vertical ? Desired.y : Desired.x;
            const float ChildCross  = Vertical ? Desired.x : Desired.y;
            const float MarginMain  = Vertical ? (Margin.Top + Margin.Bottom) : (Margin.Left + Margin.Right);
            const float MarginCross = Vertical ? (Margin.Left + Margin.Right) : (Margin.Top + Margin.Bottom);

            if (Index > 0) {
                MainTotal += ChildGap;
            }
            MainTotal += ChildMain + MarginMain;
            CrossMax = std::max(CrossMax, ChildCross + MarginCross);
        }

        ContentDesired = Vertical ? SDL_FPoint{CrossMax, MainTotal} : SDL_FPoint{MainTotal, CrossMax};
    }

    return {ContentDesired.x + Frame.x, ContentDesired.y + Frame.y};
}

void Box::Arrange(SDL_FRect FinalRectLogical) {
    ArrangedRect = FinalRectLogical;

    if (Layout == LayoutMode::None) {
        return; // children, if any, are not laid out (footgun accepted)
    }

    const SDL_FRect Content = ContentRectFrom(FinalRectLogical);
    const bool Vertical = (Layout == LayoutMode::VerticalStack);

    // Places a child along the CROSS axis, returning its {position, extent}.
    auto PlaceCross = [&](float ContentStart, float ContentExtent, float ChildExtent,
                          float MarginLead, float MarginTrail) -> std::pair<float, float> {
        const float Available = NonNegative(ContentExtent - MarginLead - MarginTrail);
        switch (CrossAlignment) {
        case CrossAlign::Start:
            return {ContentStart + MarginLead, ChildExtent};
        case CrossAlign::Center:
            return {ContentStart + MarginLead + (Available - ChildExtent) / 2.0f, ChildExtent};
        case CrossAlign::End:
            return {ContentStart + ContentExtent - MarginTrail - ChildExtent, ChildExtent};
        case CrossAlign::Stretch:
            return {ContentStart + MarginLead, Available};
        }
        return {ContentStart + MarginLead, ChildExtent};
    };

    const std::size_t Count = Children.size();
    float Cursor = Vertical ? Content.y : Content.x;

    for (std::size_t Index = 0; Index < Count; ++Index) {
        WidgetBase* Child = Children[Index].get();
        const EdgeInsets Margin = Child->GetMarginLogical();

        const SDL_FPoint ChildAvailable{NonNegative(Content.w - Margin.Left - Margin.Right),
                                        NonNegative(Content.h - Margin.Top - Margin.Bottom)};
        const SDL_FPoint Desired = Child->Measure(ChildAvailable);

        if (Vertical) {
            auto [CrossPos, CrossExtent] =
                PlaceCross(Content.x, Content.w, Desired.x, Margin.Left, Margin.Right);
            const float Top = Cursor + Margin.Top;
            Child->Arrange({CrossPos, Top, CrossExtent, Desired.y});
            Cursor = Top + Desired.y + Margin.Bottom;
        } else {
            auto [CrossPos, CrossExtent] =
                PlaceCross(Content.y, Content.h, Desired.y, Margin.Top, Margin.Bottom);
            const float Left = Cursor + Margin.Left;
            Child->Arrange({Left, CrossPos, Desired.x, CrossExtent});
            Cursor = Left + Desired.x + Margin.Right;
        }

        if (Index + 1 < Count) {
            Cursor += ChildGap;
        }
    }
}

bool Box::UpdateInteractionState(const Platform::InputState& Input) {
    // Depth-first, last child (drawn on top) gets first refusal.
    for (auto Iterator = Children.rbegin(); Iterator != Children.rend(); ++Iterator) {
        if ((*Iterator)->UpdateInteractionState(Input)) {
            return true;
        }
    }
    return false;
}

void Box::Draw(Render::Renderer& Renderer) {
    if (Style.ColorBackground.a != 0) {
        Renderer.DrawFilledRect(ArrangedRect, Style.ColorBackground);
    }
    if (Style.BorderWidth > 0.0f && Style.ColorBorder.a != 0) {
        Renderer.DrawRectOutline(ArrangedRect, Style.ColorBorder, Style.BorderWidth);
    }

    DrawContent(Renderer, ContentRectFrom(ArrangedRect));

    for (auto& Child : Children) {
        Child->Draw(Renderer);
    }
}

} // namespace Penumbra::Widgets
