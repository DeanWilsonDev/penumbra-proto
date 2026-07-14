#pragma once

#include "Penumbra/Backends/IImageBackend.h"

namespace Penumbra::Backends {

// SDL_image-backed IImageBackend. Loads whatever format SDL_image supports
// (PNG/JPG at minimum) via IMG_Load, then hands the renderer a texture via
// SDL_CreateTextureFromSurface — the same two-step decode-then-upload path
// SDL_image's own IMG_LoadTexture convenience function takes internally, spelled
// out here so the intermediate surface is freed immediately rather than held.
class SdlImageBackend : public IImageBackend {
public:
    SDL_Texture* LoadTexture(SDL_Renderer* Renderer, std::string_view FilePath) override;
    void         ReleaseTexture(SDL_Texture* Texture) override;
};

} // namespace Penumbra::Backends
