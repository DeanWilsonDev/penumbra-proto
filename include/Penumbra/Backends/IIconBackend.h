#pragma once

#include "Penumbra/Geometry.h"

#include <string_view>

namespace Penumbra::Render {
class Renderer;
} // namespace Penumbra::Render

namespace Penumbra::Backends {

// Abstraction over an app-owned vector-icon catalog (e.g. Pharos's `icons::IconKind` /
// `icons::drawIcon`), mirroring IImageBackend's role for <Image>: widget code asks for
// an icon by name, never draws one directly, so Penumbra never needs to know the
// concrete catalog. Unlike IImageBackend there is no load/release step — an icon is
// drawn procedurally (Renderer::DrawLine/DrawTriangleFilled — already exactly what
// Renderer.h's own DrawTriangleFilled doc comment calls out as "enough ... to build a
// chevron ... or a filled caret/arrow") fresh every frame, not decoded into a cached
// texture, so there is nothing to release.
class IIconBackend {
public:
    virtual ~IIconBackend() = default;

    // Draws IconName into BoundsLogical via Renderer. A no-op (not an error) if
    // IconName isn't in the backend's catalog — callers can't validate names ahead of
    // time since the catalog is entirely backend-owned.
    virtual void DrawIcon(Render::Renderer& Renderer, std::string_view IconName, Rect BoundsLogical) = 0;
};

} // namespace Penumbra::Backends
