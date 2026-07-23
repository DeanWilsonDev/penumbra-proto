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
    if (!IsEnabled) {
        CurrentState = InteractionState::Disabled;
        PressedInside = false;
        return false;
    }

    // A leaf with no children to give first refusal to. Same inert-unless-opted-in
    // guard as ImageWidget::UpdateInteractionState -- state tracking below still
    // runs regardless, so ColorForState() reflects hover/press even for an <Icon>
    // that opted into none of the callbacks (e.g. DropdownMenuRow's selection-driven
    // color swap, which reads GetInteractionState() rather than a callback).
    const bool Hovered  = PointInRect(Input.MousePosition, ArrangedRect);
    const bool Pressed  = Input.MouseButtonPressedThisFrame[LeftButton];
    const bool Down     = Input.MouseButtonDown[LeftButton];
    const bool Released = Input.MouseButtonReleasedThisFrame[LeftButton];

    if (OnPressed || OnReleased || OnHovered || OnFocused || OnChanged) {
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
    } else if (Pressed && Hovered) {
        PressedInside = true;
    } else if (Released) {
        PressedInside = false;
    }

    if (PressedInside && Down && Hovered) {
        CurrentState = InteractionState::Pressed;
    } else if (Hovered) {
        CurrentState = InteractionState::Hovered;
    } else {
        CurrentState = InteractionState::Default;
    }

    return Hovered || (PressedInside && Down);
}

Render::Color IconWidget::ColorForState() const {
    switch (CurrentState) {
    case InteractionState::Hovered:
        return ColorLogicalHovered.A != 0 ? ColorLogicalHovered : ColorLogical;
    case InteractionState::Pressed:
        return ColorLogicalPressed.A != 0 ? ColorLogicalPressed : ColorLogical;
    case InteractionState::Disabled:
        return ColorLogicalDisabled.A != 0 ? ColorLogicalDisabled : ColorLogical;
    case InteractionState::Default:
    default:
        return ColorLogical;
    }
}

void IconWidget::Draw(Render::Renderer& Renderer) {
    if (IconBackend && !IconName.empty()) {
        IconBackend->DrawIcon(Renderer, IconName, ArrangedRect, ColorForState());
    }
}

IconWidget::Builder::Builder() : Owned(std::make_unique<IconWidget>()) {}

IconWidget::Builder& IconWidget::Builder::icon(std::string Value) {
    Owned->IconName = std::move(Value);
    return *this;
}

IconWidget::Builder& IconWidget::Builder::size(float Value) {
    Owned->SizeLogical = Value;
    return *this;
}

IconWidget::Builder& IconWidget::Builder::className(std::string Value) {
    Owned->ClassName = std::move(Value);
    return *this;
}

std::unique_ptr<IconWidget> IconWidget::Builder::build() { return std::move(Owned); }

} // namespace Penumbra::Widgets
