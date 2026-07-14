#pragma once

#include <SDL3/SDL.h>

#include <string_view>

namespace Penumbra::Backends {

// Abstraction over the concrete image-decoding library (SDL_image today), mirroring
// Render::IFontBackend's shape and purpose: widget code loads/releases textures
// through this interface, not directly against SDL_image, so the concrete loader
// can be swapped without touching widget code.
class IImageBackend {
public:
    virtual ~IImageBackend() = default;

    // Loads an image file into a texture ready to draw through Render::Renderer.
    // Returns nullptr on failure (missing file, unsupported format, decode error) —
    // callers must check before use.
    virtual SDL_Texture* LoadTexture(SDL_Renderer* Renderer, std::string_view FilePath) = 0;

    // Releases a texture previously returned by LoadTexture. Safe to call with
    // nullptr (no-op).
    virtual void ReleaseTexture(SDL_Texture* Texture) = 0;
};

} // namespace Penumbra::Backends
