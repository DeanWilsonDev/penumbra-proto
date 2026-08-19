#include "Penumbra/LifecycleRegistry.h"

#include <algorithm>

namespace Penumbra {

void LifecycleRegistry::RegisterLifecycle(IWidgetLifecycle* Lifecycle) {
    RegisteredLifecycles.push_back(Lifecycle);
    Lifecycle->OnMount();
}

void LifecycleRegistry::UnregisterLifecycle(IWidgetLifecycle* Lifecycle) {
    Lifecycle->OnUnmount();
    std::erase(RegisteredLifecycles, Lifecycle);
}

void LifecycleRegistry::Tick(float DeltaSeconds) {
    const TickInfo Info{DeltaSeconds};

    for (IWidgetLifecycle* Lifecycle : RegisteredLifecycles) {
        Lifecycle->OnTick(Info);
    }
}

} // namespace Penumbra
