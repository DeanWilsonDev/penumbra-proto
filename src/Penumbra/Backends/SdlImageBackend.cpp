#include "Penumbra/Backends/SdlImageBackend.h"

#include <SDL3_image/SDL_image.h>

#include <string>

namespace Penumbra::Backends {

SDL_Texture* SdlImageBackend::LoadTexture(SDL_Renderer* Renderer, std::string_view FilePath) {
    SDL_Surface* Surface = IMG_Load(std::string(FilePath).c_str());
    if (!Surface) {
        return nullptr;
    }
    SDL_Texture* Texture = SDL_CreateTextureFromSurface(Renderer, Surface);
    SDL_DestroySurface(Surface);
    return Texture;
}

void SdlImageBackend::ReleaseTexture(SDL_Texture* Texture) {
    if (Texture) {
        SDL_DestroyTexture(Texture);
    }
}

} // namespace Penumbra::Backends
