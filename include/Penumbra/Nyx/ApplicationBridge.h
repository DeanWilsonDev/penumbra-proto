#pragma once

#include "Penumbra/Application.h"

#include "host/nyx-runtime.hpp"

#include <filesystem>
#include <string>

// Only compiled when this repo's own CMakeLists.txt has PENUMBRA_WITH_NYX
// enabled -- the rest of Penumbra has zero dependency on nyx-proto, and this
// pulls in a whole second interpreter plus its own C++26-only build. See
// docs/next_steps.md's "Application base class" entry for why this stays an
// opt-in module rather than folded into the main `penumbra` library.
namespace Penumbra::Nyx {

// Loads Source as a .nyx script and instantiates ApplicationClassName (a class
// declared `class X : Application { ... }` inside it) as a real
// Penumbra::Application* -- nyx-proto's RegisterInheritableType/NyxBridge
// mechanism (nyx-proto/docs/nyx-scripting-language/decision-log.md §6.4/§6.5),
// wired once here rather than per-app. The returned Application dispatches
// OnStart/OnUpdate/OnShutdown/OnDpiScaleChanged virtually into whichever of
// those the Nyx class overrides, falling back to Application's real C++
// implementation for anything it doesn't (nyx-proto's own "Invoke returns
// nullopt when no Nyx override exists" contract) -- these four are exactly the
// hooks nyx-proto's marshalling supports today (primitives/void only, see
// host/marshal.hpp); Configure(ApplicationConfig&) and
// OnRender(Render::Renderer&) both take a non-primitive reference it can't
// wrap yet, so a Nyx class extending Application cannot override either of
// those two -- Application's own C++ defaults for them always run.
//
// Suitable to return directly from an app's CreateApplication()
// (Penumbra/EntryPoint.h): the interpreter backing the returned pointer is
// kept alive internally (a static registry, scoped to the whole process --
// see the .cpp file), so `delete app` in EntryPoint.h's main() is all the
// caller needs to do. Returns nullptr if Source fails to parse/interpret, or
// ApplicationClassName doesn't name a class extending "Application".
//
// A mounted Nyx subclass can call Application's non-virtual host methods
// directly. The bridge exposes RequestQuit, GetWindowLogicalSize (returning a
// PenumbraPoint handle with X()/Y()), GetDpiScaleFactor, GetFontBackend,
// SetTextInputActive, SetRootWidget, GetRootWidget,
// GetRootWidgetConsumedInputThisFrame, and GetLifecycleRegistry. Font backend,
// lifecycle registry, and widget results are opaque host handles intended to be
// passed to other functions/types registered on the same NyxRuntime.
// SetRootWidget takes ownership of a PenumbraWidget handle; the producer must
// have released its own ownership before returning that handle to Nyx, and the
// script must not use the consumed handle afterward. Passing null unmounts the
// current root.
Application* LoadApplication(const std::string& Source, const std::string& Filename,
                             const std::string& ApplicationClassName);

// LoadApplication, reading Source from a file on disk. Returns nullptr (without
// calling into nyx-proto at all) if Path can't be opened.
Application* LoadApplicationFromFile(const std::filesystem::path& Path,
                                     const std::string& ApplicationClassName);

// Same as LoadApplication above, but mounts against ExternalRuntime -- a
// nyx::host::NyxRuntime the caller already owns -- instead of this bridge's own
// process-lifetime runtime (GetRuntime() below). Lets one caller-owned runtime
// back both a Nyx-authored Application subclass and something else built against
// the same runtime (e.g. an iris-proto IrisNyxDriver constructed with its own
// external-runtime constructor -- see iris-proto's docs archive
// "iris_nyx_runtime_injection_gap_resolved.md", the precedent this mirrors), so
// registrations on one side are directly callable from the other with no ad hoc
// relay interpreter in between.
//
// The Application inheritable type is registered on ExternalRuntime itself
// (once per distinct runtime, tracked internally) the first time this overload
// (or the ExternalRuntime LoadApplicationFromFile below) is called with it --
// this never touches GetRuntime()'s own runtime or its registration state, and
// vice versa: the self-owned and caller-owned paths are fully isolated from
// each other. Register any host callbacks the script needs
// (ExternalRuntime.RegisterFunction/RegisterType/...) before calling this --
// same MountBridged-snapshots-at-mount-time timing rule GetRuntime()'s doc
// comment states below.
//
// The caller owns ExternalRuntime and must keep it alive for at least as long
// as the returned Application* and the retained interpreter bridge state
// backing it -- unlike the no-runtime overload above, nothing here extends
// ExternalRuntime's own lifetime the way the internal process-lifetime
// BridgeHost does for its own runtime.
Application* LoadApplication(const std::string& Source, const std::string& Filename,
                             const std::string& ApplicationClassName,
                             ::nyx::host::NyxRuntime& ExternalRuntime);

// LoadApplicationFromFile, mounting against ExternalRuntime -- see the
// ExternalRuntime overload of LoadApplication above for the full ownership and
// isolation contract, which applies identically here.
Application* LoadApplicationFromFile(const std::filesystem::path& Path,
                                     const std::string& ApplicationClassName,
                                     ::nyx::host::NyxRuntime& ExternalRuntime);

// The same process-lifetime NyxRuntime LoadApplication/LoadApplicationFromFile
// use internally -- exposed so a caller can RegisterFunction/RegisterType its
// own host callbacks (e.g. a "Log" a Nyx OnStart/OnUpdate override can call)
// before loading a script. Must be used, if at all, before the first
// LoadApplication call that needs the registration visible: NyxRuntime::
// MountBridged snapshots registered globals into the interpreter at mount
// time, not lazily.
::nyx::host::NyxRuntime& GetRuntime();

} // namespace Penumbra::Nyx
