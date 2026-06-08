#include "Penumbra/Widgets/Button.h"

namespace Penumbra::Widgets {

namespace {
bool PointInRect(SDL_FPoint Point, SDL_FRect Rect) {
    return Point.x >= Rect.x && Point.x < Rect.x + Rect.w &&
           Point.y >= Rect.y && Point.y < Rect.y + Rect.h;
}
constexpr int LeftButton = 0;
} // namespace

void Button::ApplyStyle(const ButtonStyle& InStyle) {
    Style = static_cast<const BoxStyle&>(InStyle); // box model + default background
    ColorBackgroundHovered  = InStyle.ColorBackgroundHovered;
    ColorBackgroundPressed  = InStyle.ColorBackgroundPressed;
    ColorBackgroundDisabled = InStyle.ColorBackgroundDisabled;
}

bool Button::UpdateInteractionState(const Platform::InputState& Input) {
    if (!IsEnabled) {
        CurrentState = InteractionState::Disabled;
        PressedInside = false;
        return false;
    }

    const bool Hovered = PointInRect(Input.MousePosition, ArrangedRect);
    const bool Down     = Input.MouseButtonDown[LeftButton];
    const bool Pressed  = Input.MouseButtonPressedThisFrame[LeftButton];
    const bool Released = Input.MouseButtonReleasedThisFrame[LeftButton];

    if (Pressed && Hovered) {
        PressedInside = true;
    }

    // Click fires on release, but only if the press both began and ended on the
    // button — dragging off and releasing cancels it.
    bool Clicked = false;
    if (Released) {
        Clicked = PressedInside && Hovered;
        PressedInside = false;
    }

    if (PressedInside && Down && Hovered) {
        CurrentState = InteractionState::Pressed;
    } else if (Hovered) {
        CurrentState = InteractionState::Hovered;
    } else {
        CurrentState = InteractionState::Default;
    }

    if (Clicked && OnClicked) {
        OnClicked();
    }

    return Hovered || (PressedInside && Down);
}

SDL_Color Button::BackgroundForState() const {
    switch (CurrentState) {
    case InteractionState::Hovered:  return ColorBackgroundHovered;
    case InteractionState::Pressed:  return ColorBackgroundPressed;
    case InteractionState::Disabled: return ColorBackgroundDisabled;
    case InteractionState::Default:
    default:                         return Style.ColorBackground;
    }
}

void Button::Draw(Render::Renderer& Renderer) {
    // Swap in the state background, reuse Box's drawing, then restore.
    const SDL_Color Chosen = BackgroundForState();
    const SDL_Color Saved = Style.ColorBackground;
    Style.ColorBackground = Chosen;
    Box::Draw(Renderer);
    Style.ColorBackground = Saved;
}

} // namespace Penumbra::Widgets
