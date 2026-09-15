#pragma once

#include "Penumbra/Geometry.h"
#include "Penumbra/Platform/InputState.h"
#include "Penumbra/Render/Renderer.h"
#include "Penumbra/Widgets/Styles.h"

#include <cstddef>
#include <functional>
#include <string>

namespace Penumbra::Widgets {

// Four states only — this is complete. If you think you need a fifth, you need a
// different widget.
enum class InteractionState { Default, Hovered, Pressed, Disabled };

class WidgetBase {
public:
    virtual ~WidgetBase() {
        if (OnDestroyed) {
            OnDestroyed();
        }
    }

    // Two-phase layout: Measure reports desired size bottom-up; Arrange commits the
    // final rect top-down.
    virtual Point Measure(Point AvailableSizeLogical) = 0;
    virtual void  Arrange(Rect FinalRectLogical) = 0;

    // Returns true if this widget consumed the input (stops propagation).
    virtual bool UpdateInteractionState(const Platform::InputState&) = 0;

    // Whether this widget itself used a mouse-wheel scroll on ITSELF this frame --
    // distinct from UpdateInteractionState's general "consumed" return, which also
    // covers plain hover/click and would otherwise make an ancestor ScrollablePanel
    // think a nested wheel-scrollable region (another ScrollablePanel, or a TextArea)
    // already claimed the wheel even when that region had no room left to scroll (and
    // so should let the wheel fall through to the ancestor instead of getting stuck).
    // False by default; only a widget with its own WheelStepLogical-style field needs
    // to override it.
    virtual bool ConsumedWheelThisFrame() const { return false; }

    virtual void Draw(Render::Renderer&) = 0;

    // A child's margin is consumed by its parent during Arrange. Margin lives on
    // BoxStyle, but the parent reaches it polymorphically because every widget is a
    // Box; non-Box widgets (there are none in this PoC) report zero.
    virtual EdgeInsets GetMarginLogical() const { return {0.0f, 0.0f, 0.0f, 0.0f}; }

    // A read-only, uniform way to walk a widget's children regardless of how it
    // stores them internally (a generic vector, named slots, ...). Widgets with no
    // children report zero — the default — rather than every leaf overriding it.
    virtual std::size_t GetChildCount() const { return 0; }
    virtual WidgetBase* GetChildAt(std::size_t Index) const { return nullptr; }

    // Generic pointer/focus/value callbacks any widget can opt into, including a
    // plain Box rather than only dedicated interactive subclasses. All
    // null by default: a widget with none set is exactly as inert as before this
    // existed, and dispatch skips hit-testing entirely rather than paying for it
    // (see Box::UpdateInteractionState).
    std::function<void()> OnPressed  = nullptr;
    std::function<void()> OnReleased = nullptr;
    std::function<void()> OnHovered  = nullptr;
    std::function<void()> OnFocused  = nullptr;
    std::function<void()> OnChanged  = nullptr;

    // Fires from ~WidgetBase() when set -- the generic "this widget is being torn
    // down" hook a plain Box handed out via BuildWidgetTree can opt into, mirroring
    // OnPressed/etc.'s "null by default, opt-in" convention. Lets a caller detect
    // host-driven removal (e.g. OverlayHost erasing this widget from its stack)
    // without needing a hand-rolled subclass whose sole purpose is running cleanup
    // in its own destructor.
    std::function<void()> OnDestroyed = nullptr;

    // Inert class-name storage. Penumbra holds the string and does nothing with
    // it, consistent with "no defaults, no opinions"; interpretation belongs to
    // the consumer.
    std::string ClassName;

    void SetIsEnabled(bool Enabled) { IsEnabled = Enabled; }
    bool GetIsEnabled() const { return IsEnabled; }
    Rect GetArrangedRect() const { return ArrangedRect; }

    // A widget that is not visible contributes zero size, is not arranged, does not
    // participate in hit-testing, and is not drawn — the same way display:none does.
    void SetIsVisible(bool Visible) { IsVisible = Visible; }
    bool GetIsVisible() const { return IsVisible; }

protected:
    bool IsEnabled{true};
    bool IsVisible{true};
    Rect ArrangedRect{0.0f, 0.0f, 0.0f, 0.0f};
};

} // namespace Penumbra::Widgets
