#pragma once

#include "Penumbra/Application.h"

#include <cstdio>

// Mirrors the "engine owns main(), the app supplies one factory function" shape
// used by e.g. Hazel-style engines (see snake/src/engine/entry-point.hpp for a
// worked example this repo was modelled after): an app's own main.cpp includes
// this header and defines CreateApplication(), and nothing else -- Penumbra owns
// main() itself, same as Application::Run() now owns the frame loop
// (docs/next_steps.md's "Application base class" entry).
//
// This works completely unchanged whether CreateApplication() returns a plain
// C++ Application subclass, or a Nyx-bridged one built via
// Penumbra::Nyx::LoadApplication/LoadApplicationFromFile
// (Penumbra/Nyx/ApplicationBridge.h, PENUMBRA_WITH_NYX only) -- Run() dispatches
// through Application's own virtual hooks either way, so this entry point has no
// Nyx-specific case to add.
extern Penumbra::Application* CreateApplication();

int main() {
    Penumbra::Application* App = CreateApplication();
    if (!App) {
        std::fprintf(stderr, "CreateApplication() returned nullptr\n");
        return 1;
    }
    const int Result = App->Run();
    delete App;
    return Result;
}
