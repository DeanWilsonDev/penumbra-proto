#pragma once

namespace Penumbra {

// Per-frame timing handed to lifecycle hooks. A plain struct (not a float
// parameter) so more fields (e.g. total elapsed time) can be added later
// without changing every OnTick override.
struct TickInfo {
    float DeltaSeconds = 0.0f;
};

// Shared lifecycle contract between a widget-owning runtime (Iris, for now) and
// Penumbra's Application frame loop. Penumbra calls these hooks; it never calls
// into Iris directly, and this header names nothing Iris-specific — Iris is just
// the first implementer.
//
// Lives under Penumbra today because Penumbra is the first backend. When Umbra
// Engine needs the same contract, this should lift out into a standalone
// umbra-interfaces library unchanged — hence no Penumbra-specific assumptions
// (widget types, render state, ...) baked into the interface itself.
class IWidgetLifecycle {
public:
    virtual void OnMount() {}
    virtual void OnUnmount() {}
    virtual void OnTick(const TickInfo& Info) {}
    virtual ~IWidgetLifecycle() = default;
};

} // namespace Penumbra
