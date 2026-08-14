#pragma once

#include "Penumbra/Widgets/Box.h"

#include <host/nyx-runtime.hpp>
#include <interpreter/interpreter.hpp>

#include <string>

// Prototype (pharos-proto's docs/next_steps.md "Phase 2 (2b)" investigation: whether a
// genuinely custom-drawn widget's DrawContent/MeasureContent logic can live in Nyx script
// instead of a bespoke hand-written C++ Box subclass, the way TreeRow/ViewportWidget/
// ChevronSeparator are built today). ScriptCanvas is the Penumbra equivalent of an HTML5
// <canvas> element: ONE generic, hand-written C++ Box, reused for every Nyx-scripted
// custom-drawn widget, instead of one bespoke Box subclass per widget. C++ still owns the
// rasterization primitives (Renderer::DrawLine etc., exposed to Nyx via RegisterRendererType
// below, the same role a browser's native Canvas2D context plays); Nyx owns each widget's own
// drawing/measurement *decisions*, invoked once per Draw/Measure pass via
// Interpreter::CallFunction -- the same split as JS driving a <canvas> context.
//
// This does not need (or get) any new Iris capability: <Native> already splices any
// already-built WidgetBase into a tree by position, generically -- a ScriptCanvas instance
// would be registered as a <Native> builder exactly the way TreeRow/ChevronSeparator are
// today. This header only adds the C++ Box subclass and the Renderer Nyx binding; nothing
// about Iris changes.
//
// Only wired in behind PENUMBRA_WITH_NYX, same as ApplicationBridge.h -- depends on nyx-core
// headers, which the rest of Penumbra has zero dependency on.
namespace Penumbra::Nyx {

// Registers Penumbra::Render::Renderer as a Nyx-visible host type named "Renderer" on
// Runtime, exposing just enough of its drawing surface to prove the mechanism: DrawLine, the
// two-call primitive pharos-proto's own drawChevron (src/ui/text_utils.cpp) already builds a
// chevron from. Takes raw floats/ints rather than Point/Color structs, deliberately --
// nyx-proto's marshalling only auto-converts primitives today (see ApplicationBridge.h's own
// "non-primitive reference it can't wrap yet" note); Point/Color would need their own
// registered host types first, out of scope for this prototype. Returns the registered
// TypeDescriptor so a caller can wrap a live Renderer& into a Value
// (nyx::host::ToValue(&renderer, descriptor)) to pass into a Nyx function call -- the same
// pattern pharos-proto's explorer_panel.cpp already uses to wrap a TreeNode*.
const ::nyx::runtime::TypeDescriptor* RegisterRendererType(::nyx::host::NyxRuntime& Runtime);

// A <Native>-spliceable Box whose own DrawContent/MeasureContent call into Nyx instead of
// being hand-written C++. Interp/RendererDescriptor are not owned by this widget -- the
// caller's Interpreter (e.g. pharos-proto's own loadNyxScript helper) and
// RegisterRendererType's return value must both outlive it, the same lifetime shape every
// other host/Interpreter pairing in this codebase already has (see pharos-proto's
// json_path_field.h doc comment on why the interpreter must outlive the widget reading it).
//
// OnDrawFunctionName is invoked every DrawContent as
// Interpreter::CallFunction(OnDrawFunctionName, {wrappedRenderer, x, y, w, h}) --
// MeasureWidthFunctionName/MeasureHeightFunctionName (each a zero-arg Nyx function returning
// a float) stand in for MeasureContent, called once per Measure pass. Any left empty falls
// back to Box's own MeasureContent/DrawContent no-op defaults (zero size / nothing drawn).
class ScriptCanvas : public Widgets::Box {
public:
    ::nyx::interpreter::Interpreter*      Interp = nullptr;
    const ::nyx::runtime::TypeDescriptor* RendererDescriptor = nullptr;
    std::string                           OnDrawFunctionName;
    std::string                           MeasureWidthFunctionName;
    std::string                           MeasureHeightFunctionName;

protected:
    Point MeasureContent(Point AvailableContentSize) override;
    void  DrawContent(Render::Renderer& Renderer, Rect ContentRect) override;
};

} // namespace Penumbra::Nyx
