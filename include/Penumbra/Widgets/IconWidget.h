#pragma once

#include "Penumbra/Backends/IIconBackend.h"
#include "Penumbra/Widgets/WidgetBase.h"

#include <memory>
#include <string>

namespace Penumbra::Widgets {

// A leaf widget that draws a single named vector glyph through an app-supplied
// IIconBackend — the Penumbra-side half of Iris's `<Icon icon="...">` (docs/
// penumbra_iris_lustre_componentization_gaps_requirements.md §1). A WidgetBase
// subclass directly, same as ImageWidget: no box model of its own. Unlike
// ImageWidget there is no separate load step — IconBackend is a plain public field
// (mirroring how Walker.cpp already sets Label::FontBackend/Font directly post-build,
// not through the Builder), and Draw() calls it fresh every frame since an icon is
// redrawn procedurally, never cached as a texture.
class IconWidget : public WidgetBase {
public:
    std::string       IconName;
    Backends::IIconBackend* IconBackend{nullptr};

    // No true intrinsic size exists for a procedurally-drawn glyph (no asset to
    // measure, unlike ImageWidget's decoded texture) — a fixed logical square,
    // overridable directly, same "reasonable default, not a real measurement"
    // treatment as everywhere else in this PoC that has no better answer.
    float SizeLogical{16.0f};

    Point Measure(Point AvailableSizeLogical) override;
    void  Arrange(Rect FinalRectLogical) override;
    bool  UpdateInteractionState(const Platform::InputState&) override;
    void  Draw(Render::Renderer&) override;

    // Fluent, chainable construction — see ImageWidget::Builder for the naming-
    // convention rationale. Deliberately narrow: just icon() (Iris <Icon>'s content
    // prop) and className() — no child()/children() (a leaf) and no onPress()/etc,
    // matching <Icon>'s own prop list in docs/iris_core_spec.md §3.1.
    class Builder {
    public:
        Builder();

        Builder& icon(std::string Value);
        Builder& className(std::string Value);

        std::unique_ptr<IconWidget> build();

    private:
        std::unique_ptr<IconWidget> Owned;
    };

private:
    bool PressedInside{false};
};

} // namespace Penumbra::Widgets
