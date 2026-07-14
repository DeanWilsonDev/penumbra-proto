#pragma once

#include "Penumbra/IWidgetLifecycle.h"

#include <vector>

namespace Penumbra {

// The application host: owns the set of registered IWidgetLifecycle
// implementations and dispatches OnTick to them once per frame. This is the
// only place Penumbra talks to a widget-owning runtime (Iris, later Umbra
// Engine) — entirely through the interface, never a concrete include.
//
// Application owns lifecycle registration and OnTick dispatch only — nothing
// else. The caller (e.g. demo/main.cpp's frame loop) is still the one that
// reconciles (iris::Tick()) and runs Measure/Arrange/Draw on its own widget
// tree; Tick() below must be called before that reconciliation step so
// lifecycle hooks observe the previous frame's committed state first.
class Application {
public:
    void RegisterLifecycle(IWidgetLifecycle* Lifecycle);
    void UnregisterLifecycle(IWidgetLifecycle* Lifecycle);

    // Fires OnTick on every registered lifecycle. Call this before reconciling
    // (e.g. iris::Tick()) and before Measure/Arrange/Draw.
    void Tick(float DeltaSeconds);

private:
    std::vector<IWidgetLifecycle*> RegisteredLifecycles;
};

} // namespace Penumbra
