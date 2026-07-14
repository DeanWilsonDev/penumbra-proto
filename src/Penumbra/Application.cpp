#include "Penumbra/Application.h"

namespace Penumbra {

void Application::RegisterLifecycle(IWidgetLifecycle* Lifecycle) {
    RegisteredLifecycles.push_back(Lifecycle);
    Lifecycle->OnMount();
}

void Application::UnregisterLifecycle(IWidgetLifecycle* Lifecycle) {
    Lifecycle->OnUnmount();
    std::erase(RegisteredLifecycles, Lifecycle);
}

void Application::Tick(float DeltaSeconds) {
    const TickInfo Info{DeltaSeconds};

    for (IWidgetLifecycle* Lifecycle : RegisteredLifecycles) {
        Lifecycle->OnTick(Info);
    }
}

} // namespace Penumbra
