#pragma once

#include "Penumbra/Platform/InputState.h"
#include "Penumbra/Render/Renderer.h"
#include "Penumbra/Widgets/Styles.h"

#include <SDL3/SDL.h>

namespace Penumbra::Widgets {

// Four states only — this is complete. If you think you need a fifth, you need a
// different widget.
enum class InteractionState { Default, Hovered, Pressed, Disabled };

class WidgetBase {
public:
    virtual ~WidgetBase() = default;

    // Two-phase layout: Measure reports desired size bottom-up; Arrange commits the
    // final rect top-down.
    virtual SDL_FPoint Measure(SDL_FPoint AvailableSizeLogical) = 0;
    virtual void       Arrange(SDL_FRect FinalRectLogical) = 0;

    // Returns true if this widget consumed the input (stops propagation).
    virtual bool UpdateInteractionState(const Platform::InputState&) = 0;

    virtual void Draw(Render::Renderer&) = 0;

    // A child's margin is consumed by its parent during Arrange. Margin lives on
    // BoxStyle, but the parent reaches it polymorphically because every widget is a
    // Box; non-Box widgets (there are none in this PoC) report zero.
    virtual EdgeInsets GetMarginLogical() const { return {0.0f, 0.0f, 0.0f, 0.0f}; }

    void      SetIsEnabled(bool Enabled) { IsEnabled = Enabled; }
    bool      GetIsEnabled() const { return IsEnabled; }
    SDL_FRect GetArrangedRect() const { return ArrangedRect; }

protected:
    bool      IsEnabled{true};
    SDL_FRect ArrangedRect{0.0f, 0.0f, 0.0f, 0.0f};
};

} // namespace Penumbra::Widgets
