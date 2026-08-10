# Penumbra — Next Steps

> Living backlog, not a session-changelog: add an entry for anything new
> and open; remove or mark an entry done the moment it's actually
> implemented, rather than letting finished work linger. First file of
> this kind in this repo (previously used `docs/*_requirements.md`, one
> file per investigation, still present as historical record — not
> migrated into here retroactively).
> Last updated: 2026-08-10.

## Open items

_None open right now._

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
