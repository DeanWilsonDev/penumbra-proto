#include "Penumbra/Nyx/ScriptCanvas.h"

#include "Penumbra/Render/Renderer.h"

#include <host/marshal.hpp>

#include <cstdint>
#include <memory>

namespace {

// The one drawing primitive this prototype exposes to Nyx -- enough to reimplement
// pharos-proto's own drawChevron (src/ui/text_utils.cpp), which is just two DrawLine calls.
// Free-function-taking-T&-first shape (TypeBuilder<T>::Method's "bare struct" overload,
// type-builder.hpp) rather than a pointer-to-member, since Color/Point need decomposing into
// raw primitives anyway -- there's no real Renderer::DrawLine overload this could forward to
// directly without an adapter.
void DrawLineRaw(Penumbra::Render::Renderer& Renderer, float X1, float Y1, float X2, float Y2, int R, int G, int B,
                 int A, float Thickness) {
    Renderer.DrawLine({X1, Y1}, {X2, Y2},
                      Penumbra::Render::Color{static_cast<std::uint8_t>(R), static_cast<std::uint8_t>(G),
                                               static_cast<std::uint8_t>(B), static_cast<std::uint8_t>(A)},
                      Thickness);
}

} // namespace

namespace Penumbra::Nyx {

const ::nyx::runtime::TypeDescriptor* RegisterRendererType(::nyx::host::NyxRuntime& Runtime) {
    Runtime.RegisterType<Render::Renderer>("Renderer").Method("DrawLine", &DrawLineRaw);
    // Same "read the descriptor back off Globals()" pattern pharos-proto's explorer_panel.cpp
    // already uses for TreeNode -- RegisterType's own TypeBuilder commits the registration to
    // Runtime when the temporary is destroyed above; this call reads it back out.
    return std::get<std::shared_ptr<::nyx::runtime::HostObject>>(Runtime.Globals().at("Renderer").data)->descriptor;
}

Point ScriptCanvas::MeasureContent(Point /*AvailableContentSize*/) {
    if (!Interp) return {0.0f, 0.0f};
    float Width = 0.0f;
    float Height = 0.0f;
    if (!MeasureWidthFunctionName.empty()) {
        Width = ::nyx::host::FromValue<float>(Interp->CallFunction(MeasureWidthFunctionName, {}));
    }
    if (!MeasureHeightFunctionName.empty()) {
        Height = ::nyx::host::FromValue<float>(Interp->CallFunction(MeasureHeightFunctionName, {}));
    }
    return {Width, Height};
}

void ScriptCanvas::DrawContent(Render::Renderer& Renderer, Rect ContentRect) {
    if (!Interp || OnDrawFunctionName.empty() || !RendererDescriptor) return;
    Interp->CallFunction(OnDrawFunctionName, {
                                                  ::nyx::host::ToValue(&Renderer, RendererDescriptor),
                                                  ::nyx::host::ToValue(ContentRect.X),
                                                  ::nyx::host::ToValue(ContentRect.Y),
                                                  ::nyx::host::ToValue(ContentRect.W),
                                                  ::nyx::host::ToValue(ContentRect.H),
                                              });
}

} // namespace Penumbra::Nyx
