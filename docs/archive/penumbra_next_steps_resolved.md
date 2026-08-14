# Penumbra — Next Steps: Resolved / Historical Entries

> Split out of `docs/next_steps.md` on 2026-08-14, the first time this convention is used in
> this repo (see `docs/archive/pharos_next_steps_resolved.md` and
> `docs/archive/iris_next_steps_resolved.md` in sibling repos for the same pattern). Nothing
> here was rewritten or summarized away — this is the original text, moved verbatim, in the
> same order it appeared in the live doc (most recent first). It is **not** a place new entries
> go; add those to `docs/next_steps.md` itself.
>
> One entry (§8, "Nyx bridge + entry point mechanism") was **not** fully closed at the time of
> this split — it shipped the bridge mechanism itself, but its own "Explicitly not requested"
> section flagged `Configure(ApplicationConfig&)`/`OnRender(Render::Renderer&)` bridging to Nyx
> script as blocked on an upstream `nyx-proto` change, never resolved since. That still-open
> thread is condensed into the live doc's "Open items" section rather than dropped — this
> archived copy is the full original write-up for reference, not the current status.
>
> Contents (each section is one dated `### Fixed ...` entry from the live doc, verbatim):
> 1. TextInput activation (2026-08-14)
> 2. DPI scale accessor (2026-08-13)
> 3. `GetFontBackend()` made public (2026-08-13)
> 4. `SetOnUpdateHook`/`InputState` access (2026-08-13)
> 5. `SetOnRenderHook`/`OnRender` hook (2026-08-12)
> 6. `PENUMBRA_WITH_NYX`/`amanuensis` build collision (2026-08-11)
> 7. `Application` base class + frame-loop ownership (2026-08-11)
> 8. Nyx bridge + entry point mechanism (2026-08-11) — partially superseded, see note above
> 9. `Box` `justify-content` (2026-08-10)
> 10. `Box` `FixedLeadingStack` (2026-08-03)

## 1. TextInput activation (2026-08-14)

### Fixed 2026-08-14: a `LoadApplicationFromFile`-bootstrapped `Application` has no way to enable SDL text-input mode — a focused `TextInput`'s click/Enter/Backspace all work, but typed characters never arrive

> **Trigger:** `pharos-proto`'s "Nyx-native application" effort building a real, free-text
> `JsonPathField`-based Toolbar for `pharos_nyx_bootstrap` (its own `docs/next_steps.md`,
> "Load schema is still a button" entry) — the last of the three real gaps that entry's own
> 2026-08-13 review found. Confirmed empirically, not just reasoned from source: focus-claim on
> click worked correctly (`FocusState::Focused` set to the `TextInput` on the right frame,
> verified via `fprintf` instrumentation since reverted), Backspace/Enter/other regular keys
> worked fine, but `cliclick t:'...'`-typed characters never once showed up in the field's own
> `Text`, across several repeated tries with verified-correct click coordinates.

**Root cause, confirmed directly against this repo's current source (not guessed):**

- `include/Penumbra/Platform/InputState.h:36`: `std::string TextInputThisFrame; // from SDL
  text-input events` — this is the *only* source `TextInput`'s own character-insertion logic
  reads from (confirmed against `TextInput::UpdateInteractionState` reading `Input.
  TextInputThisFrame`, not raw key codes, for literal character entry).
- `src/Penumbra/Platform/PlatformWindow.cpp:116,131-132`: `TextInputThisFrame` is populated
  *only* inside the `SDL_EVENT_TEXT_INPUT` case of the platform event pump:
  ```cpp
  OutInputState.TextInputThisFrame.clear();
  ...
  case SDL_EVENT_TEXT_INPUT:
      OutInputState.TextInputThisFrame += Event.text.text;
  ```
  SDL3 does not deliver `SDL_EVENT_TEXT_INPUT` for a window until `SDL_StartTextInput` has been
  called for it — text-input mode is off by default per window, unlike raw key-down/up events.
- `src/Penumbra/Platform/PlatformWindow.cpp:181-188` is the *only* place `SDL_StartTextInput`/
  `SDL_StopTextInput` are ever called, inside `PlatformWindow::SetTextInputActive(bool)`:
  ```cpp
  void PlatformWindow::SetTextInputActive(bool Active) {
      if (Active) {
          SDL_StartTextInput(Window);
      } else {
          SDL_StopTextInput(Window);
      }
  }
  ```
  `PlatformWindow::Initialise` never calls this itself (confirmed by reading it in full) — text
  input starts permanently off for every window unless something calls `SetTextInputActive(true)`
  explicitly.
- `include/Penumbra/Application.h:144`: `Platform::PlatformWindow& GetWindow();` is `protected`
  (block starts at line 134), same as `GetRenderer()`/`GetInput()`/`GetConfig()` — and
  `Application` itself never calls `SetTextInputActive` anywhere in `src/Penumbra/Application.cpp`
  (confirmed: no occurrence in that file at all). So a caller with no subclassing point (anyone
  holding only the `Application*` returned by `Penumbra::Nyx::LoadApplicationFromFile`, e.g.
  `pharos_nyx_bootstrap`) has no way to reach `SetTextInputActive` at all — not through
  `GetWindow()` (protected), not through any hook (`SetOnUpdateHook`/`SetOnRenderHook` hand back
  `InputState&`/`Renderer&`, neither owns this call), not through a public wrapper (none exists).
- The real host app, `pharos-proto/src/main.cpp:313-317`, shows what correct behavior looks like
  when a caller *does* own its own `PlatformWindow` directly:
  ```cpp
  const bool wantTextInput = (focus.Focused != nullptr);
  if (wantTextInput != textInputActive) {
      window.SetTextInputActive(wantTextInput);
      textInputActive = wantTextInput;
  }
  ```
  This path is closed to any `Application`-based app today.

### What shipped

Implemented exactly as proposed — a bare public wrapper, mirroring the `GetFontBackend()`/
`GetDpiScaleFactor()` precedent exactly, `GetWindow()`/`GetRenderer()`/etc. left `protected`:

```cpp
// include/Penumbra/Application.h, in the same public block GetFontBackend()/GetDpiScaleFactor()
// already live in
void SetTextInputActive(bool Active);
```

```cpp
// src/Penumbra/Application.cpp
void Application::SetTextInputActive(bool Active) { Window.SetTextInputActive(Active); }
```

A hook-based caller drives it exactly the way `src/main.cpp` already does, just calling
`GApplication->SetTextInputActive(...)` instead of a locally-owned `window.SetTextInputActive(...)`
— same "call once per focus-change" shape, no new hook type needed.

Verified by building `libpenumbra.a` and `penumbra_demo` clean (`Application.cpp` recompiles,
static library and demo executable both link with no errors) — confirming the widened surface
doesn't collide with anything.

**2026-08-14, later the same day: wired into `pharos_nyx_bootstrap` and verified end-to-end, in
a `pharos-proto` session.** Picked up via a plain `cmake -B build` reconfigure (no pin bump
needed — `pharos-proto` tracks this repo's `main`). `nyx_app/main.cpp` calls
`GApplication->SetTextInputActive(...)` once per focus-change, the same shape described above.
Verified via `cliclick`: cleared the JSON-path field with Backspace, typed `HELLO`, and confirmed
it was inserted at the cursor — the first time typed characters have landed in that field. See
`pharos-proto/docs/next_steps.md`'s own "Load schema is still a button" entry for the full
verification trace. This item is fully closed on both sides now.

### What this unblocked

`pharos_nyx_bootstrap`'s own `JsonPathField`-based Toolbar (already built and wired — focus,
Enter-submit, and Load-button-click all worked before this fix) can now accept typed characters,
making its free-text JSON-path field fully usable for anything other than the pre-filled default
path — confirmed above, not just theoretical.

### Explicitly not requested

- **Widening `GetWindow()` to public.** Same reasoning every prior entry in this doc has kept —
  nothing else currently needs the full `PlatformWindow&`, only this one narrow capability.
- **IME candidate-window positioning / `SDL_SetTextInputArea`.** Out of scope — the ask here is
  just "receive typed characters at all," not full IME composition-window support.
- **Auto-toggling text input based on focus inside `Application::Run()` itself.** Left as the
  caller's own responsibility (matching `src/main.cpp`'s own explicit per-frame check) rather than
  Penumbra guessing when a widget wants text input; a bare accessor is enough.


## 2. DPI scale accessor (2026-08-13)

### Fixed 2026-08-13: a `LoadApplicationFromFile`-bootstrapped `Application` has no way to reach the current DPI scale factor — blocked mounting the real `.irisx` UI tree, not just a bare native panel

> **Trigger:** picking up where the `IFontBackend`/`InputState` entries below left off —
> `pharos-proto`'s "Nyx-native application" effort has now mounted native, hand-wired ports of
> all four panels (Toolbar/Explorer/Atlas/Inspector) inside `pharos_nyx_bootstrap`, using the
> already-shipped `SetOnUpdateHook`/`SetOnRenderHook`/`GetFontBackend()`. The next milestone the
> user picked is the one those hooks were originally filed to eventually reach: mount the real
> `.irisx`-authored UI tree (`src/ui/nyx/App.irisx`, via `Iris::IrisNyxDriver` +
> `PenumbraUiBackend::BuildContext`) inside `pharos_nyx_bootstrap`'s own Nyx-owned window,
> replacing the four bare native panels — the same tree the separate, already-complete "Nyx
> integration" effort already mounts inside the hand-rolled `src/main.cpp` host app. Traced how
> far that gets with `Application`'s current public surface before concluding it needs another
> upstream ask, per `CLAUDE.md`'s "one rule that matters most here."

**Root cause, confirmed directly against this repo's current source (not guessed):**

- `include/Penumbra/Application.h:133-136`: `GetWindow()`, `GetRenderer()`, `GetInput()`, and
  `GetConfig()` are still `protected` — unchanged since the `InputState` entry below explicitly
  declined to widen them ("nothing currently needs them from outside the class"). That's no
  longer true for `GetRenderer()` specifically: it's the only place `Renderer::GetDpiScaleFactor()`
  lives (`include/Penumbra/Render/Renderer.h:142`), and nothing else exposes the current DPI
  scale to a caller with no subclassing point.
- `SetOnUpdateHook`'s `UpdateHook` alias (`Application.h:109`) is
  `std::function<void(float, const Platform::InputState&)>` — no DPI scale parameter. This is
  exactly the hook `pharos_nyx_bootstrap` uses to lazily build panels and load fonts
  (`PenumbraUiBackend::BuildContext::Font`, a `FontHandle` loaded via
  `IFontBackend::LoadFont(path, size, dpiScaleFactor)`), so it's the phase that actually needs
  the value.
- `src/Penumbra/Application.cpp:29-37` (`Run()`'s frame loop), confirmed the value *is* already
  correct and available internally every frame, just not surfaced:
  ```cpp
  const float CurrentDpiScaleFactor = Window.GetDpiScaleFactor();
  Renderer.SetDpiScaleFactor(CurrentDpiScaleFactor);
  if (CurrentDpiScaleFactor != LastKnownDpiScaleFactor) {
      LastKnownDpiScaleFactor = CurrentDpiScaleFactor;
      OnDpiScaleChanged(CurrentDpiScaleFactor);
  }
  ```
  This runs *before* either `SetOnUpdateHook`/`SetOnRenderHook` fires each frame, so
  `Renderer.GetDpiScaleFactor()` is always accurate by the time a hook body runs — the value
  just has no public accessor. `OnDpiScaleChanged(float)` (`Application.h:85`) is a public
  *virtual*, not a hook, so it's unreachable the same way `OnUpdate`/`OnRender` were before
  their own hooks landed — a `LoadApplicationFromFile`-only caller has no subclassing point to
  override it from.
- Checked whether this is actually needed, not assumed: `PenumbraUiBackend::BuildContext`
  (`penumbra-ui-backend/include/PenumbraUiBackend/Walker.h`) only reads
  `FontBackend`/`Font`/`Style`/`StyleApplier` — nothing from `Window` or `Renderer` directly.
  `Renderer&` itself is already delivered per-frame via `SetOnRenderHook`, and `InputState` via
  `SetOnUpdateHook` — DPI scale is the one remaining value a caller can't reach at all today.
  Confirmed against `pharos-proto`'s own `src/nyx_app/main.cpp:206-208`, which currently
  hardcodes every font load to `dpiScaleFactor=1.0f` with a comment naming this exact gap
  ("`GetRenderer()` (the only place to learn the real one) stays protected/unreachable from
  here"). The real host app, `src/main.cpp:80,254-271`, shows what correct behavior looks like
  when a caller *does* own its own `Window`/`Renderer`: it re-reads
  `window.GetDpiScaleFactor()` every frame and reloads fonts whenever it changes.

### What shipped

Implemented exactly as proposed — a bare accessor, not a new hook parameter, mirroring
`GetFontBackend()`'s own precedent exactly:

```cpp
// include/Penumbra/Application.h, in the same public block GetFontBackend() already lives in
[[nodiscard]] float GetDpiScaleFactor() const;
```

```cpp
// src/Penumbra/Application.cpp
float Application::GetDpiScaleFactor() const { return Renderer.GetDpiScaleFactor(); }
```

`GetWindow()`/`GetRenderer()`/`GetInput()`/`GetConfig()` stay `protected`, unchanged — only the
one missing piece (DPI scale) is exposed, as reasoned in "Explicitly not requested" below. A
caller still needs to re-check the value once per update-hook call to detect a change (DPI can
change frame-to-frame, e.g. the window moving to a different-DPI display) — the same
`CurrentDpiScaleFactor != LastKnownDpiScaleFactor` comparison `Run()` already does internally,
now doable by a hook-based caller too since it can read the current value at all.

Also wired into `pharos-proto`'s own `src/nyx_app/main.cpp:227-232` (the consumer this was
filed for): the hardcoded `dpiScaleFactor=1.0f` in the `GToolbar`/`GBodyFont` first-frame load
now reads `GApplication->GetDpiScaleFactor()` instead. Only the initial load is DPI-aware —
that hook only ever builds these fonts once, so there's still no per-frame reload-on-DPI-change
path there (unlike `src/main.cpp`'s own hand-rolled loop); left open for whoever needs that
next, noted inline at the call site.

Verified by building `libpenumbra.a` clean (`Application.cpp` recompiles, static library links
with no errors) — confirming the widened surface doesn't collide with anything. The
`pharos-proto` edit itself couldn't be built end-to-end in this session: `pharos-proto` tracks
`penumbra-proto`'s `main` via `FetchContent_Declare(... GIT_TAG main)`
(`cmake/Dependencies.cmake`), not a local path, so it only picks up this change once it's
pushed to `main` here — noted for whoever pushes this commit.

### What this unblocks

`pharos_nyx_bootstrap` now loads fonts at the real DPI scale (instead of the hardcoded `1.0f`)
and *can* detect scale changes the same way `src/main.cpp` already does by hand, once a future
session adds the per-frame re-check — the last piece named in `pharos-proto/docs/next_steps.md`'s
"Nyx-native application" entry blocking it from mounting the real `Iris::IrisNyxDriver`-built
`App.irisx` tree (the actual Toolbar/Explorer/Atlas/Inspector panels, not the four bare
hand-wired native ports it uses today) inside its own Nyx-owned window.

### Explicitly not requested

- **Widening `GetWindow()`/`GetInput()`/`GetConfig()` to public.** Nothing currently needs them
  from outside the class — `Renderer&`/`InputState&` are already delivered per-frame via the
  existing two hooks, and window size is a fixed, known `ApplicationConfig` default (per the
  `InputState` entry below's own note). Only `GetRenderer()`'s one missing piece (DPI scale) is
  requested here, as a narrow accessor rather than exposing `Renderer&`/`PlatformWindow&`
  themselves.
- **A `SetOnDpiScaleChangedHook` mirroring the protected `OnDpiScaleChanged` virtual.** Not
  needed — a bare accessor lets a hook-based caller do the same before/after comparison `Run()`
  already does internally, without a third hook type.
- **Bridging DPI scale into Nyx script itself.** Host-C++-side access only, same scoping every
  prior ask in this doc has kept.


## 3. `GetFontBackend()` made public (2026-08-13)

### Fixed 2026-08-13: a `LoadApplicationFromFile`-bootstrapped `Application` has no way to reach its own `IFontBackend` — blocks building any real (text-bearing) widget tree from outside the class

> **Trigger:** picking up where the just-fixed `InputState` entry below left off —
> `pharos-proto`'s "Nyx-native application" effort is now trying to actually mount a real
> `Penumbra::Widgets` tree (an `Iris::IrisNyxDriver`-built Explorer panel) inside
> `pharos_nyx_bootstrap`, using the newly-shipped `SetOnUpdateHook`/`SetOnRenderHook` pair.
> Building that tree needs an `IFontBackend*` for text measurement
> (`PenumbraUiBackend::BuildContext::FontBackend`) before the first frame ever runs — traced
> how a `pharos_nyx_bootstrap`-shaped caller (an `Application*` from `LoadApplicationFromFile`,
> no subclassing point) would reach one, and it can't.

**Root cause, confirmed directly against this repo's current source (not guessed):**

- `include/Penumbra/Application.h:125`: `[[nodiscard]] Render::IFontBackend&
  GetFontBackend();` stayed `protected`, same "only reachable from within the class hierarchy"
  problem `GetInput()` had — and `pharos_nyx_bootstrap`'s own `main.cpp` never subclasses
  `Application` at all (it only ever holds the plain `Application*`
  `LoadApplicationFromFile` hands back).
- `include/Penumbra/Render/Renderer.h:166`: `Renderer`'s own `FontBackend` member is
  `private`, no getter — so even though `SetOnRenderHook`'s lambda already receives a real
  `Render::Renderer&` every frame, there's no way to recover the `IFontBackend*` paired with
  it from that reference either.
- Net effect: there was no path, hook or otherwise, for a `LoadApplicationFromFile`-only
  caller to obtain an `IFontBackend*`/load a `FontHandle` at all.

### What shipped

Implemented exactly as proposed — smaller than the `InputState` fix, no new hook type needed.
`Application.h`'s `[[nodiscard]] Render::IFontBackend& GetFontBackend();` moved out of the
`protected:` block into the public block `SetOnRenderHook`/`SetOnUpdateHook` already live in,
right after `HasUpdateHook()`. Unlike `OnUpdate`/`OnRender`, `Application.h` never assigned
`FontBackend` access to a specific per-frame-cadence hook's own documented responsibility —
it's a plain resource accessor, the same shape `GetRenderer()`/`GetWindow()` already have,
just needed by a caller with no subclassing point. A caller only ever needs it *once* (to
build a widget tree and load fonts, typically lazily on the first frame after
window/renderer construction, not every frame), so a `SetOnRenderHook`-style hook parameter
would have been over-engineering — a bare public getter is both smaller and matches what's
actually needed. `GetWindow()`/`GetRenderer()`/`GetInput()`/`GetConfig()` stay `protected`,
unchanged.

Verified by building `libpenumbra.a` clean (`Application.cpp` — the only translation unit
that defines `GetFontBackend()` — recompiles and the static library links with no errors),
confirming the widened access doesn't collide with anything and the declaration/definition
still agree.

**What this unblocks:** `pharos_nyx_bootstrap` can now call `GetFontBackend()` directly on
the `Application*` `LoadApplicationFromFile` already returns, to lazily build a real
`Iris::IrisNyxDriver`-mounted widget tree (starting with a native-`NyxTree`-backed Explorer
panel, `pharos-proto`'s own `docs/next_steps.md`) — the same way `SetOnUpdateHook` unblocked
reaching `InputState` for that same tree's interaction state.

**Explicitly not requested:** bridging `IFontBackend`/font loading into Nyx script itself —
same host-C++-side-only scoping every prior ask here has kept. Not requesting `GetRenderer()`/
`GetWindow()` be made public too — nothing currently needs them from outside the class (the
render hook already carries `Renderer&` directly), so only the one accessor that was actually
missing was widened here.


## 4. `SetOnUpdateHook`/`InputState` access (2026-08-13)

### Fixed 2026-08-13: a `LoadApplicationFromFile`-bootstrapped `Application` has no way to reach that frame's `InputState` — blocks mounting a real (interactive) widget tree from outside the class

> **Trigger:** `pharos-proto`'s "Nyx-native application" effort (its own `docs/next_steps.md`)
> has reached full parity on data (load → native tree → treemap layout) and now draws the
> result via `SetOnRenderHook` — but only as raw `Renderer::DrawFilledRect`/`DrawRectOutline`
> calls, not real widgets. The next milestone the user picked is converging it with the
> separate, already-complete "Nyx integration" effort: mount the real `.irisx`-authored UI tree
> (`src/ui/nyx/App.irisx` — Explorer/Inspector/Atlas panels, toolbar, all real interactive
> `Penumbra::Widgets`) inside `pharos_nyx_bootstrap`'s own Nyx-owned window, replacing the
> rect-drawing lambda. Traced how far that gets with `Application`'s current public surface
> before concluding it needs an upstream ask, per `CLAUDE.md`'s "one rule that matters most
> here."

**Root cause, confirmed directly against this repo's current source (not guessed):**

- `include/Penumbra/Application.h:109-113`: `GetWindow()`, `GetRenderer()`, `GetFontBackend()`,
  `GetInput()`, and `GetConfig()` are all `protected`. A caller that only holds the
  `Application*` `LoadApplicationFromFile` returns (never a subclass — the Nyx script itself is
  the "subclass," bridged via `NyxBridge<Application>`, and `pharos_nyx_bootstrap`'s own
  `main.cpp` never derives from `Application` at all) cannot call any of these. The only public
  surface today besides `Run`/`RequestQuit`/`RegisterLifecycle`/`UnregisterLifecycle` is the
  `SetOnRenderHook`/`HasRenderHook` pair added for the entry above.
- `include/Penumbra/IWidgetLifecycle.h`: `RegisterLifecycle` is public, but
  `IWidgetLifecycle::OnTick(const TickInfo& Info)` only carries `DeltaSeconds`
  (`Application.cpp:80-86`, `Tick()`'s own construction of `TickInfo`) — no `InputState` reaches
  it either. There is no path today, hook or otherwise, for anything outside `Application`'s own
  member functions to see that frame's `Platform::InputState` (mouse position, click edges, text
  input, keys).
- This matters specifically because mounting a real widget tree needs
  `WidgetBase::UpdateInteractionState` called every frame with the real `InputState` (this
  repo's own retained-mode convention — every existing Penumbra app, `pharos-proto`'s hand-rolled
  `main.cpp` included, does this from its own owned frame loop). `pharos_nyx_bootstrap` has no
  owned frame loop — `Application::Run()` owns it — so without some form of `InputState` access,
  a mounted tree could be drawn (via the existing render hook) but never actually clicked,
  hovered, or scrolled.
- Window/viewport size is *not* part of this ask: `Configure(ApplicationConfig&)` still can't be
  overridden from an external `Application*` or from Nyx (unchanged from the entry below —
  `ApplicationConfig&` isn't marshallable and isn't hook-exposed either), so a
  `LoadApplicationFromFile`-bootstrapped app's window size is always exactly
  `ApplicationConfig`'s own defaults (`Application.h:19-23`, currently 1280×720). That's a fixed,
  known constant a caller can already rely on without any new accessor — only `InputState` is
  genuinely unreachable.

### What shipped

Implemented exactly as proposed — mirrors `SetOnRenderHook`'s own precedent exactly, same file,
same "public hook takes priority over the protected virtual" pattern. `Application.h`:
`using UpdateHook = std::function<void(float, const Platform::InputState&)>;`, a public
`void SetOnUpdateHook(UpdateHook Hook)` / `[[nodiscard]] bool HasUpdateHook() const` pair
(declared alongside `SetOnRenderHook`/`HasRenderHook`), and a private `UpdateHook
OnUpdateHookFn` member. `Application.cpp`'s `Run()` frame loop now checks `HasUpdateHook()`
in the same slot `OnUpdate()` already fired from, immediately after `Tick(...)`:

```cpp
Tick(Input.DeltaTimeSeconds);
if (HasUpdateHook()) {
    OnUpdateHookFn(Input.DeltaTimeSeconds, Input);
} else {
    OnUpdate(Input.DeltaTimeSeconds);
}
```

— falling back to the untouched virtual `OnUpdate(float)` dispatch when no hook is set, so
ordinary C++ subclasses (`demo/main.cpp`-style apps) are unaffected. Kept as its own hook
(firing where `OnUpdate` already fires) rather than just widening `GetInput()` to `public`,
matching `Application.h`'s own existing doc comment on `OnUpdate` ("reconcile ... Measure/
Arrange, and update interaction state here") — the render hook's own doc comment says only
"draw the widget tree here." A `pharos_nyx_bootstrap`-style caller uses `SetOnUpdateHook` to
reconcile/measure/arrange/`UpdateInteractionState` its mounted tree, and the already-shipped
`SetOnRenderHook` to draw it — the same two-phase split every other Penumbra app's own frame
loop already has, just externalized through two hooks instead of one owned loop.

Verified two ways: `cmake --build` (library + `penumbra_demo`) stays clean, zero new warnings.
Then a standalone scratch program (this repo's usual verification precedent, no automated
widget test suite), linked directly against `libpenumbra.a` and run under
`SDL_VIDEODRIVER=dummy`, driven for three frames via `RequestQuit()`: with no hook set, the
subclass's overridden `OnUpdate` fired all three times and `HasUpdateHook()` read `false`
throughout; with `SetOnUpdateHook` called before `Run()`, the hook fired all three times (each
call receiving a live `const InputState&`), the subclass's `OnUpdate` override fired zero
times, and `HasUpdateHook()` read `true` — confirming the hook actually takes priority over
virtual dispatch and reaches `InputState`, not just that both compile.

### What this unblocks

`pharos-proto`'s `pharos_nyx_bootstrap` can now call `SetOnUpdateHook` on the `Application*`
`LoadApplicationFromFile` already returns, with a lambda that runs
`WidgetBase::UpdateInteractionState` (and any reconcile/Measure/Arrange) against the real
per-frame `InputState`, alongside the already-shipped `SetOnRenderHook` for drawing — the last
piece needed before a real `.irisx`-authored widget tree mounted inside `pharos_nyx_bootstrap`
can actually be clicked, hovered, or scrolled, not just drawn.

### Explicitly not requested

- **Bridging `InputState` (or anything else) into Nyx script itself.** This is host-C++-side
  access only, same scoping the `SetOnRenderHook` entry above already established for
  `Renderer&`.
- **A `Configure`/window-size hook.** Not actually needed — window size is already a fixed,
  known constant (`ApplicationConfig`'s own defaults) a caller can rely on without any new
  accessor.


## 5. `SetOnRenderHook`/`OnRender` hook (2026-08-12)

### Fixed 2026-08-12: a `LoadApplicationFromFile`-bootstrapped `Application` has no way to actually draw anything — picking up the "real, load-bearing gap" flagged in the Nyx-bridge entry below

> **Trigger:** `pharos-proto`'s "Nyx-native application" effort (its own `docs/next_steps.md`)
> reached the point where its `.nyx`-driven app, `pharos_nyx_bootstrap`, computes a real
> treemap layout every run (`NyxTree::ComputeTreemap`, a native Nyx `TreeNode`/`TreeBuilder`)
> and logs `NativeSquarifiedTreemap produced 248 rects` — but nothing puts those rects on
> screen. This is exactly the gap the "Fixed 2026-08-11: Nyx bridge + entry point mechanism"
> entry below already named and deliberately deferred: "a real, load-bearing gap for
> `pharos-proto`'s actual 'own window bootstrap' milestone, left for whoever picks that up
> next to design deliberately rather than guessed at here." This is that pickup, from
> `pharos-proto`.

**Root cause, confirmed directly against this repo's current source (not guessed):**

- `include/Penumbra/Application.h:94`: `virtual void OnRender(Render::Renderer& Renderer) {}`
  stays `protected`, and its C++ default is a no-op — unchanged since the entry below already
  established this can't be bridged to Nyx script (`nyx-proto`'s `marshal.hpp` only marshals
  primitives/`void`, not a reference type like `Render::Renderer&`; a `nyx-proto` change, out
  of scope here, and this new entry doesn't ask for that either — see below).
- `src/Penumbra/Nyx/ApplicationBridge.cpp:15-42`: the `NyxBridge<Penumbra::Application>`
  specialization overrides exactly four hooks — `OnStart`, `OnUpdate`, `OnShutdown`,
  `OnDpiScaleChanged` — each `Invoke`-ing a Nyx-side override when one exists, falling back to
  `Application`'s real C++ default otherwise. It never overrides `OnRender` at all, so
  `Application::OnRender`'s no-op default is what actually runs, unconditionally, for every
  Nyx-bootstrapped app.
- `LoadApplication` (`ApplicationBridge.cpp:73-97`) and `LoadApplicationFromFile`
  (`:99-109`) construct the `NyxBridge<Application>` instance internally
  (`Host.Runtime.MountBridged<Application>(...)`) and hand back a plain `Application*`
  (`ApplicationBridge.h:38-44`) — the caller (e.g. `pharos_nyx_bootstrap`'s own `main.cpp`,
  which only calls `Penumbra::Nyx::LoadApplicationFromFile` and returns the result from
  `CreateApplication()`) has no subclassing point to supply its own `OnRender` override onto
  that already-constructed object. There is today no way at all — Nyx-side or host-C++-side —
  for a `LoadApplicationFromFile`-returned `Application*` to draw anything.

**Why this is narrower than "bridge `OnRender` to Nyx"**: Nyx doesn't need to draw anything
itself for `pharos-proto`'s milestone — it only needs to keep computing data, which already
works end to end (`ComputeTreemap`'s real rects, logged and verified). What's missing is a
*host*-side way in: `pharos_nyx_bootstrap`'s own `main.cpp` already has everything it needs to
draw (the computed rects, held in its own `GTree` global) — it just has no hook that runs
during the frame's render pass to do it with.

### What shipped

Put the hook on `Application` itself, not the bridge — no change to `ApplicationBridge.h`/
`.cpp`, `LoadApplication`/`LoadApplicationFromFile`'s signatures, or the Nyx registration, as
predicted: `NyxBridge<Application>` already *is-a* `Application`
(`ApplicationBridge.cpp:16`) and never overrides `OnRender`, so the new behavior on
`Application::OnRender`'s call site is inherited by every Nyx-bootstrapped app for free.

`include/Penumbra/Application.h`: `using RenderHook = std::function<void(Render::Renderer&)>;`,
a public `void SetOnRenderHook(RenderHook Hook)` / `[[nodiscard]] bool HasRenderHook() const`
pair (declared alongside the four Nyx-bridged hooks, not the `protected` `OnRender` block —
the whole point is a caller that only holds an `Application*` from `LoadApplicationFromFile`
needs public access to set it), and a private `RenderHook OnRenderHookFn` member.
`SetOnRenderHook` is `void`, not chaining-return — `Application*` isn't a builder, and a
return value here would just invite chained calls that don't read as anything meaningful.
`HasRenderHook()` is the public query pattern (mirrors `Get*` accessors already on
`Application`) rather than exposing the stored `RenderHook` itself.

`src/Penumbra/Application.cpp`: `Run()`'s frame loop now checks `HasRenderHook()` first —

```cpp
Renderer.BeginFrame(Config.ClearColor);
if (HasRenderHook()) {
    OnRenderHookFn(Renderer);
} else {
    OnRender(Renderer); // unchanged virtual dispatch — plain C++ subclasses still override this
}
Renderer.EndFrameAndPresent();
```

— falling back to the untouched virtual `OnRender()` dispatch when no hook is set, so ordinary
C++ subclasses (`demo/main.cpp`-style apps) are unaffected. `SetOnRenderHook`/`HasRenderHook`
themselves are two one-line definitions in the same file.

Verified two ways: `cmake --build` (library + `penumbra_demo`) stays clean, zero new warnings.
Then a standalone scratch program (this repo's usual verification precedent, no automated
widget test suite) subclassing `Application`, run under `SDL_VIDEODRIVER=dummy`, driven for
three frames via `RequestQuit()` from `OnUpdate`: with no hook set, the subclass's overridden
`OnRender` fired all three times and `HasRenderHook()` read `false` throughout; with
`SetOnRenderHook` called before `Run()`, the hook fired all three times and the subclass's
`OnRender` override fired zero times — confirming the hook actually takes priority over virtual
dispatch, not just that both compile.

### What this unblocks

`pharos-proto`'s `pharos_nyx_bootstrap` can now call `SetOnRenderHook` on the `Application*`
`LoadApplicationFromFile` already returns, with a lambda that reads its own `GTree` (already
holds the computed rects after `ComputeLayout`) and issues real `Renderer::` draw calls — the
last piece needed to actually see the loaded tree on screen, rather than just a log line
reporting a rect count. Didn't touch `nyx-proto` at all.

### Explicitly not requested

- **Bridging `OnRender(Render::Renderer&)` to Nyx script itself.** Still structurally blocked
  on `nyx-proto/src/host/marshal.hpp` only marshalling primitives/`void` (confirmed in the
  entry below) — unchanged, and not what this ask needs anyway, since Nyx only has to compute
  data, not draw it.
- **A 2D drawing API exposed to Nyx.** Same reasoning — the host C++ side already knows how to
  draw via `Renderer::`; nothing here asks Nyx to gain drawing primitives of its own.
- **Bridging `Configure(ApplicationConfig&)`.** Unrelated to rendering; left exactly as the
  entry below already scoped it.


## 6. `PENUMBRA_WITH_NYX`/`amanuensis` build collision (2026-08-11)

### Fixed 2026-08-11: `PENUMBRA_WITH_NYX`'s own build wiring collided with a consumer's `amanuensis` target — found immediately while `pharos-proto` tried to actually consume the Nyx bridge

> **Trigger:** `pharos-proto` enabled `PENUMBRA_WITH_NYX` in its own `cmake/Dependencies.cmake`
> (`set(PENUMBRA_WITH_NYX ON CACHE BOOL "" FORCE)` before `FetchContent_MakeAvailable(penumbra)`)
> to consume exactly the capability the "Nyx bridge + entry point mechanism" entry below just
> shipped — the very next step after that entry's own "What this unblocks" paragraph. It
> didn't build. Not a `pharos-proto`-side mistake: traced to this option's own CMake wiring
> here, confirmed against real source on both sides, not guessed.
> **Status:** fixed. This repo's own `PENUMBRA_WITH_NYX` block in `CMakeLists.txt` no longer
> `add_subdirectory()`s Firefly's vendored tree — see "Fix applied" below.

**Symptom, reproduced two ways in `pharos-proto`'s own build (which already has its own
`amanuensis` target, declared by `iris-proto`'s vendored `libs/amanuensis`):**

1. With `pharos-proto`'s existing fetch order unchanged (`penumbra` fetched before `iris`):
   configure succeeds silently, but `iris-proto`'s own sources then fail to compile —
   `fatal error: 'amanuensis/json.hpp' file not found` (`IrisConfig.cpp`, `IrisIr.cpp`,
   `IrisIrDocument.cpp`).
2. Reordering `pharos-proto`'s own fetches so `iris` runs first instead (to make its
   self-guarded `amanuensis` target win first): configure itself now hard-fails —
   `CMakeLists.txt:16 (add_library): add_library cannot create target "amanuensis" because
   another target with the same name already exists.`

**Root cause, traced through real source on both sides:**

This repo's own `CMakeLists.txt` (`PENUMBRA_WITH_NYX` block) does:

```
add_subdirectory(${nyx_SOURCE_DIR}/external/firefly ${CMAKE_CURRENT_BINARY_DIR}/_deps/firefly-build)
```

running Firefly's own full `CMakeLists.txt`, which itself unconditionally does
`add_subdirectory(external/amanuensis)` — no `if(TARGET amanuensis)` guard of any kind. Two
things compound to make this actually break, not just theoretically risky:

- `amanuensis`'s own `CMakeLists.txt` self-guards (`if(TARGET amanuensis) return() endif()`)
  in its *current* form — but the specific copy nested inside `nyx-proto`'s vendored Firefly,
  at whatever commit `nyx-proto`'s own `external/firefly` submodule pin points to
  (`ebb5d80` as of this writing), does **not** have that guard — confirmed by reading
  `build/_deps/nyx-src/external/firefly/external/amanuensis/CMakeLists.txt` directly in
  `pharos-proto`'s own build tree: a bare `add_library(amanuensis STATIC ...)`, no guard.
  Firefly's own *current* `main` (commit `5e1bad3`, checked directly in the sibling
  `~/development/projects/firefly` checkout) already bumped its own `amanuensis` submodule pin
  to `60ff233`, which does have the guard — so `nyx-proto`'s own vendored Firefly pin is stale
  relative to Firefly's own `main`, and carries the old, unguarded copy transitively.
- Because that specific copy is unguarded, whichever order the two `add_subdirectory(amanuensis)`
  calls (this repo's, via Firefly; the consumer's own, e.g. `iris-proto`'s) happen to run in,
  the result is bad: if the guarded one runs first, the second (unguarded) one hard-errors
  (repro #2 above); if the unguarded one runs first, it silently "wins" and every consumer
  expecting the *other* copy's headers breaks instead (repro #1 above, where `iris-proto`'s own
  sources expected `libs/amanuensis`'s `amanuensis/json.hpp`, present in its own vendored copy
  but not necessarily laid out identically in Firefly's differently-pinned one).

This is exactly the trap `iris-proto`'s own `CMakeLists.txt` already documents and deliberately
avoids for this identical dependency (`nyx-proto`) — its own comment reads: *"Deliberately does
NOT add_subdirectory(libs/nyx-proto) and build nyx-proto's own CMakeLists.txt: that pulls in
nyx-proto's own vendored Firefly/Amanuensis/Cimmerian submodules, which would define amanuensis
and cimmerian targets a second time and collide with this repo's own libs/amanuensis and
libs/cimmerian."* `iris-proto` instead compiles only the two specific lexer `.cpp` files it
actually needs from `nyx-proto`, directly, never running `add_subdirectory` on the vendored tree
at all. `pharos-proto`'s own `cmake/Dependencies.cmake` independently arrived at the same fix for
its own (separate, top-level) `nyx-proto` dependency: populate-source-only
(`FetchContent_Populate`, never `_MakeAvailable`/`add_subdirectory`), then compile `nyx-core`'s
and Firefly's own specific `.cpp` files directly against a single already-declared `amanuensis`
target — see that repo's own `cmake/Dependencies.cmake`, the `nyx`/`firefly`/`nyx-core` block,
for a working reference implementation of exactly this pattern, already proven in that repo's
own build. This repo's own `PENUMBRA_WITH_NYX` wiring is the one place in the whole ecosystem
that still does the unsafe `add_subdirectory`-the-whole-vendored-tree thing instead.

### Fix applied

`nyx-proto`'s own `external/firefly` submodule pin was first bumped `ebb5d80` → `5e1bad3`
(picking up Firefly's own guarded `amanuensis` copy) — masks the immediate symptom, but per the
analysis above doesn't fix the underlying fragility on its own, so the real fix was also applied:
the `PENUMBRA_WITH_NYX` block in this repo's own top-level `CMakeLists.txt` no longer calls
`add_subdirectory(${nyx_SOURCE_DIR}/external/firefly ...)` — it never runs `nyx-proto`'s or
Firefly's own `CMakeLists.txt` at all now. `nyx-proto`'s source is still populated via
`FetchContent_Populate` only; the specific `amanuensis`, `firefly`, and `nyx-core` `.cpp` files
this module needs are compiled directly into their own targets here instead, each guarded with
`if(NOT TARGET amanuensis)` / `if(NOT TARGET firefly)` / `if(NOT TARGET nyx-core)` — so a
consumer that's already declared any of these itself (e.g. `pharos-proto` via `iris-proto`'s
vendored `amanuensis`, or by `FetchContent`ing `nyx-proto` directly the way `pharos-proto` does
for `nyx-core`) reuses that existing target instead of colliding with a second declaration.
`pharos-proto`'s own `cmake/Dependencies.cmake` nyx/firefly/nyx-core block was the working
reference implementation this pattern was copied from. Verified with a full clean rebuild of
both `build` (`PENUMBRA_WITH_NYX` off, unaffected) and `build-nyx` (`PENUMBRA_WITH_NYX` on) —
`amanuensis`, `firefly`, `nyx-core`, `penumbra_nyx_bridge`, and `penumbra_demo_nyx` all build.

### What this unblocks

`pharos-proto`'s "Nyx-native application" window-bootstrap milestone (its own
`docs/next_steps.md`) is otherwise fully unblocked by the "Nyx bridge + entry point mechanism"
entry below — a standalone `pharos_nyx_bootstrap` executable was built and run successfully
against a hand-compiled-in-place `penumbra_nyx_bridge` (not this option), proving the mechanism
itself works end to end (a real window opened, titled "Penumbra Application", with the `.nyx`
script's own `OnStart`/`OnUpdate`/`OnShutdown` all confirmed firing via log output) — but that
proof was thrown away rather than kept, specifically because keeping it meant working around
this gap in `pharos-proto`'s own application code instead of fixing it here. Now that this
option's own build wiring is safe to enable, `pharos-proto` can flip `PENUMBRA_WITH_NYX` on
cleanly with no special-casing on its side.


## 7. `Application` base class + frame-loop ownership (2026-08-11)

### Fixed 2026-08-11: `Penumbra::Application` had no window/frame-loop ownership — only a narrow `IWidgetLifecycle::OnTick` dispatcher

> **Trigger:** `pharos-proto` wants to build a Nyx-driven `.nyx` application (its own
> "Nyx-native application" entry, `pharos-proto/docs/next_steps.md`) whose first milestone
> is a `.nyx` script owning window bootstrap itself, not just UI-tree composition inside an
> already-C++-owned window. Nyx's own inheritance-from-`Application` pattern (`class
> SnakeApplication : Application { ... }`, exercised throughout `nyx-proto/tests/
> host_test.cpp`) is designed for exactly this: a Nyx script's entry-point class inherits
> from a real C++ `Application` base and overrides its lifecycle hooks. `nyx-proto`'s own
> decision log (`docs/nyx-scripting-language/decision-log.md` §6.4/§6.5, both resolved)
> designed the mechanism and built the Nyx-side half — but explicitly assigns writing the
> actual base class + bridge specialization to "Umbra and Penumbra each provide one bridge
> per inheritable type — written once, in the integration layer, not in game code" (§6.5).
> Nobody has done that for Penumbra yet.

Confirmed directly against real source, not guessed:

- **`nyx-proto`'s side is built and tested.** `RegisterInheritableType<T>()` (a builder
  registering a C++ base type as Nyx-inheritable, with an `.Override(name, thunk)` per
  overridable method) and the two-part bridge mechanism (`NyxBridgeBase` + a per-type
  `NyxBridge<T>` specialization) are both real and exercised end-to-end in
  `nyx-proto/tests/host_test.cpp` via a stand-in `FakeEngineApplication`/
  `NyxBridge<FakeEngineApplication>` — Nyx scripts declaring `class SnakeApplication :
  Application { override void OnUpdate(float dt) { ... } }` are instantiated as real bridge
  objects, and Nyx-side super calls (`Application::Initialize()`) correctly reach the C++
  base through a qualified-call thunk (not a raw pointer-to-member, which the decision log
  notes was tried and recurses infinitely through virtual dispatch — see §6.4's "Corrected
  during implementation" note).
- **The decision log sketches a concrete hook set for the integration-layer base class**
  (`Engine::Application` in the sketch — a placeholder name, not a real Penumbra type):
  `Initialize()`, `OnUpdate(float dt)`, `OnRender(const IRenderer&)`,
  `Configure(ApplicationConfig&)`, `RegisterDependencies()`. This is a starting sketch from
  `nyx-proto`'s own design side, not a spec Penumbra is bound to — `IRenderer`/
  `ApplicationConfig` don't exist in Penumbra today, and the real hook set should be derived
  from what Penumbra's actual bootstrap/frame loop needs (below), not copied verbatim.
- **Penumbra already ships an unrelated `Penumbra::Application`**
  (`include/Penumbra/Application.h`/`src/Penumbra/Application.cpp`) — but it's a narrow
  per-frame `OnTick` dispatcher over a list of registered `IWidgetLifecycle*`
  (`RegisterLifecycle`/`UnregisterLifecycle`/`Tick`), with no window/platform ownership, no
  `Initialize`/`OnRender`/shutdown hooks, and zero `RegisterInheritableType`/`NyxBridge`
  wiring. Grepped every consumer in `penumbra-proto`, `penumbra-ui-backend`, and
  `pharos-proto` — it's currently referenced nowhere outside its own `.cpp`, i.e. unused
  today. This is a real naming collision to resolve deliberately (extend this type into the
  fuller lifecycle base this entry asks for, or introduce a differently-named one and decide
  how the two relate) — not something to guess at from a consuming repo.
- **What a real Penumbra `Application` base would need to own, grounded in `pharos-proto`'s
  actual bootstrap** (`pharos-proto/src/main.cpp`, the only real caller of this shape
  today): constructing `PlatformWindow` and calling `Initialise("Pharos", ...)`
  (`main.cpp:71-72`), constructing `Renderer` and calling
  `Initialise(window.GetSdlRenderer(), ...)` (`main.cpp:90`), and the per-frame loop ending
  in `EndFrameAndPresent()` (`main.cpp:339`) — currently all hand-rolled per-app C++, exactly
  the "we aren't rewriting it in every app" duplication this entry exists to close.

### What shipped

Resolved the naming collision by extending the existing `Penumbra::Application`
(`include/Penumbra/Application.h`/`src/Penumbra/Application.cpp`) in place, rather than
introducing a differently-named type — its `RegisterLifecycle`/`UnregisterLifecycle`/`Tick`
`IWidgetLifecycle` dispatch is kept unchanged and still fires once per frame, now
automatically from inside the new `Run()` loop, immediately before `OnUpdate` (preserving
its own doc comment's contract: lifecycle ticks fire before reconciliation/Measure/Arrange).

`Application` now owns `Platform::PlatformWindow`, `Render::Renderer`, and a
`Render::SdlTtfFontBackend` (the only real `IFontBackend` implementation today, same
ownership precedent as `PlatformWindow` owning "the only code that talks to SDL") as
private members, plus a new `ApplicationConfig` struct (`Title`/`WindowLogicalWidth`/
`WindowLogicalHeight`/`ClearColor`). `Run()` sequences `Configure(Config)` → window →
renderer/font-backend construction → `OnStart()` → the frame loop → `OnShutdown()`, mirroring
`demo/main.cpp`'s and `pharos-proto/src/main.cpp`'s hand-rolled shape exactly (both grounded
this design, per the trigger below) including the per-frame DPI-scale-change tracking both
of them duplicated by hand — now a single `OnDpiScaleChanged(float)` hook fired once when
`Window.GetDpiScaleFactor()` changes between frames, instead of copy-pasted inline in every
app's loop. Hook names landed exactly on the doc's own "naming TBD" sketch above:
`Configure(ApplicationConfig&)`, `OnStart()` (returns `bool`; `false` aborts `Run()` before
the frame loop starts, mirroring `PlatformWindow::Initialise`/`Renderer::Initialise`'s own
bool-failure convention), `OnUpdate(float DeltaSeconds)`, `OnRender(Render::Renderer&)`,
`OnShutdown()`. Protected accessors (`GetWindow`/`GetRenderer`/`GetFontBackend`/`GetInput`/
`GetConfig`) give overrides everything `main.cpp` closed over locally today. `RequestQuit()`
lets a subclass end the loop itself, alongside the existing OS-quit path
(`PumpEventsAndBuildInput` returning `false`).

Verified with a standalone scratch subclass (this repo's usual verification precedent, no
automated widget test suite) run under `SDL_VIDEODRIVER=dummy`: confirmed `Configure` →
window/renderer/font-backend construction → `OnStart` (with a live, non-null window and font
backend) → three frames of `Tick`+`OnUpdate`+`OnRender` firing 1:1 in order, with the
registered `IWidgetLifecycle`'s `OnMount`/`OnTick`/`OnUnmount` firing at the right points
(`OnMount` inside `OnStart`'s `RegisterLifecycle` call, `OnTick` once per frame before
`OnUpdate`, `OnUnmount` inside `OnShutdown`'s `UnregisterLifecycle` call) → `RequestQuit()`
from inside `OnUpdate` ending the loop → `OnShutdown` → `Run()` returning `0`. Library and
`penumbra_demo` (unchanged — this is purely additive to `Application`, not a `main.cpp`
migration) both stay clean under `cmake --build build`.

### What this unblocks

Any Penumbra app's `main.cpp` can now replace its hand-rolled window/renderer construction
and frame loop (`pharos-proto/src/main.cpp`'s and `demo/main.cpp`'s own shape, both grounded
this design) with a subclass of `Penumbra::Application` overriding `OnStart`/`OnUpdate`/
`OnRender`. This does **not** yet unblock `pharos-proto`'s Nyx-native-application milestone
by itself — that still needs the `nyx-proto` half (`RegisterInheritableType<Application>`
+ a `NyxBridge<Application>` specialization), deliberately deferred out of this pass (see
below) now that the C++ base shape it would bridge actually exists.

### Explicitly not requested

- ~~The `nyx-proto` `RegisterInheritableType`/`NyxBridge<Application>` wiring itself.~~ Done
  below (Fixed 2026-08-11: "Nyx bridge + entry point mechanism").
- **Migrating `demo/main.cpp` or `pharos-proto/src/main.cpp` to actually subclass the new
  base.** This entry ships the base class only; converting either app's existing hand-rolled
  `main.cpp` to use it is separate follow-up work, not required for the base class itself to
  be real and buildable.
- **A full engine dependency-injection system.** The decision log's sketch hook
  `RegisterDependencies()` is noted for completeness, not requested here — scope this entry
  to bootstrap/frame-loop ownership and the inheritance wiring, not a broader DI framework.
- **Migrating existing widget-lifecycle dispatch (`IWidgetLifecycle`/the existing
  `Application::Tick`) to something new.** Kept exactly as it was — `Application` now just
  calls `Tick` for the caller automatically instead of requiring the frame-loop owner to call
  it by hand.


## 8. Nyx bridge + entry point mechanism (2026-08-11)

### Fixed 2026-08-11: Nyx bridge + entry point mechanism — `Penumbra::Application` still wasn't actually inheritable from a `.nyx` script, and every app hand-rolled its own `main()`

> **Trigger:** direct follow-up request, once the base class above existed: wire up
> `nyx-proto`'s `RegisterInheritableType`/`NyxBridge` mechanism for real (this repo had
> deliberately deferred it, `PENUMBRA_WITH_NYX` and the dependency-cost question above), and
> give Penumbra a "simple entry point mechanism" analogous to a Hazel-style engine's
> `engine/entry-point.hpp` (`extern IApplication* CreateApplication(); int main() {...}`) —
> explicitly asked to "work the nyx bridge into this pattern somehow."

Confirmed directly against `nyx-proto`'s real source before writing anything: the `Override`
thunk `nyx-proto/src/host/inheritable-type-builder.hpp` generates is a **free function**
(`Ret (*)(T&, Args...)`, converted from a non-capturing lambda via `+[]`), not a member of
`T`'s own hierarchy — `nyx-proto`'s own `FakeEngineApplication` test stand-in makes
`Initialize`/`OnUpdate` `public` for exactly this reason. A protected hook (what
`Application`'s hooks were, from the entry above) cannot be reached from a plain
`self.Application::Method()` qualified call written outside the class, so bridging is
impossible without a visibility change. Also confirmed `nyx-proto/src/host/marshal.hpp`
marshals primitives/`void` only (`bool`/`int32`/`int64`/`float`/`double`/`std::string`) — no
overload exists for an arbitrary C++ reference like `Render::Renderer&` or
`ApplicationConfig&`, so those two hooks structurally cannot be bridged without a
`marshal.hpp` change (a `nyx-proto` change, out of scope here).

### What shipped

**Visibility fix:** `OnStart`/`OnUpdate`/`OnShutdown`/`OnDpiScaleChanged` moved from
`protected` to `public` on `Application` (`include/Penumbra/Application.h`) — exactly the
four hooks bridged below, and exactly why: the other two hooks, `Configure` and `OnRender`,
stay `protected` since they're not bridgeable (see above) and have no other reason to be
public.

**The bridge itself**, `include/Penumbra/Nyx/ApplicationBridge.h` /
`src/Penumbra/Nyx/ApplicationBridge.cpp`: a `template<> nyx::host::NyxBridge<Penumbra::
Application>` specialization (defined in the `.cpp`, not the header — nothing outside this
file needs to name it directly) overriding those same four hooks, each `Invoke`-ing the
Nyx-side override when one exists and falling back to `Application`'s real C++ default via a
qualified super call otherwise — the identical pattern `nyx-proto`'s own
`NyxBridge<FakeEngineApplication>` test stand-in already proved generically, now written for
real against Penumbra's own type. `Penumbra::Nyx::LoadApplication(source, filename,
className)` / `LoadApplicationFromFile(path, className)` wrap
`RegisterInheritableType<Application>("Application").Override(...)` (registered once, guarded
by a `bool`, not re-registered on repeat calls) plus `NyxRuntime::MountBridged<Application>`
into a single call returning a plain `Application*` — catching `LexError`/`ParseError`/
`RuntimeError` (confirmed all three derive from `std::runtime_error`) and returning `nullptr`
on failure instead of letting them propagate as C++ exceptions, matching Penumbra's existing
bool/nullptr-return error convention (`PlatformWindow::Initialise`,
`Renderer::Initialise`) rather than `nyx-proto`'s own throw-based one. The returned
`Application*`'s backing `NyxRuntime`/`Interpreter` are kept alive in a static, process-lifetime
registry internal to the `.cpp` (documented as such) — correct for this factory's own stated
contract ("call once, from `main()`, for the app's whole lifetime"); a caller wanting a
shorter-lived or repeated-mount scope should drive `NyxRuntime`/`MountBridged` directly
instead, using the same public `NyxBridge<Application>` specialization... except it isn't
public (see above) — noted as a real limitation below, not silently glossed over.

**The entry point**, `include/Penumbra/EntryPoint.h`: `extern Penumbra::Application*
CreateApplication();` plus a real `int main()` that calls it, runs `Run()`, and deletes the
result — a direct match for `snake/src/engine/entry-point.hpp`'s own shape. Needs no
Nyx-specific case at all: `CreateApplication()` returning a plain C++ `Application` subclass
or an `Application*` obtained from `Penumbra::Nyx::LoadApplication` are indistinguishable to
`main()`, since `Run()` only ever calls virtual hooks — this is the concrete payoff of the
"work the Nyx bridge into this pattern" ask, not a bolted-on second path.

**Build wiring:** a new `PENUMBRA_WITH_NYX` CMake option (default `OFF`) gates a
`FetchContent`-populated-source-only `nyx-core` + `firefly` build (mirroring
`pharos-proto/cmake/Dependencies.cmake`'s own nyx-proto dependency, simplified since this repo
has no `iris`/`amanuensis` target already declared to collide with — `add_subdirectory`ing
`firefly` directly works here, no hand-copied source-file-list workaround needed) and a new
`penumbra_nyx_bridge` static library target. Off by default: the main `penumbra` library and
`penumbra_demo` have zero dependency on `nyx-proto`, unchanged.

Verified against the real `nyx-proto` repo (network-fetched, not stubbed) with
`-DPENUMBRA_WITH_NYX=ON`: a standalone scratch program (this repo's usual verification
precedent) called `Penumbra::Nyx::LoadApplication` against four real `.nyx` sources and
called the resulting `Application*`'s hooks directly (no window/SDL needed — `OnStart` etc.
are pure dispatch, same reasoning as `Box`'s own `Measure`/`Arrange` verification not needing
a window). All four passed: a script overriding `OnStart` to return `false` — confirmed the
override is reached, not silently ignored; a script with no overrides — confirmed the C++
default (`true`) is reached via fallback; a script explicitly calling
`Application::OnStart()` as a super call before returning `false` — confirmed the qualified
super-call thunk reaches the real base without infinitely recursing back into the Nyx
override (the exact failure mode `nyx-proto`'s own decision log §6.4 documents fixing); and a
class not extending `"Application"` — confirmed `LoadApplication` returns `nullptr` with a
diagnostic on `stderr`, not a crash or an uncaught exception. Default (`PENUMBRA_WITH_NYX`
unset) and opt-in builds both verified clean under `cmake --build`.

**Follow-up, same day:** added a real `penumbra_demo_nyx` executable (`demo_nyx/main.cpp` +
`demo_nyx/assets/DemoApp.nyx`, also `PENUMBRA_WITH_NYX`-gated) as a standing, runnable
end-to-end proof, not just the throwaway scratch program above — `DemoApplication` is defined
entirely in `DemoApp.nyx`, not C++, overriding `OnStart`/`OnUpdate`/`OnShutdown`; its C++ host
(`demo_nyx/main.cpp`) only registers two callbacks (`Log`, `Tick`) via a new
`Penumbra::Nyx::GetRuntime()` accessor (exposes the same process-lifetime `NyxRuntime`
`LoadApplication` uses internally) and loads the script — window construction and the frame
loop are entirely `Penumbra::Application::Run()`'s. Verified two ways: headless
(`SDL_VIDEODRIVER=dummy`) showing `OnStart`/periodic `OnUpdate`/`OnShutdown` all firing from
the real script, plus the `Tick` callback (invoked *from* the Nyx script every frame) calling
`RequestQuit()` on the live `Application*` after 300 frames to end the run — the concrete
illustration of "a Nyx script can't call `RequestQuit()` directly, but a host callback it
calls into, can." Then re-run with a real (non-dummy) SDL video driver — an actual window
opened on screen, ran for ~5 seconds at real display timing (`dt` in the 0.006–0.011s range,
vs. the dummy driver's near-zero), and auto-closed cleanly, exit code 0.

### What this unblocks

`pharos-proto`'s planned `.nyx`-driven application (a Nyx script owning its own window, the
first milestone of its "Nyx-native application" effort) is now genuinely buildable for the
lifecycle-hook half of that story: a Nyx `class PharosApplication : Application { ... }`
overriding `OnStart`/`OnUpdate`/`OnShutdown`/`OnDpiScaleChanged`, loaded via
`Penumbra::Nyx::LoadApplication` and driven through `Penumbra/EntryPoint.h`'s `main()`
exactly like any plain C++ app. Any Penumbra app (Nyx-driven or not) can also now drop its
own hand-rolled `main()` in favor of `#include "Penumbra/EntryPoint.h"` + one
`CreateApplication()` definition.

### Explicitly not requested

- **Bridging `Configure(ApplicationConfig&)` or `OnRender(Render::Renderer&)`.** Structurally
  blocked on `nyx-proto/src/host/marshal.hpp` only marshalling primitives/`void` today — a
  `nyx-proto` change, not a `penumbra-proto` one. A Nyx-driven `Application` subclass
  therefore cannot yet control the window title/size/clear color from script, or draw
  anything from script — `OnRender`'s C++ default (a no-op) always runs unless a cooperating
  C++-side mechanism (not built here) supplies one. This is a real, load-bearing gap for
  `pharos-proto`'s actual "own window bootstrap" milestone, left for whoever picks that up
  next to design deliberately rather than guessed at here.
- ~~Exposing `nyx::host::NyxBridge<Application>` publicly, or any way for a caller to
  register extra Nyx-callable host functions.~~ Partially done in the same-day follow-up
  above: `Penumbra::Nyx::GetRuntime()` exposes the shared `NyxRuntime` for
  `RegisterFunction`/`RegisterType` calls (what `demo_nyx` needed for `Log`/`Tick`). The
  bridge specialization itself (`NyxBridge<Application>`) is still `.cpp`-local — a consumer
  needing a shorter-lived or repeated mount (rather than this module's single
  process-lifetime `NyxRuntime`) still needs to re-declare it, unchanged from before.
- **Migrating `demo/main.cpp` or `pharos-proto/src/main.cpp` to `Penumbra/EntryPoint.h`.**
  Same scoping as the base-class entry above — this ships the mechanism, not a migration of
  either existing app onto it.


## 9. `Box` `justify-content` (2026-08-10)

### Fixed 2026-08-10: `Box` had no main-axis space-distribution (`justify-content`) concept — only sequential packing from the start

> **Trigger:** `pharos-proto`'s `src/ui/layout_helpers.h` has a hand-rolled
> `ThreeZoneRow : public WidgetBase` composite ("Left"/"Center"/"Right" independently
> positioned in one row) specifically because `Box`'s `HorizontalStack` only packs
> children sequentially from the content start — no `space-between`/center/end-anchored
> distribution of the group as a whole. This was noted in this repo's own
> `LayoutMode::FixedLeadingStack` entry above ("What this unblocks") as a *different*,
> still-unimplemented capability, and `lustre` shipped the CSS-*parsing* side
> (`Lustre::Justify`/`ResolvedStyle::JustifyContent`) on 2026-08-03 — but, like the
> `FixedLeadingStack` gap before it, was never actually filed here as its own entry
> (this doc showed "None open right now" the whole time). `pharos-proto` asked directly
> for this to be resolved, not just re-filed.

Confirmed directly in this repo's own `src/Penumbra/Widgets/Box.cpp`'s `VerticalStack`/
`HorizontalStack` branch of `Arrange` (the *ordinary* stack path, not `FixedLeadingStack` —
that layout's two slots are sized directly by `LeadingExtentLogical`/the remainder, not
distributed, and `JustifyContentMode` is correctly not consulted there): a single `Cursor`
starts at `Content`'s main-axis origin and advances by each child's own measured extent
plus `ChildGap`, with no concept of the *total* free space left over once every child is
placed — every child ends up packed against the previous one starting from the content's
own start edge, with no way to center the group, anchor it to the far edge, or spread the
leftover room as extra gaps between siblings.

### What shipped

New `enum class Justify { Start, Center, End, SpaceBetween }` (`Styles.h`, alongside
`CrossAlign` — same four keyword values `lustre::Justify` already ships, deliberately no
`SpaceAround`/`SpaceEvenly`, matching `lustre`'s own "not requested by any real consumer"
scoping) and `Box::JustifyContentMode{Justify::Start}` (`Box.h`). `Start` (the default)
needed no algorithm change at all — it's byte-for-byte the sequential-packing behavior
every `Box` already had, so every existing caller is unaffected without touching a single
call site.

`Box::Arrange`'s `VerticalStack`/`HorizontalStack` branch: when `JustifyContentMode !=
Justify::Start`, a first pass measures every visible child to sum their total main-axis
extent (`Desired` + margin), computes `FreeSpace = ContentMain - TotalChildrenMain -
TotalGaps` (clamped to non-negative — an overflowing group falls back to plain sequential
packing with no negative offset/gap), then derives a `StartOffset` (`Center`: half the free
space; `End`: all of it) and/or an `ExtraGapPerGap` (`SpaceBetween`: free space split evenly
across the *N-1* gaps between visible children, added on top of `ChildGap` rather than
replacing it — matches modern CSS's `gap`+`justify-content: space-between` interaction, not
the pre-`gap` behavior where `space-between` owned 100% of the distribution). The existing
per-child loop is otherwise untouched: `Cursor` just starts at `ContentStart + StartOffset`
instead of `ContentStart`, and each inter-child advance becomes `ChildGap + ExtraGapPerGap`
instead of `ChildGap` alone. `Box::Measure` needed no change — `JustifyContentMode` only
affects how a `Box`'s *own already-given* final rect is subdivided among children, not the
`Box`'s own reported desired size.

Verified with a standalone scratch program (this repo has no automated widget test suite,
same precedent as `FixedLeadingStack`'s own verification above) linking directly against
`libpenumbra.a` — no window/SDL context needed, since `Measure`/`Arrange` are pure geometry,
untouched by `Draw`. Six cases, asserting exact `ArrangedRect` values against hand-computed
expected numbers (not just "looks right"): `SpaceBetween` with 3 fixed-width children in a
400-wide row (`left.X=0`, `center.X=175`, `right.X=350` — exactly `ThreeZoneRow`'s own
shape); `SpaceBetween` with a single child (falls back to `Start`, `x=0` — nothing to space
between); `Center` on `VerticalStack` with a `ChildGap` (confirms the gap and the
distribution offset compose correctly, not one overriding the other); `End` on
`HorizontalStack`; `Start` left at its default (confirms zero change from pre-existing
behavior); and `SpaceBetween` with children wider than the container (confirms `FreeSpace`
clamps to zero rather than going negative). All six passed. `cmake --build build` (library +
`penumbra_demo`) stays clean, zero new warnings.

### What this unblocks

`pharos-proto`'s `ThreeZoneRow` (`src/ui/layout_helpers.h`) is now replaceable by an
ordinary `Box`/`Frame` with `Layout = HorizontalStack; JustifyContentMode =
Justify::SpaceBetween` — closing the second (and last) of the two structural blockers on
`DropdownTrigger`'s `<Native>` migration (`FixedLeadingStack`, above, already closed the
`ViewportWidget` half). Unlike `FixedLeadingStack`, this one *is* reachable from `.iris`/
`.lustre` once `penumbra-ui-backend`'s `StyleApplier.cpp` maps `ResolvedStyle::
JustifyContent` onto this new field (`lustre`'s own `justify-content` CSS-parsing already
ships, cross-referenced in that repo's own docs — the missing half was purely this
`Box`-algorithm gap plus the `StyleApplier` mapping, not anything left in `lustre`).

### Explicitly not requested

- **`SpaceAround`/`SpaceEvenly`.** Not requested by any real consumer — `justify-content`'s
  four keyword values (matching `lustre::Justify` exactly) are the concrete ask, not a full
  flexbox-equivalent distribution model, same scoping precedent as `FixedLeadingStack`'s own
  "explicitly not requested" section above.
- **`JustifyContentMode` support in `FixedLeadingStack`.** That layout's two slots are
  sized directly by `LeadingExtentLogical`/the remainder — there's no "free space" concept
  left to distribute once both slots are already fully accounted for, so this field is
  simply not consulted there (confirmed unreachable by that branch's own code path).


## 10. `Box` `FixedLeadingStack` (2026-08-03)

### Fixed 2026-08-03: `Box`'s stack layout offered every child the same full available size, rather than shrinking it per sibling

> **Trigger:** `pharos-proto`'s `src/ui/layout_helpers.h` has a hand-rolled
> `FixedLeadingStrip : public WidgetBase` composite ("fixed-height leading child +
> fill-remainder child") specifically because a plain `Box` with `VerticalStack`/
> `HorizontalStack` can't host a fixed-size sibling next to a greedy one (a `SplitPanel`
> or `ViewportWidget`, which reports its own desired size as "whatever my parent gives
> me") without double-counting space. This was scoped in `iris-proto`'s
> `docs/next-steps.md` ("No layout-container primitive beyond Frame's three stack modes",
> 2026-07-30) as *not* an Iris-grammar question — `FixedLeadingStrip`'s "don't shrink
> either child" contract is "a `Box`-layout-algorithm question in `penumbra-proto`, not a
> `Frame`-prop or Iris-grammar one" — but it was never actually filed here, which
> currently showed "None open right now."

Confirmed directly in this repo's own `src/Penumbra/Widgets/Box.cpp`: both `Box::Measure`
(line 104) and `Box::Arrange` (line 169) compute one `ContentAvailable`/`Content` rect for
the whole `Box` and then call `Child->Measure(ChildAvailable)` for **every** child using
that *same* full content extent minus only that child's own margin (`Box.cpp:139-140`
inside `Measure`, `Box.cpp:207-208` inside `Arrange` — both shrink only by
`Margin.Left/Right`/`Top/Bottom`, never by a preceding sibling's already-placed extent).
`Arrange`'s `Cursor` (line 197) does advance past each child's *desired* size once placed
(line 220/226), so children don't visually overlap after layout — but each child's own
`Measure()` call was already handed the full, undiminished container size to measure
*against*, before any sibling's space was subtracted. For an intrinsically-sized child
(a `Label`, a fixed-height header) this is harmless: it reports its own natural size
regardless of how much room it's offered. For a **greedy** child that reports "fill
whatever I'm given" as its desired size (a `SplitPanel`/`ViewportWidget` — see
`FixedLeadingStrip`'s own doc comment), it means that child's *measured* desired extent is
the full container size, identical to what a fixed-size sibling in the same stack would
also be entitled to request — there is no per-sibling partitioning at the `Box::Measure`
level at all, only sequential *placement* after the fact in `Arrange`. Mixing a fixed-size
leading child with a greedy fill child in one ordinary `VerticalStack` therefore either
overflows the `Box`'s own reported size (if the greedy child's desired size is taken at
face value) or requires the caller to already know each child's real size up front and
work around `Box` entirely — which is exactly what `FixedLeadingStrip` does today, via a
hand-written `WidgetBase` override.

### What shipped

Went with the first of the two proposed directions — a new `LayoutMode::FixedLeadingStack`
value plus a companion `Box::LeadingExtentLogical` field (`Styles.h`/`Box.h`) — not the
"generalize the per-child loop to shrink by running `Cursor`" alternative, since the real
`FixedLeadingStrip` this generalizes doesn't measure its leading child to decide its
height at all: it hands `Leading` exactly `LeadingHeightLogical` regardless of what
`Leading->Measure()` would naturally report (confirmed directly in `pharos-proto`'s
`src/ui/layout_helpers.cpp` before implementing — `Leading->Measure({..., LeadingHeightLogical})`
discards the returned `Point` entirely), so an explicit override field matches real usage,
not a guess.

`Box::Measure` (`FixedLeadingStack` branch): reports the full `ContentAvailable` back as
its own desired size, greedy like `FixedLeadingStrip::Measure`'s own "return
`AvailableSizeLogical` unchanged" contract — not the sum of its children's sizes the way
`VerticalStack` does. Still calls `children[0]->Measure()` with exactly
`LeadingExtentLogical` and every child after it with whatever's left once
`LeadingExtentLogical + ChildGap` are subtracted, so nested widgets update whatever they
cache there; the returned `Point` itself is only used later, in `Arrange`, for cross-axis
placement.

`Box::Arrange`: partitions `Content` into a Leading rect (exactly `LeadingExtentLogical`
tall, clamped to `Content.H`) and a Fill rect (the remainder after `ChildGap`), mirroring
`FixedLeadingStrip::Arrange` exactly — neither slot is sized by the child's own measured
main-axis extent, only the cross axis (width) still goes through the existing
`PlaceCross`/`CrossAlignment` machinery, reused unchanged. Generalized slightly beyond
`FixedLeadingStrip`'s hard two-child (`Leading`/`Fill`) model: `children[0]` is always
Leading, and *every* child after it shares the identical Fill rect — a third child would
overlap a second one, an accepted footgun for a case with no real use today, matching
`Box::Arrange`'s own existing precedent language ("children, if any, are not laid out
(footgun accepted)" for `LayoutMode::None`).

Verified with a standalone scratch program (this repo has no automated widget test suite,
only `demo/`) rendering a real window: a `Box` with `Layout = FixedLeadingStack`,
`LeadingExtentLogical = 60`, `ChildGap = 10`, two children — confirmed both the exact
arranged rects (`Leading`: y=0 h=60; `Fill`: y=70 h=230, full width on both, for a 400×300
window) and a real screenshot showing a fixed-height leading bar above a fill area with a
visible gap between them, matching `FixedLeadingStrip`'s intended appearance exactly.
`cmake --build build` (library + `penumbra_demo`) stays clean.

### What this unblocks

`pharos-proto`'s `FixedLeadingStrip` (`src/ui/layout_helpers.h`) is now replaceable by an
ordinary `Box`/`Frame` with `Layout = LayoutMode::FixedLeadingStack` set directly — no
Lustre/`StyleApplier` property needed first, since `pharos-proto` already has precedent
for setting a `Box` field directly in C++ after building via `.iris` for properties Lustre
can't reach yet (`SplitPanelStyle`'s handle colors, `TextInput::ColorText`). This unblocks
the `ViewportWidget` treemap's `<Native>` migration specifically (blocked on
`FixedLeadingStrip`'s parent, `strip`, per `pharos-proto/docs/next_steps.md`'s "Open
items"). It does **not** unblock `DropdownTrigger` — that one's blocked on `ThreeZoneRow`,
which needs actual main-axis space-distribution (`justify-content`) in `Box`, a different,
still-unimplemented capability (`lustre` only shipped the CSS-parsing side of that so
far, cross-referenced in `lustre`'s and this repo's own docs, not touched by this entry).

### Explicitly not requested

- **A general CSS-flexbox `flex-grow`/`flex-shrink` sizing model.** The concrete ask is
  narrower: exactly one leading child gets its natural size, everything after it shares
  the remainder. No known consumer needs arbitrary per-child grow/shrink ratios.
- **A `Disabled` gradient variant.** No known consumer wants one; adding it
  speculatively would be unused surface. Revisit only if a real consumer
  needs it.
