# Penumbra — Next Steps

> Living backlog, not a session-changelog: add an entry for anything new
> and open; remove or mark an entry done the moment it's actually
> implemented, rather than letting finished work linger. First file of
> this kind in this repo (previously used `docs/*_requirements.md`, one
> file per investigation, still present as historical record — not
> migrated into here retroactively).
> Last updated: 2026-08-14.

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

## Open items

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

### A Nyx-loaded `Application*` has no way to read the current window size — every frame's Measure/Arrange is stuck on a hardcoded guess

**Status:** open, found 2026-08-17 while fixing a `pharos-proto` bug report (window resize doing
nothing visible; text bleeding out of the Inspector panel).

`Application` (`include/Penumbra/Application.h`) exposes `GetFontBackend()`, `GetDpiScaleFactor()`,
and `SetTextInputActive(bool)` as public members specifically so a caller that only holds an
`Application*` from `Penumbra::Nyx::LoadApplication`/`LoadApplicationFromFile` — no subclassing
point, since the Nyx script itself is the subclass — can still reach them (each one's own doc
comment says as much: "no subclassing point, but still needs X"). `GetWindow()`/`GetRenderer()`,
which is where the live window size actually lives (`Platform::PlatformWindow`), stay `protected`
(lines 155–156) with no counterpart public accessor — there's no `GetWindowSize()`/
`GetWindowLogicalSize()` alongside the three that already exist for exactly this reason.

`pharos-proto/src/nyx_app/main.cpp` is exactly this kind of caller (`GApplication` is the
`Application*` returned from `LoadApplicationFromFile`, held in a free function, no subclass).
Its `updateWidgetTree()` calls `GOverlayHost->Measure(...)`/`Arrange(...)` every frame against a
`constexpr Penumbra::Rect kWindowRect{0.0f, 0.0f, 1280.0f, 720.0f}` — a compile-time guess, not a
live query — because there's genuinely nothing else it can call. The real `pharos` binary (the
hand-rolled, non-`Application`, non-Nyx `src/main.cpp`) doesn't have this problem: it owns its
`PlatformWindow` directly and calls `window.GetLogicalWindowSize()` every frame, so it already
resizes correctly. Confirmed by reproducing both: built and ran `pharos_nyx_bootstrap`, resized
its OS window from 1280×752 to 1443×820 via System Events, and the mounted tree stayed pinned at
its original 1280×720 arrangement — the extra space just renders as empty background, nothing
reflows into it.

Fix looks like the same shape as the three existing accessors: a public
`Point GetWindowLogicalSize() const` (or similar) on `Application`, forwarding to
`Window.GetLogicalWindowSize()` the same way `GetDpiScaleFactor()` forwards to the Renderer's own
scale factor. Once that exists, `pharos_nyx_bootstrap`'s `main.cpp` can replace `kWindowRect` with
a live per-frame query and the resize bug goes away on the Pharos side with no further Penumbra
work — noted here rather than worked around with a raw SDL call or a reach into `GetWindow()`,
per this ecosystem's own "ask the dependency, don't hack around it" rule.
