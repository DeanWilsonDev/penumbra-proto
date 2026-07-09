#pragma once

#include "Penumbra/Widgets/Box.h"

#include <functional>

namespace Penumbra::Widgets {

// A boolean toggle with an intrinsic check glyph. The box itself is the glyph: its
// size is GlyphSizeLogical (a value the demo supplies) plus the box-model frame.
class Checkbox : public Box {
public:
    Render::Color ColorCheckMark{0, 0, 0, 0};
    Render::Color ColorBoxChecked{0, 0, 0, 0};
    float         GlyphSizeLogical{0.0f};
    bool          Checked{false};

    std::function<void(bool)> OnChanged;

    void ApplyStyle(const CheckboxStyle& Style);

    bool UpdateInteractionState(const Platform::InputState&) override;

protected:
    Point MeasureContent(Point AvailableContentSize) override;
    void  DrawContent(Render::Renderer&, Rect ContentRect) override;

private:
    bool PressedInside{false};
};

} // namespace Penumbra::Widgets
