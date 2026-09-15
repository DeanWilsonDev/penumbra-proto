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
// CreateApplication may return any concrete Application implementation. Run()
// dispatches through Application's virtual hooks without requiring the entry
// point to know how that implementation was produced.
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
