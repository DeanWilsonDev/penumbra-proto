# Penumbra — Next Steps

> Living backlog, not a session-changelog: add an entry for anything new
> and open; remove or mark an entry done the moment it's actually
> implemented, rather than letting finished work linger. First file of
> this kind in this repo (previously used `docs/*_requirements.md`, one
> file per investigation, still present as historical record — not
> migrated into here retroactively).
> Last updated: 2026-08-19.

> **2026-08-14 audit:** every entry that had accumulated under "Open items" (ten dated `Fixed`
> write-ups, 2026-08-03 through 2026-08-14, ~1000 lines) was actually resolved — none of them
> were genuinely open, just never moved out per this doc's own convention above. Moved verbatim
> to `docs/archive/penumbra_next_steps_resolved.md` (nothing rewritten, only relocated — see
> that file's own header for the index). One thread inside the tenth entry ("Nyx bridge + entry
> point mechanism," 2026-08-11) was never actually closed; it's condensed into the single open
> item below rather than archived with the rest.

> **2026-08-17 agent status note:** the multi-repo coordination agent assigned to this repo
> (name "penumbra" in that experiment) was briefed on this doc and the repo's clean/up-to-date
> `main` state, confirmed on standby, and received no ask before the fleet was retired. No work
> was started, no files were touched, nothing is in-flight or partially done. The one open item
> below is unchanged from the 2026-08-14 audit. This note exists only so a fresh session/agent
> picking this doc up next has an honest record that the prior agent did nothing beyond reading
> and reporting status.

> **2026-08-17: `GetWindowLogicalSize()` shipped.** The "no way to read the current window size"
> entry that had accumulated below the agent-status note above is now fixed — moved to
> `docs/archive/penumbra_next_steps_resolved.md` §1. `pharos-proto/src/nyx_app/main.cpp` was
> updated and verified (built + resize-tested) against this change too, via a local
> `FETCHCONTENT_SOURCE_DIR_PENUMBRA` override — not yet picked up on `pharos-proto`'s own `main`
> until this commit is pushed here (that repo tracks `penumbra-proto`'s `main` unpinned).

## Open items

### `IWidgetLifecycle` registration/ticking needs to work without owning a full `Application` — cross-repo ask from `pharos-proto`

**Status:** implemented 2026-08-19. `Penumbra::LifecycleRegistry` shipped
(`include/Penumbra/LifecycleRegistry.h`, `src/Penumbra/LifecycleRegistry.cpp`) — see "Implemented
fix" and "How it was verified" below for the final shape and how it was proven. What's still open
is entirely on `penumbra-ui-backend`'s side: it needs to actually widen `BuildContext::LifecycleHost`
to consume this. This repo's own piece of the cross-repo ask is done.

**The concrete case that hit this:** `pharos-proto` has two binaries mounting the same shared
`src/ui/nyx/App.irisx` tree. `pharos_nyx_bootstrap` (`src/nyx_app/main.cpp`) is built on a real
`Penumbra::Application` subclass and sets `penumbra-ui-backend`'s `BuildContext::LifecycleHost =
GApplication`, so `App.irisx`'s `<InspectorPanel />` child component gets a real `OnMount`/
`OnTick` and its row values sync live via the `GetRef(name).SetText/...` mechanism
(`penumbra-ui-backend`'s own `docs/next_steps.md`, done 2026-08-18). The real `pharos` binary
(`src/main.cpp`) mounts the *exact same* `App.irisx` file, but it's the hand-rolled
`Platform::PlatformWindow`-owning app `pharos-proto/CLAUDE.md`'s "Verifying UI changes" section
documents — it constructs `PlatformWindow`/`Renderer`/`SdlTtfFontBackend` directly and drives its
own frame loop, never a `Penumbra::Application`. So it has no object it can legally assign to
`BuildContext::LifecycleHost` (`Penumbra::Application* LifecycleHost{nullptr};`,
`penumbra-ui-backend/include/PenumbraUiBackend/Walker.h:89` — the field is typed as a concrete
`Application*`, not an interface). Once `App.irisx`'s Inspector splice became a real
`<InspectorPanel />` component invocation instead of a `<Native>` splice, this binary's Inspector
panel lost its only working sync path (it used to hand-roll its own C++ `panel.sync()` closure
against a separately-mounted tree, `src/ui/inspector_panel.cpp`'s `buildInspectorPanel()` — that
mechanism still runs every frame but now updates an orphaned, never-drawn widget tree, since
`App.irisx` no longer references the `<Native>` name it used to attach to). Full write-up of how
this was found in `pharos-proto`'s own `docs/next_steps.md`.

**Grounding, confirmed by reading `Application.h`/`Application.cpp` directly, not guessed:**
`RegisterLifecycle`/`UnregisterLifecycle`/the private `Tick(float)` and the
`std::vector<IWidgetLifecycle*> RegisteredLifecycles` member they operate on
(`include/Penumbra/Application.h:57-58,221`, `src/Penumbra/Application.cpp:101-117`) touch
**nothing else on `Application`** — no `Window`, `Renderer`, `FontBackend`, `Input`, or frame-loop
state:

```cpp
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
    for (IWidgetLifecycle* Lifecycle : RegisteredLifecycles) Lifecycle->OnTick(Info);
}
```

This is a self-contained, trivially-extractable mechanism bolted onto `Application` only because
`Application` was the first (and so far only) place `IWidgetLifecycle` got wired up — not because
lifecycle dispatch has any actual dependency on owning a window/renderer/frame loop.

**Implemented fix**, mirroring how `IWidgetLifecycle` itself was deliberately kept
Penumbra-independent (see that header's own doc comment — "should lift out into a standalone
`umbra-interfaces` library unchanged"): the three members/methods were factored out of
`Application` into a small standalone class, exactly as proposed:

```cpp
// include/Penumbra/LifecycleRegistry.h
class LifecycleRegistry {
public:
    void RegisterLifecycle(IWidgetLifecycle* Lifecycle);   // same OnMount-on-register behavior
    void UnregisterLifecycle(IWidgetLifecycle* Lifecycle); // same OnUnmount-on-unregister behavior
    void Tick(float DeltaSeconds);                          // same OnTick fan-out
private:
    std::vector<IWidgetLifecycle*> RegisteredLifecycles;
};
```

`Application` gained one as a private member (`LifecycleRegistry Lifecycles;`,
`include/Penumbra/Application.h`) and its own `RegisterLifecycle`/`UnregisterLifecycle`/`Tick`
became thin forwards — zero behavior change for `pharos_nyx_bootstrap` or any other existing
consumer. `Application` also gained a public accessor, `[[nodiscard]] LifecycleRegistry&
GetLifecycleRegistry()`, returning the same registry its own Register/Unregister/Tick forward to —
this is the piece that unblocks the "Required changes elsewhere" note below: an
`Application`-backed host hands over `&App->GetLifecycleRegistry()`, and a hand-rolled host like
`pharos-proto/src/main.cpp` constructs its own `Penumbra::LifecycleRegistry` directly (alongside
its own `PlatformWindow`/`Renderer`) and calls `.Tick(deltaSeconds)` once per frame from its own
loop — no window/renderer duplication, no need to become `Application`-based just to get `OnTick`
dispatch, and both kinds of host now hand over the exact same pointer type.

**Required changes elsewhere:** `penumbra-ui-backend`'s `BuildContext::LifecycleHost`
(`include/PenumbraUiBackend/Walker.h:89`) is still typed as a concrete `Penumbra::Application*` —
confirmed (by reading `Walker.cpp`'s `RegisterLifecycleIfPresent`) that it only ever calls
`Host->RegisterLifecycle(...)`/`Host->UnregisterLifecycle(...)` on it, nothing else, so it needs
only a straight retype to `Penumbra::LifecycleRegistry*` (no wrapper/interface needed) before a
hand-rolled host can actually plug in. That retype is `penumbra-ui-backend`'s own piece of this
ask, not done here — coordinate the exact shape with that repo's own `docs/next_steps.md`.

**What unblocks:** once `penumbra-ui-backend` retypes `LifecycleHost`, `pharos-proto/src/main.cpp`
(the real, non-Nyx-native `pharos` binary) could construct its own `Penumbra::LifecycleRegistry`,
set `BuildContext::LifecycleHost` to it, `.Tick()` it once per frame from its own hand-rolled loop,
and delete its own hand-rolled `inspector_panel.cpp`/`buildInspectorPanel()` `panel.sync()`
mechanism — the same way `pharos_nyx_bootstrap` already deleted `inspector_panel_native.cpp` —
letting both binaries share one live-synced `InspectorPanel.irisx` mount instead of only one of
them working.

**How it was verified:** built both configurations (`build/` — plain, `build-nyx/` —
`-DPENUMBRA_WITH_NYX=ON`) clean after adding `src/Penumbra/LifecycleRegistry.cpp` to
`CMakeLists.txt`'s `add_library(penumbra STATIC ...)` list (this repo's source list is explicit,
not `GLOB_RECURSE`, so this was a required manual step, followed by re-running `cmake -B build`/
`cmake -B build-nyx -DPENUMBRA_WITH_NYX=ON` before building). No demo or test in this repo
registers an `IWidgetLifecycle` today (confirmed by grep — matches the "zero implementers
anywhere in the ecosystem" note elsewhere in this doc), so compiling both configs alone doesn't
prove dispatch behavior. Wrote and ran a standalone smoke program (compiled ad hoc against the
built `libpenumbra.a`, not added to the permanent build) with a `RecordingLifecycle` that appends
a tag to a string on each hook call, exercised two ways: (1) through
`Application::RegisterLifecycle`/`UnregisterLifecycle` plus `GetLifecycleRegistry().Tick(...)` —
confirmed `OnMount` fires on register, `OnTick` fan-out carries the right `DeltaSeconds` to every
registered lifecycle, `OnUnmount` fires on unregister, and an unregistered lifecycle is not ticked
again afterward; (2) through a bare `Penumbra::LifecycleRegistry` constructed with no `Application`
involved at all, same three checks. Both passed, proving both the "zero behavior change through
`Application`" claim and the "hand-rolled host can use `LifecycleRegistry` standalone" claim this
entry was filed to unblock.

### `Application` needs to own the Measure/Arrange/UpdateInteractionState/Draw pass for a mounted root tree, not leave it to the app

**Status:** implemented 2026-08-17. `Application::SetRootWidget(std::unique_ptr<Widgets::WidgetBase>)`
shipped in `include/Penumbra/Application.h`/`src/Penumbra/Application.cpp` — see "Implemented API"
below for the final shape and "How it was verified" for how this was proven end-to-end. What's
still open is entirely on `penumbra-ui-backend`'s side: it needs to actually call this once its own
mount code (`BuildWidgetTree`/`IrisNyxDriver`) is ready to hand a built tree back in. This repo's
own piece of the cross-repo ask is done. Originated from `pharos-proto`'s own `docs/next_steps.md`
"Phase 3" entry (under "Nyx-native application") — not an internal bug report. Matching asks filed
in `iris-proto`, `nyx-proto`, and `penumbra-ui-backend`'s own docs for the rest of that design.

The user's ask, via `pharos-proto`: `Application` should have something like a real render
entry point that takes the app's UI tree, after which nothing app-side has to drive the frame
loop by hand — part of a broader "framework-owned component lifecycle system, hidden from the
developer" design (see `pharos-proto`'s entry for the full picture; not repeated here).

**Grounding, confirmed by reading `Application.h`/`Application.cpp` directly:** `Run()`'s frame
loop (`Application.cpp:30-54`) pumps input, syncs DPI, calls `Tick(DeltaSeconds)` (which already
fans `OnTick` out to every registered `IWidgetLifecycle*` — see below), calls `OnUpdate`, then
`Renderer.BeginFrame`/`OnRender`/`EndFrameAndPresent`. **At no point does `Application` itself
call `Measure`/`Arrange`/`UpdateInteractionState`/`Draw` on any widget tree — it owns no root
widget pointer at all.** That whole sequence is still 100% the consuming app's own job, exactly
matching `pharos-proto/src/nyx_app/main.cpp`'s hand-rolled `updateWidgetTree()` (its own
`GOverlayHost->Measure(...)`/`Arrange(...)`/`UpdateInteractionState(...)` calls, run from
Nyx-side `OnUpdate` via a registered host function). This is the one piece of the per-frame
loop that has never moved anywhere — not into Nyx (Phase 2a), not into a framework-owned
system — because `Application` has never had anywhere for it to move *to*.

Separately, `Application` already has real, live, currently-unused infrastructure that's
exactly shaped for the other half of this design: `RegisterLifecycle`/`UnregisterLifecycle`
and a flat `RegisteredLifecycles` vector (`Application.cpp:81-97`), whose `OnTick` fan-out
already runs every frame from `Tick()` — with zero implementers anywhere in the ecosystem
today (confirmed by grep across this repo, `penumbra-ui-backend`, and `iris-proto`). That part
doesn't need to change; it's the producer side (something registering against it) that's
missing, tracked in the sibling repo asks above.

### Implemented API

```cpp
// include/Penumbra/Application.h — public, alongside SetOnRenderHook/SetOnUpdateHook, for
// the same reason those are public: a caller that only holds an Application* obtained from
// Penumbra::Nyx::LoadApplication/LoadApplicationFromFile has no subclassing point.
class Application {
public:
    // Takes ownership of Root. Once set, Run()'s own frame loop calls
    // Measure/Arrange/UpdateInteractionState on it automatically every frame — right after
    // OnUpdate (hook or virtual) returns, sized against GetWindowLogicalSize() — and Draw()s
    // it every frame too, right after Renderer::BeginFrame, before OnRender (hook or
    // virtual) runs, so any extra host-drawn content layers on top rather than under it.
    // Passing nullptr un-mounts (and destroys) the current root.
    void SetRootWidget(std::unique_ptr<Widgets::WidgetBase> Root);

    // Non-owning observer; nullptr if nothing is mounted.
    [[nodiscard]] Widgets::WidgetBase* GetRootWidget() const;

    // Mirrors the bool pharos-proto's own updateWidgetTree() returns today — whether the
    // root widget's own UpdateInteractionState(...) call (part of the automatic pass above)
    // consumed this frame's input. False whenever no root widget is mounted.
    [[nodiscard]] bool GetRootWidgetConsumedInputThisFrame() const;
};
```

`Run()`'s loop now reads, in order: `Tick()` → `OnUpdate`(hook/virtual) →
[if a root is mounted] `RootWidget->Measure(WindowSize)` → `Arrange(WindowRect)` →
`UpdateInteractionState(Input)` (result cached for `GetRootWidgetConsumedInputThisFrame()`) →
`Renderer.BeginFrame` → [if mounted] `RootWidget->Draw(Renderer)` → `OnRender`(hook/virtual) →
`EndFrameAndPresent`. `OnUpdate`'s own doc comment was updated to say an override no longer
needs (or should) duplicate that Measure/Arrange/UpdateInteractionState sequence by hand once
a root is mounted.

### How it was verified

Built both configurations (`build/` — plain, `build-nyx/` — `-DPENUMBRA_WITH_NYX=ON`), then
exercised `SetRootWidget` end-to-end from exactly its target caller shape: `demo_nyx/main.cpp`'s
`CreateApplication()` builds a solid-colour `Box` filling the window and calls
`GApplication->SetRootWidget(...)` on the `Application*` returned by `LoadApplicationFromFile`
(`DemoApplication`, authored entirely in `DemoApp.nyx`, has no C++ subclassing point of its own —
the same "no subclassing point" caller `GetFontBackend()`/`GetWindowLogicalSize()` etc. were
built for). No `OnRender` override or `SetOnRenderHook` is registered anywhere in that demo — the
Box is the demo's entire visual, drawn purely by `Run()`'s automatic pass. Confirmed three ways:
(1) the host-registered `Tick` callback reads `GetRootWidget()->GetArrangedRect()` back at frame 2
and logs a real `1280x720`, proving the automatic Measure/Arrange ran (frame 1's own Tick call
fires *before* that frame's automatic pass, so frame 1 itself still reads as unarranged — a
verification-script ordering detail, not a framework bug); (2) `screencapture` against the running
app shows the whole window filled with the Box's colour, proving the automatic Draw ran; (3) a
real `cliclick` press+release into the window (after explicitly bringing the app frontmost via
`osascript`'s `System Events ... set frontmost of (first process whose unix id is <pid>)` —
plain window-content clicks alone did not reliably focus/route to an SDL window launched
backgrounded from a shell in this environment, worth knowing for future headed verification here)
reached the Box's `OnPressed` callback and logged it, proving the automatic
`UpdateInteractionState` call reaches real widget callbacks, not just layout.

### Required changes elsewhere

- `penumbra-ui-backend`: needs to be the thing that actually calls `SetRootWidget` once it's
  built the real tree via `BuildWidgetTree`/`IrisNyxDriver` — this repo intentionally didn't grow
  its own `.irisx`-parsing knowledge to do this itself. Coordinate the exact handoff shape (this
  signature) with that repo's own filed ask before it implements its side.

### What unblocks

`pharos-proto`'s `nyx_app/main.cpp` (or any future Nyx-native Penumbra app) no longer needs its
own `updateWidgetTree()`-equivalent function at all, once it (or `penumbra-ui-backend` on its
behalf) calls `SetRootWidget` — mounting the root tree once is enough, and every frame's
Measure/Arrange/UpdateInteractionState/Draw pass runs automatically as part of
`Application::Run()`.

### `Configure(ApplicationConfig&)` and `OnRender(Render::Renderer&)` still can't be bridged to Nyx script itself — blocked on an upstream `nyx-proto` change

**Status:** open since 2026-08-11, unaddressed since. Cross-repo blocker — the fix has to land
in `nyx-proto`, not here.

`nyx-proto/src/host/marshal.hpp` only marshals primitives and `void` (`bool`/`int32`/`int64`/
`float`/`double`/`std::string`) — no overload exists for a C++ reference type. That means two of
`Application`'s six lifecycle hooks structurally cannot be overridden from a `.nyx` script today:

- **`Configure(ApplicationConfig&)`** — a Nyx-driven app can't control its own window
  title/size/clear color from script. Every `LoadApplicationFromFile`-bootstrapped app is stuck
  on `ApplicationConfig`'s plain C++ defaults (1280×720 as of this writing).
- **`OnRender(Render::Renderer&)`** — a Nyx script still can't draw anything itself.
  `SetOnRenderHook` (shipped 2026-08-12, archived entry §5) gives a *host C++* caller a way to
  draw on a Nyx-bootstrapped app's behalf, which is why `pharos_nyx_bootstrap` isn't blocked on
  this day-to-day — but that's a workaround one level down, not Nyx script itself gaining
  drawing control. The gap this entry originally flagged (true Nyx-side `OnRender` override) is
  still exactly as blocked as it was on 2026-08-11.

The other four bridged hooks (`OnStart`/`OnUpdate`/`OnShutdown`/`OnDpiScaleChanged`) all marshal
fine since they're primitives/`void` only — this is specifically a reference-type gap.

No known consumer has hit a hard wall on this yet (`pharos_nyx_bootstrap` routes around both gaps
via host-side hooks), so there's no active pressure to file the `nyx-proto` change — noting it
here so it doesn't get re-discovered from scratch next time someone reaches for real Nyx-side
window/draw control. See `docs/archive/penumbra_next_steps_resolved.md` §8 for the full original
investigation (why the marshal-type gap exists, what was tried, what it would take to unblock).
