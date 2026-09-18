#pragma once

#include "Penumbra/Render/IFontBackend.h"
#include "Penumbra/Render/TextWrap.h"
#include "Penumbra/Widgets/Box.h"

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Penumbra::Widgets {

// Intrinsic text content; a leaf. The font backend is injected (the Measure pass
// has no Renderer, yet text size must be known there) and identifies the font the
// Renderer was initialised with.
class Label : public Box {
public:
    Render::IFontBackend* FontBackend{nullptr};
    Render::FontHandle    Font{0};
    std::string           Text;
    Render::Color         ColorText{0, 0, 0, 0};

    // Optional width constraint and overflow behavior. Unset MaxWidthLogical
    // means "no constraint," the
    // current unbounded-intrinsic-size behavior. Set with
    // TruncateWithEllipsis false means "clip, no dots" (a Renderer clip
    // rect); true means "truncate and append .." -- both plain fields
    // rather than Builder methods, same "set directly post-build" pattern
    // FontBackend/Font above already use.
    std::optional<float> MaxWidthLogical;
    bool                  TruncateWithEllipsis{false};

    // Wrap -- when true, Text reflows across as many lines as it takes to fit
    // whatever content width MeasureContent/DrawContent are actually given (the
    // same greedy word-wrap-with-character-fallback TextArea already implements,
    // Render::WrapText, factored out so both widgets share one algorithm), rather
    // than the single-line MaxWidthLogical/TruncateWithEllipsis clip-or-ellipsis
    // behavior above. For static multi-line text that was never meant to be
    // editable (TextArea's own reason for existing) -- no caret, selection, or
    // scroll state, just reflow. Mutually exclusive with MaxWidthLogical/
    // TruncateWithEllipsis in practice (they're two different overflow
    // strategies); nothing enforces that here, so don't set both.
    bool                  Wrap{false};

    // Fluent, chainable construction; adds text() to the shared Box builder set.
    class Builder {
    public:
        Builder();

        Builder& className(std::string Value);
        Builder& text(std::string Value);
        Builder& child(std::unique_ptr<WidgetBase> Child);
        Builder& children(std::vector<std::unique_ptr<WidgetBase>> Kids);
        Builder& onPress(std::function<void()> Handler);
        Builder& onRelease(std::function<void()> Handler);
        Builder& onHover(std::function<void()> Handler);
        Builder& onFocus(std::function<void()> Handler);
        Builder& onChange(std::function<void()> Handler);

        std::unique_ptr<Label> build();

    private:
        std::unique_ptr<Label> Owned;
    };

protected:
    Point MeasureContent(Point AvailableContentSize) override;
    void  DrawContent(Render::Renderer&, Rect ContentRect) override;

private:
    float LineHeight() const; // same "Ag" ascent/descent probe TextArea::LineHeight uses
};

} // namespace Penumbra::Widgets
