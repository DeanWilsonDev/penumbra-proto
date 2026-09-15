#pragma once

namespace Penumbra {

// Per-frame timing handed to lifecycle hooks. A plain struct (not a float
// parameter) so more fields (e.g. total elapsed time) can be added later
// without changing every OnTick override.
struct TickInfo {
    float DeltaSeconds = 0.0f;
};

// Shared lifecycle contract between an external widget-owning runtime and
// Penumbra's Application frame loop. It deliberately carries no widget or
// rendering assumptions.
class IWidgetLifecycle {
public:
    virtual void OnMount() {}
    virtual void OnUnmount() {}
    virtual void OnTick(const TickInfo& Info) {}
    virtual ~IWidgetLifecycle() = default;
};

} // namespace Penumbra
