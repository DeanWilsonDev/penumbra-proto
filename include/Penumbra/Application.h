#pragma once

#include "Penumbra/IWidgetLifecycle.h"
#include "Penumbra/LifecycleRegistry.h"
#include "Penumbra/Platform/InputState.h"
#include "Penumbra/Platform/PlatformWindow.h"
#include "Penumbra/Render/Color.h"
#include "Penumbra/Render/Renderer.h"
#include "Penumbra/Render/SdlTtfFontBackend.h"
#include "Penumbra/Widgets/WidgetBase.h"

#include <functional>
#include <memory>
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
// (docs/next_steps.md's "Application base class" entry).
//
// External widget-owning runtimes integrate through IWidgetLifecycle, never a
// concrete runtime include.
class Application {
public:
    Application();
    virtual ~Application();

    // Configure -> window/renderer/font-backend construction -> OnStart -> the
    // frame loop (pumps input, ticks registered lifecycles, calls
    // OnUpdate/OnRender once per frame) -> OnShutdown. Runs until the OS requests
    // quit or RequestQuit() is called; blocks for the application's whole
    // lifetime. Returns 1 if window/renderer construction failed or OnStart
    // returned false, 0 otherwise. Implemented as Initialize() -> RunOneFrame()
    // in a loop -> Shutdown(), below -- a driver that needs frame-by-frame
    // control (a visual-regression test harness stepping frames between
    // NAVIGATE calls, e.g.) calls those three directly instead of Run(), the
    // same real window/renderer/OnStart path, just not blocking for the whole
    // process lifetime.
    int Run();

    // Ends the frame loop after the current frame finishes.
    void RequestQuit();

    // Run()'s own three phases, public for a caller that needs to step frames
    // itself rather than block inside Run() for the application's whole
    // lifetime (a test harness driving a real window, e.g. -- GetWindow()/
    // GetRenderer() stay protected; a caller that only needs "construct the
    // real window and run frames" never needs either directly). Calling
    // Initialize()/RunOneFrame()/Shutdown() directly and calling Run() are
    // mutually exclusive for a given instance -- Run() already calls all
    // three itself.

    // Configure -> window/renderer/font-backend construction -> OnStart.
    // Returns false on window/renderer construction failure or if OnStart()
    // itself returned false (OnShutdown()/window teardown already ran in that
    // case, matching Run()'s own early-return behavior -- do not call
    // Shutdown() again after a false return).
    bool Initialize();

    // Runs exactly one iteration of Run()'s own frame body: pump OS events,
    // track DPI-scale changes, Tick() registered lifecycles, OnUpdate(),
    // Measure/Arrange/UpdateInteractionState the mounted root widget (if any),
    // then Draw()/OnRender()/present. Returns false when the OS asked to quit
    // (the window closed) or RequestQuit() was called during this frame --
    // the same "keep looping?" signal Run()'s own `while (!QuitRequested)`
    // re-checks every iteration, just handed back to the caller instead of
    // driving the loop itself. Undefined if called before a successful
    // Initialize().
    bool RunOneFrame();

    // OnShutdown() -> window teardown. Call after Initialize() succeeded and
    // the caller is done stepping frames (RunOneFrame() returned false, or the
    // caller decided to stop early) -- mirrors Run()'s own post-loop cleanup.
    void Shutdown();

    // Thin forwards onto an owned Penumbra::LifecycleRegistry (see
    // GetLifecycleRegistry() below) -- same behavior as always, now factored
    // out into a standalone class a non-Application host can also construct
    // directly (docs/next_steps.md's "IWidgetLifecycle registration/ticking
    // needs to work without owning a full Application" entry).
    void RegisterLifecycle(IWidgetLifecycle* Lifecycle);
    void UnregisterLifecycle(IWidgetLifecycle* Lifecycle);

    // Public so a reconciler wiring component lifecycles against whatever
    // host is available can obtain the registry without subclass access. This
    // lets an Application-backed host hand one over
    // (&App->GetLifecycleRegistry()) exactly the same way a hand-rolled,
    // non-Application host would hand over a LifecycleRegistry it
    // constructed and Tick()s itself, so BuildContext::LifecycleHost can be
    // typed as Penumbra::LifecycleRegistry* and accept either kind of host
    // uniformly.
    [[nodiscard]] LifecycleRegistry& GetLifecycleRegistry();

    // Public so adapters can dispatch lifecycle hooks through an Application*
    // without requiring a second framework-specific subclass.

    // Called once, after the window/renderer/font backend are ready. Build the
    // widget tree here. Returning false aborts Run() before the frame loop starts
    // (OnShutdown still runs).
    virtual bool OnStart() { return true; }

    // Called once per frame, before OnRender. Registered IWidgetLifecycle::OnTick
    // has already fired by the time this runs; reconcile external state here.
    // If a root widget has been handed to SetRootWidget(), Run() itself calls
    // Measure/Arrange/UpdateInteractionState on it immediately after this returns --
    // an override does not need (and should not duplicate) that sequence by hand.
    // Only a caller with no root widget mounted still owns that pass itself.
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
    // first). Lets an external host supply rendering without introducing an
    // application subclass.
    using RenderHook = std::function<void(Render::Renderer&)>;
    void                     SetOnRenderHook(RenderHook Hook);
    [[nodiscard]] bool       HasRenderHook() const;

    // Host-supplied alternative to overriding OnUpdate(). Takes priority over the
    // virtual OnUpdate() when set (Run()'s frame loop checks HasUpdateHook()
    // first), and -- unlike OnUpdate() -- is handed this frame's InputState, which
    // is otherwise unreachable outside Application's own member functions
    // (GetInput() is protected). This lets an external host drive interaction
    // state on a mounted widget tree without subclass access.
    using UpdateHook = std::function<void(float, const Platform::InputState&)>;
    void                     SetOnUpdateHook(UpdateHook Hook);
    [[nodiscard]] bool       HasUpdateHook() const;

    // Public so an external tree builder can measure text and load fonts before
    // mounting a widget tree.
    // Unlike the two hooks above, this isn't tied to a per-frame cadence -- a
    // caller only needs it once, so a bare accessor is enough; no hook required.
    [[nodiscard]] Render::IFontBackend& GetFontBackend();

    // Public so an external tree builder can load DPI-correct fonts and detect
    // display-DPI changes (docs/next_steps.md's "GetWindow/GetRenderer and
    // DPI scaling" entry). Forwards to the Renderer's own scale factor, which
    // Run() keeps synced to the window's every frame before OnDpiScaleChanged
    // fires -- not GetWindow/GetRenderer themselves; neither is needed just
    // for this, so neither is made public here.
    [[nodiscard]] float GetDpiScaleFactor() const;

    // Public so an external input adapter can enable SDL text-input mode for
    // the window before typed characters reach InputState::TextInputThisFrame
    // at all (docs/next_steps.md's "no way to enable SDL text-input mode"
    // entry). Not tied to a per-frame cadence -- a caller only needs to call
    // this once per focus change, matching src/main.cpp's own explicit
    // per-frame check -- so a bare accessor is enough; no hook required.
    void SetTextInputActive(bool Active);

    // Public so an external tree builder can Measure/Arrange against the live
    // window size instead of a compile-time guess. Forwards to the window
    // itself rather than exposing GetWindow(); this is the narrow public
    // geometry API.
    [[nodiscard]] Point GetWindowLogicalSize() const;

    // Takes ownership of Root. Once set, Run()'s own frame loop calls
    // Measure/Arrange/UpdateInteractionState on it automatically every frame --
    // right after OnUpdate (hook or virtual) returns, sized against
    // GetWindowLogicalSize() -- and Draw()s it every frame too, right after
    // Renderer::BeginFrame, before OnRender (hook or virtual) runs. This is the
    // same sequence a hand-rolled frame loop would otherwise drive itself;
    // once a root is mounted here, the app no longer needs its own copy of it.
    // Passing nullptr
    // un-mounts the current root (if any) and destroys it, matching
    // unique_ptr's own reset() semantics. Public, not protected, for the same
    // reason SetOnRenderHook/SetOnUpdateHook are: an external tree builder can
    // mount a real widget tree without subclass access or Penumbra acquiring
    // knowledge of its composition language.
    void SetRootWidget(std::unique_ptr<Widgets::WidgetBase> Root);

    // The widget last handed to SetRootWidget(), or nullptr if none is mounted.
    // Ownership stays with Application; this is a non-owning observer only.
    [[nodiscard]] Widgets::WidgetBase* GetRootWidget() const;

    // Whether the root widget's own UpdateInteractionState(...) call (part of
    // Run()'s automatic per-frame pass above) reported that it consumed this
    // frame's input, mirroring the bool pharos-proto's own updateWidgetTree()
    // returns today -- e.g. to gate a Backspace/Escape zoom-out handler behind
    // "no popover ate this click first". False (not stale) whenever no root
    // widget is mounted, so a caller never needs to null-check GetRootWidget()
    // just to read this.
    [[nodiscard]] bool GetRootWidgetConsumedInputThisFrame() const;

protected:
    // Fills Config before the window/renderer are constructed.
    virtual void Configure(ApplicationConfig& Config) {}

    // Called once per frame, between Renderer::BeginFrame and EndFrameAndPresent.
    // Draw the widget tree here.
    virtual void OnRender(Render::Renderer& Renderer) {}

    [[nodiscard]] Platform::PlatformWindow&   GetWindow();
    [[nodiscard]] Render::Renderer&           GetRenderer();
    [[nodiscard]] const Platform::InputState& GetInput() const;
    [[nodiscard]] const ApplicationConfig&    GetConfig() const;

private:
    // Forwards to Lifecycles.Tick(DeltaSeconds) -- kept as a private member
    // (rather than calling Lifecycles.Tick directly from Run()) only so
    // Run()'s own frame-loop reads the same "Tick()" name it always has.
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

    LifecycleRegistry Lifecycles;

    std::unique_ptr<Widgets::WidgetBase> RootWidget;
    bool                                 RootWidgetConsumedInputThisFrame{false};
};

} // namespace Penumbra
