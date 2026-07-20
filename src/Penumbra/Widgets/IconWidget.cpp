#include "Penumbra/Widgets/IconWidget.h"

#include <utility>

namespace Penumbra::Widgets {

namespace {
bool PointInRect(Point Point, Rect Rect) {
    return Point.X >= Rect.X && Point.X < Rect.X + Rect.W &&
           Point.Y >= Rect.Y && Point.Y < Rect.Y + Rect.H;
}
constexpr int LeftButton = 0;
} // namespace

Point IconWidget::Measure(Point /*AvailableSizeLogical*/) { return {SizeLogical, SizeLogical}; }

void IconWidget::Arrange(Rect FinalRectLogical) { ArrangedRect = FinalRectLogical; }

bool IconWidget::UpdateInteractionState(const Platform::InputState& Input) {
    // A leaf with no children to give first refusal to. Same inert-unless-opted-in
    // guard as ImageWidget::UpdateInteractionState.
    if (!IsEnabled || !(OnPressed || OnReleased || OnHovered || OnFocused || OnChanged)) {
        PressedInside = false;
        return false;
    }

    const bool Hovered  = PointInRect(Input.MousePosition, ArrangedRect);
    const bool Pressed  = Input.MouseButtonPressedThisFrame[LeftButton];
    const bool Down     = Input.MouseButtonDown[LeftButton];
    const bool Released = Input.MouseButtonReleasedThisFrame[LeftButton];

    if (Hovered && OnHovered) {
        OnHovered();
    }
    if (Pressed && Hovered) {
        PressedInside = true;
        if (OnPressed) {
            OnPressed();
        }
    }
    if (Released) {
        if (PressedInside && OnReleased) {
            OnReleased();
        }
        PressedInside = false;
    }

    return Hovered || (PressedInside && Down);
}

void IconWidget::Draw(Render::Renderer& Renderer) {
    if (IconBackend && !IconName.empty()) {
        IconBackend->DrawIcon(Renderer, IconName, ArrangedRect);
    }
}

IconWidget::Builder::Builder() : Owned(std::make_unique<IconWidget>()) {}

IconWidget::Builder& IconWidget::Builder::icon(std::string Value) {
    Owned->IconName = std::move(Value);
    return *this;
}

IconWidget::Builder& IconWidget::Builder::className(std::string Value) {
    Owned->ClassName = std::move(Value);
    return *this;
}

std::unique_ptr<IconWidget> IconWidget::Builder::build() { return std::move(Owned); }

} // namespace Penumbra::Widgets
