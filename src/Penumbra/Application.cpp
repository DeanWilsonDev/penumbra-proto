#include "Penumbra/Application.h"

#include <algorithm>
#include <utility>

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
        if (HasUpdateHook()) {
            OnUpdateHookFn(Input.DeltaTimeSeconds, Input);
        } else {
            OnUpdate(Input.DeltaTimeSeconds);
        }

        // The Measure/Arrange/UpdateInteractionState pass a hand-rolled frame loop
        // (e.g. pharos-proto's own updateWidgetTree()) would otherwise have to drive
        // itself -- runs automatically once a root is mounted via SetRootWidget(),
        // sized against the live window rather than a compile-time guess, same as
        // that hand-rolled call site does via GetWindowLogicalSize().
        if (RootWidget) {
            const Point WindowSize = Window.GetLogicalWindowSize();
            const Rect  WindowRect{0.0f, 0.0f, WindowSize.X, WindowSize.Y};
            RootWidget->Measure(WindowSize);
            RootWidget->Arrange(WindowRect);
            RootWidgetConsumedInputThisFrame = RootWidget->UpdateInteractionState(Input);
        } else {
            RootWidgetConsumedInputThisFrame = false;
        }

        Renderer.BeginFrame(Config.ClearColor);
        // Drawn before OnRender/the render hook so any extra host-side drawing (e.g.
        // a debug overlay) layers on top of the mounted tree rather than under it.
        if (RootWidget) {
            RootWidget->Draw(Renderer);
        }
        if (HasRenderHook()) {
            OnRenderHookFn(Renderer);
        } else {
            OnRender(Renderer);
        }
        Renderer.EndFrameAndPresent();
    }

    OnShutdown();
    Window.Shutdown();
    return 0;
}

void Application::RequestQuit() {
    QuitRequested = true;
}

void Application::SetOnRenderHook(RenderHook Hook) {
    OnRenderHookFn = std::move(Hook);
}

bool Application::HasRenderHook() const {
    return static_cast<bool>(OnRenderHookFn);
}

void Application::SetOnUpdateHook(UpdateHook Hook) {
    OnUpdateHookFn = std::move(Hook);
}

bool Application::HasUpdateHook() const {
    return static_cast<bool>(OnUpdateHookFn);
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

float Application::GetDpiScaleFactor() const {
    return Renderer.GetDpiScaleFactor();
}

void Application::SetTextInputActive(bool Active) {
    Window.SetTextInputActive(Active);
}

Point Application::GetWindowLogicalSize() const {
    return Window.GetLogicalWindowSize();
}

void Application::SetRootWidget(std::unique_ptr<Widgets::WidgetBase> Root) {
    RootWidget = std::move(Root);
}

Widgets::WidgetBase* Application::GetRootWidget() const {
    return RootWidget.get();
}

bool Application::GetRootWidgetConsumedInputThisFrame() const {
    return RootWidgetConsumedInputThisFrame;
}

const Platform::InputState& Application::GetInput() const {
    return Input;
}

const ApplicationConfig& Application::GetConfig() const {
    return Config;
}

} // namespace Penumbra
