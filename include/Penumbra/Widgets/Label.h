#pragma once

#include "Penumbra/Render/IFontBackend.h"
#include "Penumbra/Widgets/Box.h"

#include <string>

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

protected:
    Point MeasureContent(Point AvailableContentSize) override;
    void  DrawContent(Render::Renderer&, Rect ContentRect) override;
};

} // namespace Penumbra::Widgets
