#include "Penumbra/Application.h"

#include <algorithm>

namespace Penumbra {

Application::Application() = default;
Application::~Application() = default;

int Application::Run() {
    Configure(Config);

    if (!Window.Initialise(Config.Title.c_str(), Config.WindowLogicalWidth, Config.WindowLogicalHeight)) {
        return 1;
    }

    LastKnownDpiScaleFactor = Window.GetDpiScaleFactor();
    if (!Renderer.Initialise(Window.GetSdlRenderer(), LastKnownDpiScaleFactor, &FontBackend)) {
        Window.Shutdown();
        return 1;
    }

    if (!OnStart()) {
        OnShutdown();
        Window.Shutdown();
        return 1;
    }

    while (!QuitRequested) {
        if (!Window.PumpEventsAndBuildInput(Input)) break;

        const float CurrentDpiScaleFactor = Window.GetDpiScaleFactor();
        Renderer.SetDpiScaleFactor(CurrentDpiScaleFactor);
        if (CurrentDpiScaleFactor != LastKnownDpiScaleFactor) {
            LastKnownDpiScaleFactor = CurrentDpiScaleFactor;
            OnDpiScaleChanged(CurrentDpiScaleFactor);
        }

        Tick(Input.DeltaTimeSeconds);
        OnUpdate(Input.DeltaTimeSeconds);

        Renderer.BeginFrame(Config.ClearColor);
        OnRender(Renderer);
        Renderer.EndFrameAndPresent();
    }

    OnShutdown();
    Window.Shutdown();
    return 0;
}

void Application::RequestQuit() {
    QuitRequested = true;
}

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

Platform::PlatformWindow& Application::GetWindow() {
    return Window;
}

Render::Renderer& Application::GetRenderer() {
    return Renderer;
}

Render::IFontBackend& Application::GetFontBackend() {
    return FontBackend;
}

const Platform::InputState& Application::GetInput() const {
    return Input;
}

const ApplicationConfig& Application::GetConfig() const {
    return Config;
}

} // namespace Penumbra
