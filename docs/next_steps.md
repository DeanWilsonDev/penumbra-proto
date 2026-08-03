# Penumbra — Next Steps

> Living backlog, not a session-changelog: add an entry for anything new
> and open; remove or mark an entry done the moment it's actually
> implemented, rather than letting finished work linger. First file of
> this kind in this repo (previously used `docs/*_requirements.md`, one
> file per investigation, still present as historical record — not
> migrated into here retroactively).
> Last updated: 2026-08-03.

## Open items

_None open right now._

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
