#include "Penumbra/Render/Renderer.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

namespace Penumbra::Render {

namespace {

constexpr float Pi = 3.14159265358979323846f;

SDL_FColor ToFColor(SDL_Color Color) {
    return {Color.r / 255.0f, Color.g / 255.0f, Color.b / 255.0f, Color.a / 255.0f};
}

// More segments per corner as the radius grows, so small radii stay cheap and large
// ones stay smooth.
int SegmentsForRadius(float RadiusPhysical) {
    return std::clamp(static_cast<int>(std::ceil(RadiusPhysical / 2.0f)), 2, 24);
}

// Clockwise perimeter of a rounded rect in screen space (y down). The four corner
// arc centres are the rect's inset corners; straight edges fall out as the chords
// between adjacent corner arcs. Radius must already be clamped to <= min(w,h)/2.
std::vector<SDL_FPoint> BuildRoundedPerimeter(SDL_FRect R, float Radius, int Segments) {
    struct Corner {
        float CentreX, CentreY, AngleStart, AngleEnd;
    };
    const Corner Corners[4] = {
        {R.x + Radius,          R.y + Radius,          Pi,          Pi * 1.5f}, // top-left
        {R.x + R.w - Radius,    R.y + Radius,          Pi * 1.5f,   Pi * 2.0f}, // top-right
        {R.x + R.w - Radius,    R.y + R.h - Radius,    0.0f,        Pi * 0.5f}, // bottom-right
        {R.x + Radius,          R.y + R.h - Radius,    Pi * 0.5f,   Pi},        // bottom-left
    };

    std::vector<SDL_FPoint> Points;
    Points.reserve(static_cast<std::size_t>(Segments + 1) * 4);
    for (const Corner& C : Corners) {
        for (int Step = 0; Step <= Segments; ++Step) {
            const float T = static_cast<float>(Step) / static_cast<float>(Segments);
            const float Angle = C.AngleStart + (C.AngleEnd - C.AngleStart) * T;
            Points.push_back({C.CentreX + Radius * std::cos(Angle),
                              C.CentreY + Radius * std::sin(Angle)});
        }
    }
    return Points;
}

} // namespace

bool Renderer::Initialise(SDL_Renderer* InSdlRenderer, float InDpiScaleFactor,
                          IFontBackend* InFontBackend) {
    if (!InSdlRenderer) {
        return false;
    }
    SdlRenderer = InSdlRenderer;
    DpiScaleFactor = (InDpiScaleFactor > 0.0f) ? InDpiScaleFactor : 1.0f;
    FontBackend = InFontBackend;
    return true;
}

SDL_FRect Renderer::ToPhysical(SDL_FRect RectLogical) const {
    return {RectLogical.x * DpiScaleFactor, RectLogical.y * DpiScaleFactor,
            RectLogical.w * DpiScaleFactor, RectLogical.h * DpiScaleFactor};
}

void Renderer::BeginFrame(SDL_Color ClearColor) {
    SDL_SetRenderDrawColor(SdlRenderer, ClearColor.r, ClearColor.g, ClearColor.b, ClearColor.a);
    SDL_RenderClear(SdlRenderer);
}

void Renderer::EndFrameAndPresent() {
    assert(ClipStack.empty() && "clip stack not balanced: a PushClipRect is missing its PopClipRect");
    SDL_RenderPresent(SdlRenderer);
}

void Renderer::DrawFilledRect(SDL_FRect RectLogical, SDL_Color Color, float CornerRadiusLogical) {
    const SDL_FRect R = ToPhysical(RectLogical);
    float Radius = CornerRadiusLogical * DpiScaleFactor;

    if (Radius <= 0.0f) {
        SDL_SetRenderDrawColor(SdlRenderer, Color.r, Color.g, Color.b, Color.a);
        SDL_RenderFillRect(SdlRenderer, &R);
        return;
    }

    Radius = std::min(Radius, std::min(R.w, R.h) * 0.5f);
    const int Segments = SegmentsForRadius(Radius);
    const std::vector<SDL_FPoint> Perimeter = BuildRoundedPerimeter(R, Radius, Segments);
    const SDL_FColor FillColor = ToFColor(Color);
    const int Count = static_cast<int>(Perimeter.size());

    // Fan from the rect centre: the rounded rect is convex, so this fills cleanly.
    std::vector<SDL_Vertex> Vertices;
    Vertices.reserve(Perimeter.size() + 1);
    Vertices.push_back({{R.x + R.w * 0.5f, R.y + R.h * 0.5f}, FillColor, {0.0f, 0.0f}});
    for (const SDL_FPoint& Point : Perimeter) {
        Vertices.push_back({Point, FillColor, {0.0f, 0.0f}});
    }

    std::vector<int> Indices;
    Indices.reserve(static_cast<std::size_t>(Count) * 3);
    for (int Index = 0; Index < Count; ++Index) {
        Indices.push_back(0);
        Indices.push_back(1 + Index);
        Indices.push_back(1 + (Index + 1) % Count);
    }

    SDL_RenderGeometry(SdlRenderer, nullptr, Vertices.data(), static_cast<int>(Vertices.size()),
                       Indices.data(), static_cast<int>(Indices.size()));
}

void Renderer::DrawRectOutline(SDL_FRect RectLogical, SDL_Color Color, float ThicknessLogical,
                               float CornerRadiusLogical) {
    const SDL_FRect R = ToPhysical(RectLogical);
    const float T = ThicknessLogical * DpiScaleFactor;
    float Radius = CornerRadiusLogical * DpiScaleFactor;

    if (Radius <= 0.0f) {
        SDL_SetRenderDrawColor(SdlRenderer, Color.r, Color.g, Color.b, Color.a);
        const SDL_FRect Edges[4] = {
            {R.x,           R.y,           R.w, T  }, // top
            {R.x,           R.y + R.h - T, R.w, T  }, // bottom
            {R.x,           R.y,           T,   R.h}, // left
            {R.x + R.w - T, R.y,           T,   R.h}, // right
        };
        for (const SDL_FRect& Edge : Edges) {
            SDL_RenderFillRect(SdlRenderer, &Edge);
        }
        return;
    }

    Radius = std::min(Radius, std::min(R.w, R.h) * 0.5f);

    // A border thicker than the box collapses to a solid rounded fill.
    const SDL_FRect Inner{R.x + T, R.y + T, R.w - 2.0f * T, R.h - 2.0f * T};
    if (Inner.w <= 0.0f || Inner.h <= 0.0f) {
        DrawFilledRect(RectLogical, Color, CornerRadiusLogical);
        return;
    }

    const int Segments = SegmentsForRadius(Radius);
    const float InnerRadius = std::max(0.0f, Radius - T);
    // Inner arc centres equal the outer ones (inset by T, radius shrinks by T), so the
    // outer and inner perimeters share angles and pair up one-to-one into a ring.
    const std::vector<SDL_FPoint> Outer = BuildRoundedPerimeter(R, Radius, Segments);
    const std::vector<SDL_FPoint> InnerRing = BuildRoundedPerimeter(Inner, InnerRadius, Segments);
    const SDL_FColor LineColor = ToFColor(Color);
    const int Count = static_cast<int>(Outer.size());

    std::vector<SDL_Vertex> Vertices;
    Vertices.reserve(Outer.size() + InnerRing.size());
    for (const SDL_FPoint& Point : Outer) {
        Vertices.push_back({Point, LineColor, {0.0f, 0.0f}});
    }
    for (const SDL_FPoint& Point : InnerRing) {
        Vertices.push_back({Point, LineColor, {0.0f, 0.0f}});
    }

    std::vector<int> Indices;
    Indices.reserve(static_cast<std::size_t>(Count) * 6);
    for (int Index = 0; Index < Count; ++Index) {
        const int OuterA = Index;
        const int OuterB = (Index + 1) % Count;
        const int InnerA = Count + Index;
        const int InnerB = Count + (Index + 1) % Count;
        Indices.push_back(OuterA);
        Indices.push_back(OuterB);
        Indices.push_back(InnerB);
        Indices.push_back(OuterA);
        Indices.push_back(InnerB);
        Indices.push_back(InnerA);
    }

    SDL_RenderGeometry(SdlRenderer, nullptr, Vertices.data(), static_cast<int>(Vertices.size()),
                       Indices.data(), static_cast<int>(Indices.size()));
}

void Renderer::DrawText(FontHandle Font, std::string_view Text, SDL_FPoint PositionLogical,
                        SDL_Color Color) {
    if (!FontBackend || Text.empty()) {
        return;
    }

    SDL_Texture* Texture = FontBackend->AcquireTextTexture(SdlRenderer, Font, Text, Color);
    if (!Texture) {
        return;
    }

    // Glyph texture is already at physical pixel size; only the position scales.
    float TextureWidth = 0.0f;
    float TextureHeight = 0.0f;
    SDL_GetTextureSize(Texture, &TextureWidth, &TextureHeight);

    const SDL_FRect Destination{PositionLogical.x * DpiScaleFactor,
                                PositionLogical.y * DpiScaleFactor, TextureWidth, TextureHeight};
    SDL_RenderTexture(SdlRenderer, Texture, nullptr, &Destination);
}

void Renderer::PushClipRect(SDL_FRect RectLogical) {
    const SDL_FRect P = ToPhysical(RectLogical);
    SDL_Rect Requested{static_cast<int>(std::lround(P.x)), static_cast<int>(std::lround(P.y)),
                       static_cast<int>(std::lround(P.w)), static_cast<int>(std::lround(P.h))};

    SDL_Rect Effective = Requested;
    if (!ClipStack.empty()) {
        // Intersect with the active clip so nested clips never exceed their parent.
        if (!SDL_GetRectIntersection(&ClipStack.back(), &Requested, &Effective)) {
            Effective = {0, 0, 0, 0}; // disjoint → clip everything
        }
    }

    ClipStack.push_back(Effective);
    SDL_SetRenderClipRect(SdlRenderer, &Effective);
}

void Renderer::PopClipRect() {
    assert(!ClipStack.empty() && "PopClipRect called with an empty clip stack");
    ClipStack.pop_back();
    if (ClipStack.empty()) {
        SDL_SetRenderClipRect(SdlRenderer, nullptr);
    } else {
        SDL_SetRenderClipRect(SdlRenderer, &ClipStack.back());
    }
}

TextMetrics Renderer::MeasureText(FontHandle Font, std::string_view Text) const {
    if (!FontBackend) {
        return {0.0f, 0.0f, 0.0f};
    }
    return FontBackend->MeasureText(Font, Text);
}

float Renderer::MeasureTextWidth(FontHandle Font, std::string_view Text) const {
    if (!FontBackend) {
        return 0.0f;
    }
    return FontBackend->MeasureTextWidth(Font, Text);
}

float Renderer::GetDpiScaleFactor() const {
    return DpiScaleFactor;
}

} // namespace Penumbra::Render
