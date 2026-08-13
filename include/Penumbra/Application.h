#pragma once

#include "Penumbra/IWidgetLifecycle.h"
#include "Penumbra/Platform/InputState.h"
#include "Penumbra/Platform/PlatformWindow.h"
#include "Penumbra/Render/Color.h"
#include "Penumbra/Render/Renderer.h"
#include "Penumbra/Render/SdlTtfFontBackend.h"

#include <functional>
#include <string>
#include <vector>

namespace Penumbra {

// Filled in by Application::Configure, before the window/renderer are constructed.
// Defaults are used unchanged for anything Configure doesn't touch.
struct ApplicationConfig {
    std::string   Title{"Penumbra Application"};
    int           WindowLogicalWidth{1280};
    int           WindowLogicalHeight{720};
    Render::Color ClearColor{18, 18, 18, 255};
};

// The application host: owns PlatformWindow/Renderer/font-backend construction and
// the frame loop, and dispatches OnTick to registered IWidgetLifecycle
// implementations once per frame. A consumer subclasses this and overrides the
// hooks below to build and drive its own widget tree -- everything a hand-rolled
// main.cpp previously did itself (window/renderer construction, DPI-scale
// tracking, the frame loop) now lives here once instead of duplicated per app
// (docs/next_steps.md's "Application base class" entry). This is also the shape a
// Nyx script's own entry-point class will eventually inherit from, once
// nyx-proto's RegisterInheritableType/NyxBridge wiring is added for this type (not
// yet done -- see that same next_steps.md entry).
//
// This remains the only place Penumbra talks to a widget-owning runtime (Iris,
// later Umbra Engine) -- entirely through IWidgetLifecycle, never a concrete
// include.
class Application {
public:
    Application();
    virtual ~Application();

    // Configure -> window/renderer/font-backend construction -> OnStart -> the
    // frame loop (pumps input, ticks registered lifecycles, calls
    // OnUpdate/OnRender once per frame) -> OnShutdown. Runs until the OS requests
    // quit or RequestQuit() is called; blocks for the application's whole
    // lifetime. Returns 1 if window/renderer construction failed or OnStart
    // returned false, 0 otherwise.
    int Run();

    // Ends the frame loop after the current frame finishes.
    void RequestQuit();

    void RegisterLifecycle(IWidgetLifecycle* Lifecycle);
    void UnregisterLifecycle(IWidgetLifecycle* Lifecycle);

    // Public (not protected, unlike Configure/OnRender below) because
    // nyx-proto's RegisterInheritableType<T>::Override wires a Nyx super call
    // to a plain qualified call (`self.Application::OnStart()`) from a free
    // function outside the class hierarchy, which C++ only allows on public
    // members (Penumbra/Nyx/ApplicationBridge.h, docs/next_steps.md's
    // "Application base class" entry) -- these four are exactly the hooks
    // that bridge, since nyx-proto's marshalling only supports primitive
    // args/returns (bool/int/float/double/string/void) today.

    // Called once, after the window/renderer/font backend are ready. Build the
    // widget tree here. Returning false aborts Run() before the frame loop starts
    // (OnShutdown still runs).
    virtual bool OnStart() { return true; }

    // Called once per frame, before OnRender. Registered IWidgetLifecycle::OnTick
    // has already fired by the time this runs -- reconcile (e.g. iris::Tick()),
    // Measure/Arrange, and update interaction state here.
    virtual void OnUpdate(float DeltaSeconds) {}

    // Called once, after the frame loop ends, before the window is torn down.
    virtual void OnShutdown() {}

    // Fires whenever the window's DPI scale factor changes between frames (e.g.
    // the window moved to a different-DPI display). The Renderer's own scale
    // factor has already been updated by the time this runs -- reload fonts (or
    // anything else rasterised at the old scale) here; Penumbra does not track
    // font handles itself.
    virtual void OnDpiScaleChanged(float NewDpiScaleFactor) {}

    // Host-supplied alternative to overriding OnRender(). Takes priority over the
    // virtual OnRender() when set (Run()'s frame loop checks HasRenderHook()
    // first). Exists for callers that only hold an Application* obtained from
    // Penumbra::Nyx::LoadApplication/LoadApplicationFromFile -- a Nyx-authored
    // subclass can't override OnRender() itself (not bridgeable, see
    // Penumbra/Nyx/ApplicationBridge.h), but the C++ host loading it can still
    // draw by calling SetOnRenderHook directly on the returned pointer, no
    // subclassing point needed. Works identically for a plain C++ subclass too.
    using RenderHook = std::function<void(Render::Renderer&)>;
    void                     SetOnRenderHook(RenderHook Hook);
    [[nodiscard]] bool       HasRenderHook() const;

    // Host-supplied alternative to overriding OnUpdate(). Takes priority over the
    // virtual OnUpdate() when set (Run()'s frame loop checks HasUpdateHook()
    // first), and -- unlike OnUpdate() -- is handed this frame's InputState, which
    // is otherwise unreachable outside Application's own member functions (GetInput()
    // is protected). Exists for the same reason SetOnRenderHook does: a caller that
    // only holds an Application* obtained from Penumbra::Nyx::LoadApplication/
    // LoadApplicationFromFile has no subclassing point to reach InputState from, but
    // still needs it to drive WidgetBase::UpdateInteractionState on a tree it mounts
    // externally (docs/next_steps.md's "no way to reach InputState" entry). Works
    // identically for a plain C++ subclass too.
    using UpdateHook = std::function<void(float, const Platform::InputState&)>;
    void                     SetOnUpdateHook(UpdateHook Hook);
    [[nodiscard]] bool       HasUpdateHook() const;

protected:
    // Fills Config before the window/renderer are constructed. Not bridged to
    // Nyx (see the public block above): ApplicationConfig& isn't a
    // marshallable argument type.
    virtual void Configure(ApplicationConfig& Config) {}

    // Called once per frame, between Renderer::BeginFrame and EndFrameAndPresent.
    // Draw the widget tree here. Not bridged to Nyx, same reason as Configure.
    virtual void OnRender(Render::Renderer& Renderer) {}

    [[nodiscard]] Platform::PlatformWindow&   GetWindow();
    [[nodiscard]] Render::Renderer&           GetRenderer();
    [[nodiscard]] Render::IFontBackend&       GetFontBackend();
    [[nodiscard]] const Platform::InputState& GetInput() const;
    [[nodiscard]] const ApplicationConfig&    GetConfig() const;

private:
    void Tick(float DeltaSeconds);

    ApplicationConfig         Config;
    Platform::PlatformWindow  Window;
    Render::SdlTtfFontBackend FontBackend;
    Render::Renderer          Renderer;
    Platform::InputState      Input;
    float                     LastKnownDpiScaleFactor{1.0f};
    bool                      QuitRequested{false};
    RenderHook                OnRenderHookFn;
    UpdateHook                OnUpdateHookFn;

    std::vector<IWidgetLifecycle*> RegisteredLifecycles;
};

} // namespace Penumbra
