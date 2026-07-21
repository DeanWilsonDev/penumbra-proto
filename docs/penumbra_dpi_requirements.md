# Penumbra — Display/DPI-Change Requirements for Pharos

> **Scope:** A capability gap found after the initial ImGui→Penumbra port
> (`docs/penumbra_requirements.md`) landed and the real panels were built and
> tested against `penumbra` commit `6f2509f`.
> **Status:** Blocking a real, reported bug: moving the Pharos window to a
> different display doesn't rescale correctly.
> **Not in scope:** anything achievable in Pharos's own code. This is a
> genuine dead end — `PlatformWindow`/`Renderer` give a consumer no way to
> pick up a changed DPI scale after startup, and Pharos isn't allowed to call
> SDL directly to work around that.

---

## 0. Context

**Symptom:** dragging the Pharos window from one display to another (different
resolution/pixel density) doesn't make the app "resize" correctly — the
rendered content stops matching the window once it's on the new display.

**Root cause**, traced through `penumbra`'s current source:

1. `PlatformWindow::DpiScaleFactor` is queried exactly once, in `Initialise()`,
   via `SDL_GetWindowDisplayScale(Window)` (`PlatformWindow.cpp:56`), and
   cached in a private field. `GetDpiScaleFactor()` just returns that cached
   value (`PlatformWindow.cpp:142–144`) — it never re-queries.
2. `PumpEventsAndBuildInput`'s event loop only handles `SDL_EVENT_QUIT`,
   `SDL_EVENT_MOUSE_WHEEL`, `SDL_EVENT_TEXT_INPUT`, `SDL_EVENT_KEY_DOWN` — SDL's
   display-scale-change notification (`SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED`)
   is never observed, so there's no event-driven trigger to refresh it either.
3. `Renderer::DpiScaleFactor` is a *second*, independent copy — passed once
   into `Renderer::Initialise` at startup (`Renderer.cpp:74`) and cached in
   its own private field. Even if (1) were fixed, `Renderer` has no method to
   receive a new value; every `Draw*` call keeps multiplying logical
   coordinates by the stale factor (`Renderer.cpp:82–84, 99, 152–153, 242–243`).
4. `SdlTtfFontBackend::LoadFont` bakes the DPI scale into the *rasterized
   glyph texture* at load time — `TTF_OpenFont(Path, PointSizeLogical *
   DpiScaleFactor)` (`SdlTtfFontBackend.cpp:51–52`). A `FontHandle` obtained
   at one DPI stays rasterized for that DPI forever; there's no
   reload/invalidate path tied to a DPI change.

All three layers cache a value SDL itself treats as dynamic (it changes the
moment a window crosses onto a different-density display), so fixing only one
layer isn't enough: (2)/(3) fixed without (1) has nothing correct to read
from; (1) fixed without (3) has nowhere to deliver the new value; all three
fixed without reloading fonts leaves text the wrong physical size (soft when
upscaled, over-sharp when downscaled) even once the *layout* is correct.

Pharos cannot work around this in its own code: `PlatformWindow`/`Renderer`
are the *only* legal way for it to learn or apply a DPI scale (the
architecture's own rule — "the only code that calls SDL directly is
`Penumbra::Platform`") — and neither currently exposes a live value or an
update path.

---

## 1. REQUIRED — a live DPI scale query

`PlatformWindow::GetDpiScaleFactor()` should reflect the *current* display the
window is on, not the display it was created on.

### Proposed fix

No signature change needed — this is a behavior fix to the existing method:

```cpp
// src/Penumbra/Platform/PlatformWindow.cpp
float PlatformWindow::GetDpiScaleFactor() const {
    const float Live = SDL_GetWindowDisplayScale(Window);
    return (Live > 0.0f) ? Live : DpiScaleFactor; // fall back to the cached value if the query fails
}
```

`SDL_GetWindowDisplayScale` is a cheap OS query — `Initialise` already calls
it without any cost concern. Calling it once a frame, the same cadence
`GetLogicalWindowSize()` is already called at, is negligible.

(An event-driven alternative — watching for
`SDL_EVENT_WINDOW_DISPLAY_SCALE_CHANGED` in `PumpEventsAndBuildInput` and
refreshing the cached field only then — works too, and avoids a per-frame
syscall. This doc doesn't have a strong preference between the two; what
matters is that `GetDpiScaleFactor()` stops being permanently stuck at the
startup value.)

---

## 2. REQUIRED — a way to update `Renderer`'s DPI scale after `Initialise`

`Renderer::DpiScaleFactor` has no setter; once constructed, a `Renderer` can
never apply a new scale, even once the caller is able to obtain one correctly
via item 1.

### Proposed API

```cpp
// include/Penumbra/Render/Renderer.h

// Updates the scale factor used to convert logical -> physical pixels for
// every subsequent Draw* call. Call once per frame with the platform's
// current value (e.g. from PlatformWindow::GetDpiScaleFactor()) -- cheap to
// call even when the value hasn't changed.
void SetDpiScaleFactor(float NewScaleFactor);
```

```cpp
// src/Penumbra/Render/Renderer.cpp
void Renderer::SetDpiScaleFactor(float NewScaleFactor) {
    DpiScaleFactor = (NewScaleFactor > 0.0f) ? NewScaleFactor : DpiScaleFactor;
}
```

### What unblocks in Pharos

`src/main.cpp`'s frame loop adds one line before `BeginFrame`:

```cpp
renderer.SetDpiScaleFactor(window.GetDpiScaleFactor());
```

---

## 3. Not a new API — but flagging so it isn't missed: fonts need reloading too

Even with items 1–2 landed, existing `FontHandle`s stay rasterized at the OLD
DPI — text would be the correct logical size/position but visibly soft or
over-sharp after a cross-display move, until the app reloads its fonts.

Pharos can detect the change itself once item 1 makes `GetDpiScaleFactor()`
live (compare against a "last known" value once a frame — no new API needed
for detection) and re-call `IFontBackend::LoadFont` for each of its fonts when
it changes; `LoadFont` already returns a fresh `FontHandle`. This is a
Pharos-side loop over its own (two) fonts, not a Penumbra ask — listed here
only so the fix isn't considered "done" after items 1–2 land when text would
still look wrong.

---

## 4. Explicitly not requested

- **Automatic font-handle invalidation/reload inside Penumbra itself** (e.g.
  `Renderer` transparently re-rasterizing every known `FontHandle` on a DPI
  change). Pharos owns very few fonts; reloading them itself in response to a
  live `GetDpiScaleFactor()` value (item 3) is simple enough that asking
  Penumbra to own this is unnecessary machinery for a PoC-stage widget set.
- **Multi-window / per-monitor DPI tracking** beyond the single primary window
  `PlatformWindow` already scopes itself to.

---

## 5. What unblocks when this lands

Once items 1–2 exist, dragging the Pharos window between displays of
different pixel density renders at the correct scale immediately; adding the
font-reload loop described in item 3 (Pharos-side, no further Penumbra change
needed) finishes it by keeping text crisp too.
