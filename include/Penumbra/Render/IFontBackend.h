#pragma once

#include <SDL3/SDL.h>

#include <cstdint>
#include <string_view>

namespace Penumbra::Render {

using FontHandle = uint32_t; // opaque handle into the font backend

struct TextMetrics {
    float WidthLogical;
    float HeightLogical;
    float AscentLogical;
};

struct FontStyle {
    bool Italic{false};
    bool Underline{false};
    bool Strikethrough{false};

    friend bool operator==(const FontStyle&, const FontStyle&) = default;
};

// Abstraction over the text rasteriser. SDL_ttf today; a FreeType + glyph-atlas
// upgrade later should touch only the implementation, not this interface, and not
// any widget. The backend rasterises at physical size and reports in LOGICAL units.
class IFontBackend {
public:
    virtual ~IFontBackend() = default;

    virtual FontHandle LoadFont(const char* Path, float PointSizeLogical, float DpiScaleFactor) = 0;

    virtual FontHandle LoadStyledFont(const char* Path, float PointSizeLogical, float DpiScaleFactor, FontStyle) {
        return LoadFont(Path, PointSizeLogical, DpiScaleFactor);
    }

    virtual TextMetrics MeasureText     (FontHandle, std::string_view) const = 0;
    virtual float       MeasureTextWidth(FontHandle, std::string_view) const = 0;

    // Produces a texture for a run of text. Ownership stays with the backend;
    // callers must not destroy the returned texture. Caching is an implementation
    // detail of the backend.
    virtual SDL_Texture* AcquireTextTexture(SDL_Renderer*, FontHandle, std::string_view, SDL_Color) = 0;
};

} // namespace Penumbra::Render
