# Penumbra — Theming/Polish Requirements for Pharos

> **Scope:** Gaps found pushing a "modern/sleek" theme through Pharos's
> existing panels, tested against `penumbra` commit `fd2500c`. No new
> widgets involved — this is about how far the *styling* surface goes.
> **Status:** Not blocking. Pharos shipped a theme pass (deeper near-black
> surfaces, rounded buttons/inputs/checkboxes/row-highlights, a pill-shaped
> primary button, a left accent bar on the selected tree row) entirely within
> what Penumbra already exposes. These are the things that pass ran into.

---

## 1. REQUIRED — `SplitPanel` should honour its children's `Margin`

`Box::Arrange` already respects a stack child's `Style.Margin` (used for
ordinary spacing everywhere in Pharos). `SplitPanel::Arrange` doesn't — it
hands `First`/`Second` their full computed rect directly with no margin
consulted. That means there's no way to get a "floating card" layout (a
visible gap between panels, background peeking through) without a
Penumbra-side change: Pharos's panels currently tile edge-to-edge, and
rounding their *outer* corners in that arrangement would just look like a
rendering glitch (a rounded corner revealing a stray triangle of whatever's
behind the SplitPanel, with no consistent gap around it) rather than an
intentional gap — which is why this theme pass left panel-level corners
square and only rounded the interactive elements inside them.

### Proposed fix

```cpp
// src/Penumbra/Widgets/SplitPanel.cpp, in Arrange, before First/Second get their rects
const EdgeInsets FirstMargin  = First  ? First->GetMarginLogical()  : EdgeInsets{};
const EdgeInsets SecondMargin = Second ? Second->GetMarginLogical() : EdgeInsets{};
// inset FirstRect/SecondRect by their respective margins, same shape as
// Box::Arrange's existing Margin handling for stack children
```

### What unblocks in Pharos

Panels get `Style.Margin` + a real `BorderRadius`, producing a genuine
floating-card look instead of edge-to-edge square tiles.

---

## 2. Minor — `Checkbox`'s checked-state fill ignores its own `BorderRadius`

`Checkbox::DrawContent` draws the checked-state fill and the inner checkmark
via `Renderer.DrawFilledRect(ContentRect, ColorBoxChecked)` /
`DrawFilledRect(Mark, ColorCheckMark)` — neither call passes a corner radius,
so they're always square, even though `Box::Draw` (the unchecked border/
background) *does* honour `Style.BorderRadius`. The result: an unchecked
Checkbox is a rounded outline, but the instant it's checked the fill snaps to
square corners.

### Proposed fix

```cpp
// src/Penumbra/Widgets/Checkbox.cpp
Renderer.DrawFilledRect(ContentRect, ColorBoxChecked, Style.BorderRadius);
...
Renderer.DrawFilledRect(Mark, ColorCheckMark, Style.BorderRadius * 0.6f); // inner mark, smaller radius
```

---

## 3. REQUIRED — gradient fills

Every "modern" reference UI this theme pass drew from (Linear, Raycast,
Vercel) leans on subtle 2-stop gradients for primary buttons and accent
surfaces. `DrawFilledRect`/`DrawRectOutline` only take one solid `Color`, so
Pharos's pill-shaped Load button is a flat fill instead of the soft vertical
sheen a "sleek" button usually has.

This doesn't need a new primitive from scratch — `DrawFilledRect`'s rounded-
corner path already tessellates the rect into a vertex fan with a per-vertex
`SDL_FColor` (`Renderer.cpp`, `BuildRoundedRing` / the `Vertices` fan in
`DrawFilledRect`), currently all set to the same solid color. A gradient is
the same geometry with each vertex's color interpolated by its position.

### Proposed API

```cpp
// include/Penumbra/Render/Renderer.h

// A 2-stop vertical gradient (TopColor at the rect's top edge, BottomColor at
// its bottom edge), otherwise identical to DrawFilledRect -- same rounded-
// corner handling, same anti-aliased edge. CornerRadiusLogical 0 draws a
// gradient-filled quad via SDL_RenderGeometry (4 vertices, one flat quad)
// instead of the SDL_RenderFillRect fast path DrawFilledRect uses, since that
// fast path has no per-vertex color; a positive radius reuses the existing
// rounded-rect tessellation with each vertex's color lerped by its
// normalised Y position.
void DrawGradientRect(Rect RectLogical, Color TopColor, Color BottomColor,
                      float CornerRadiusLogical = 0.0f);
```

### What unblocks in Pharos

`resolveAccentButtonStyle`-style resolvers gain a second color per surface
(`ColorBackgroundTop`/`ColorBackgroundBottom`) and the Load button, split
handles, and selected-row highlight all get a soft vertical sheen instead of
a flat fill.

---

## 4. REQUIRED — a soft drop-shadow / elevation primitive

No blur or shadow primitive exists; depth can currently only be conveyed
through background/border color layering (panel background one step lighter
than the window, which is what this theme pass leans on). This is the single
biggest thing separating "flat dark UI" from "elevated modern UI."

A true Gaussian blur is more than a PoC widget library needs. `Renderer`
already has the right shape of technique sitting right there, though: the
anti-aliasing fringe used by `DrawFilledRect`/`DrawRectOutline` is a solid
centre ringed by vertices that fade to zero alpha over `AaFeatherPhysical`
(1 physical pixel) — a soft shadow is the same idea with a much wider fringe.

### Proposed API

```cpp
// include/Penumbra/Render/Renderer.h

// A soft rectangular shadow, meant to be drawn just before the widget that
// casts it so it reads as sitting "behind" it. RectLogical is the caster's
// own rect; the shadow extends BlurRadiusLogical beyond it on every side,
// solid-alpha (per ShadowColor) at the caster's edge, fading to fully
// transparent at the outer edge. Reuses the rounded-rect ring tessellation
// (BuildRoundedRing) already built for anti-aliasing, just with the fringe
// width set to BlurRadiusLogical instead of the fixed 1px AA feather.
void DrawDropShadow(Rect RectLogical, Color ShadowColor, float BlurRadiusLogical,
                    float CornerRadiusLogical = 0.0f);
```

### What unblocks in Pharos

Panels, the pill button, and the tooltip get a real sense of elevation above
the window background instead of relying on color contrast alone -- and once
item 1 (`SplitPanel` margin) lands too, floating panels with a soft shadow
underneath them is the actual "modern sleek" look this theme pass was
reaching for with color alone.

---

## 5. REQUIRED — minimal line/triangle primitives for simple icons

The Explorer's expand/collapse glyph is the literal ASCII characters `-`/`+`
(see `TreeRow::DrawContent`) because `Renderer` only draws rects, text, and
textures -- there's nothing to draw a crisp chevron or caret with instead.
Not asking for a whole icon system (icon font, SVG-to-texture) -- two small
primitives are enough for Pharos to build its own chevrons/carets/arrows
directly in app code, matching Penumbra's own preference for small orthogonal
primitives over a big subsystem.

### Proposed API

```cpp
// include/Penumbra/Render/Renderer.h

// A straight line segment, ThicknessLogical wide, with the same anti-aliased
// edge treatment as DrawRectOutline.
void DrawLine(Point From, Point To, Color InColor, float ThicknessLogical);

// A solid-filled triangle -- enough, combined with DrawLine, to build a
// chevron (two lines) or a filled caret/arrow (one triangle) as an app-level
// helper, the same way Pharos already builds its own composite widgets on
// top of Box.
void DrawTriangleFilled(Point A, Point B, Point C, Color InColor);
```

### What unblocks in Pharos

`TreeRow`'s `-`/`+` text glyph becomes a real chevron drawn with `DrawLine`,
and the Atlas/breadcrumb code gets the same option anywhere it currently
fakes a directional indicator with punctuation (the `>` breadcrumb separator,
for one).

---

## 6. Explicitly not requested

- **Per-corner radius.** `BorderRadius` is one float, uniform on all four
  corners — no way to round only the top of a tab-style header, for example.
  Not needed anywhere in Pharos's current panels; noting for completeness,
  not asking for it.
- **A full icon system** (icon font, SVG-to-texture, bundled icon set).
  Item 5's two primitives are deliberately the minimum that unblocks Pharos's
  actual current need (a chevron); a whole icon pipeline is a much bigger
  design question for whoever owns Penumbra's roadmap, not something this
  theme pass needs answered.

---

## 7. What unblocks when this lands

Item 1 has the biggest layout payoff (floating, gapped panels instead of
square edge-to-edge tiles). Items 3–4 are what actually let Pharos's chrome
read as "elevated modern UI" rather than "flat dark UI with a nice accent
color" — gradients on the accent surfaces, a real shadow under floating
panels/tooltips. Item 5 replaces the last piece of ASCII-art-as-UI
(`-`/`+`/`>`) with real drawn glyphs. Item 2 is a one-line consistency fix.
