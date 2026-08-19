#pragma once

#include "Penumbra/IWidgetLifecycle.h"

#include <vector>

namespace Penumbra {

// Standalone lifecycle registration/dispatch mechanism, factored out of
// Application (docs/next_steps.md's "IWidgetLifecycle registration/ticking
// needs to work without owning a full Application" entry). RegisterLifecycle/
// UnregisterLifecycle/Tick never touched any other Application state (no
// Window/Renderer/FontBackend/Input/frame-loop member) -- this class is
// exactly that self-contained slice, unchanged in behavior, so a hand-rolled
// host that isn't itself a Penumbra::Application (e.g. pharos-proto's own
// Platform::PlatformWindow-owning src/main.cpp) can construct one directly,
// alongside its own window/renderer, and call Tick() once per frame from its
// own loop -- no need to become Application-based just to get OnTick
// dispatch.
//
// Application owns one of these as a member and forwards its own
// RegisterLifecycle/UnregisterLifecycle/Tick to it unchanged -- zero
// behavior change for existing Application-based consumers.
class LifecycleRegistry {
public:
    // Registers Lifecycle and immediately calls its OnMount(). Does not take
    // ownership -- the caller must keep Lifecycle alive until a matching
    // UnregisterLifecycle() call (or this registry's own destruction).
    void RegisterLifecycle(IWidgetLifecycle* Lifecycle);

    // Calls Lifecycle->OnUnmount() and removes it from the registry. No-op if
    // Lifecycle was never registered (or was already unregistered).
    void UnregisterLifecycle(IWidgetLifecycle* Lifecycle);

    // Fans OnTick(TickInfo{DeltaSeconds}) out to every currently-registered
    // lifecycle, in registration order.
    void Tick(float DeltaSeconds);

private:
    std::vector<IWidgetLifecycle*> RegisteredLifecycles;
};

} // namespace Penumbra
