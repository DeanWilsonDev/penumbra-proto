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
Application* LoadApplication(const std::string& Source, const std::string& Filename,
                             const std::string& ApplicationClassName);

// LoadApplication, reading Source from a file on disk. Returns nullptr (without
// calling into nyx-proto at all) if Path can't be opened.
Application* LoadApplicationFromFile(const std::filesystem::path& Path,
                                     const std::string& ApplicationClassName);

// The same process-lifetime NyxRuntime LoadApplication/LoadApplicationFromFile
// use internally -- exposed so a caller can RegisterFunction/RegisterType its
// own host callbacks (e.g. a "Log" a Nyx OnStart/OnUpdate override can call)
// before loading a script. Must be used, if at all, before the first
// LoadApplication call that needs the registration visible: NyxRuntime::
// MountBridged snapshots registered globals into the interpreter at mount
// time, not lazily.
::nyx::host::NyxRuntime& GetRuntime();

} // namespace Penumbra::Nyx
