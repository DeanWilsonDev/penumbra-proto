#pragma once

#include "Penumbra/Backends/IIconBackend.h"
#include "Penumbra/Render/Color.h"

#include <span>
#include <string_view>

namespace PenumbraIcons {

class StandardIconBackend : public Penumbra::Backends::IIconBackend {
public:
    StandardIconBackend(float StrokeThicknessLogical, Penumbra::Render::Color DefaultColor);

    void DrawIcon(Penumbra::Render::Renderer& Renderer, std::string_view IconName, Penumbra::Rect BoundsLogical,
                  Penumbra::Render::Color IconColor) override;

    static std::span<const std::string_view> Names();

    float                   StrokeThicknessLogical;
    Penumbra::Render::Color DefaultColor;
};

} // namespace PenumbraIcons
