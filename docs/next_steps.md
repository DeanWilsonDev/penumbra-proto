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
