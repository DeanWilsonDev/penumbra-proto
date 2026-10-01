#include "Penumbra/Render/SdlTtfFontBackend.h"

#include <string>

namespace Penumbra::Render {

namespace {

uint32_t PackColor(SDL_Color Color) {
    return (static_cast<uint32_t>(Color.r) << 24) | (static_cast<uint32_t>(Color.g) << 16) |
           (static_cast<uint32_t>(Color.b) << 8)  |  static_cast<uint32_t>(Color.a);
}

std::string MakeCacheKey(FontHandle Handle, std::string_view Text, SDL_Color Color) {
    std::string Key;
    Key.reserve(Text.size() + 24);
    Key += std::to_string(Handle);
    Key += ':';
    Key += std::to_string(PackColor(Color));
    Key += ':';
    Key.append(Text.data(), Text.size());
    return Key;
}

} // namespace

SdlTtfFontBackend::SdlTtfFontBackend() {
    TtfInitialised = TTF_Init();
}

SdlTtfFontBackend::~SdlTtfFontBackend() {
    for (auto& [Key, Texture] : TextureCache) {
        SDL_DestroyTexture(Texture);
    }
    TextureCache.clear();

    for (auto& Loaded : Fonts) {
        if (Loaded.Font) {
            TTF_CloseFont(Loaded.Font);
        }
        if (Loaded.PlainFont) {
            TTF_CloseFont(Loaded.PlainFont);
        }
    }
    Fonts.clear();

    if (TtfInitialised) {
        TTF_Quit();
        TtfInitialised = false;
    }
}

FontHandle SdlTtfFontBackend::LoadFont(const char* Path, float PointSizeLogical, float DpiScaleFactor) {
    return LoadStyledFont(Path, PointSizeLogical, DpiScaleFactor, FontStyle{});
}

FontHandle SdlTtfFontBackend::LoadStyledFont(const char* Path, float PointSizeLogical, float DpiScaleFactor,
                                             FontStyle Style) {
    const float Scale = (DpiScaleFactor > 0.0f) ? DpiScaleFactor : 1.0f;
    TTF_Font* Font = TTF_OpenFont(Path, PointSizeLogical * Scale);
    TTF_Font* PlainFont = nullptr;
    if (Font && Style != FontStyle{}) {
        PlainFont = TTF_OpenFont(Path, PointSizeLogical * Scale);
    }
    if (Font) {
        TTF_FontStyleFlags Flags = TTF_STYLE_NORMAL;
        if (Style.Italic) {
            Flags |= TTF_STYLE_ITALIC;
        }
        if (Style.Underline) {
            Flags |= TTF_STYLE_UNDERLINE;
        }
        if (Style.Strikethrough) {
            Flags |= TTF_STYLE_STRIKETHROUGH;
        }
        TTF_SetFontStyle(Font, Flags);
    }
    const auto Handle = static_cast<FontHandle>(Fonts.size());
    Fonts.push_back({Font, Scale, PlainFont});
    return Handle;
}

const SdlTtfFontBackend::LoadedFont* SdlTtfFontBackend::GetFont(FontHandle Handle) const {
    if (Handle >= Fonts.size() || Fonts[Handle].Font == nullptr) {
        return nullptr;
    }
    return &Fonts[Handle];
}

TextMetrics SdlTtfFontBackend::MeasureText(FontHandle Handle, std::string_view Text) const {
    const LoadedFont* Loaded = GetFont(Handle);
    if (!Loaded) {
        return {0.0f, 0.0f, 0.0f};
    }

    int PhysicalWidth = 0;
    int PhysicalHeight = 0;
    TTF_Font* MeasuringFont = Loaded->PlainFont ? Loaded->PlainFont : Loaded->Font;
    TTF_GetStringSize(MeasuringFont, Text.data(), Text.size(), &PhysicalWidth, &PhysicalHeight);

    const float Scale = Loaded->DpiScaleFactor;
    const float Ascent = static_cast<float>(TTF_GetFontAscent(Loaded->Font));

    return {PhysicalWidth / Scale, PhysicalHeight / Scale, Ascent / Scale};
}

float SdlTtfFontBackend::MeasureTextWidth(FontHandle Handle, std::string_view Text) const {
    return MeasureText(Handle, Text).WidthLogical;
}

SDL_Texture* SdlTtfFontBackend::AcquireTextTexture(SDL_Renderer* SdlRenderer, FontHandle Handle,
                                                   std::string_view Text, SDL_Color Color) {
    const LoadedFont* Loaded = GetFont(Handle);
    if (!Loaded || Text.empty()) {
        return nullptr;
    }

    const std::string Key = MakeCacheKey(Handle, Text, Color);
    if (auto Found = TextureCache.find(Key); Found != TextureCache.end()) {
        return Found->second;
    }

    SDL_Surface* Surface = TTF_RenderText_Blended(Loaded->Font, Text.data(), Text.size(), Color);
    if (!Surface) {
        return nullptr;
    }

    SDL_Texture* Texture = SDL_CreateTextureFromSurface(SdlRenderer, Surface);
    SDL_DestroySurface(Surface);
    if (!Texture) {
        return nullptr;
    }

    TextureCache.emplace(Key, Texture);
    return Texture;
}

} // namespace Penumbra::Render
