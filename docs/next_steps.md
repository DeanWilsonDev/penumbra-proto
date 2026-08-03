# Penumbra — Next Steps

> Living backlog, not a session-changelog: add an entry for anything new
> and open; remove or mark an entry done the moment it's actually
> implemented, rather than letting finished work linger. First file of
> this kind in this repo (previously used `docs/*_requirements.md`, one
> file per investigation, still present as historical record — not
> migrated into here retroactively).
> Last updated: 2026-08-03.

## Open items

### `Box`'s stack layout offers every child the same full available size, rather than shrinking it per sibling (2026-08-03)

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

### Proposed API

Not scoped to a concrete signature this session — two directions worth weighing, neither
attempted here:

- A new `LayoutMode` value (e.g. `LayoutMode::FixedLeadingStack`) with a companion
  `Box::LeadingExtentLogical` field: `Measure`/`Arrange` would measure the leading child
  first (`Children[0]`), then hand every *subsequent* child a `ContentAvailable` already
  shrunk by that measured extent (plus `ChildGap`) along the main axis — the general form
  of what `FixedLeadingStrip` hand-writes for exactly two children.
- Alternatively, generalize `Box::Measure`/`Arrange`'s per-child loop itself to shrink
  `ContentAvailable`'s main-axis component by the running `Cursor` position before calling
  each subsequent child's `Measure` — closer to how a real flexbox implementation
  partitions main-axis space incrementally, but a bigger change to `Box`'s existing
  two-pass (`Measure` then `Arrange`) structure than the first option.

### Required changes elsewhere

Once `Box` itself can express this, `pharos-proto`'s `FixedLeadingStrip` becomes
replaceable by an ordinary styled `Box`/`Frame`, which in turn reopens the "layout
composition belongs in `.iris`" goal (`pharos-proto`'s own `CLAUDE.md`) for the toolbar/
content split and the Atlas header/viewport split currently blocking `DropdownTrigger`'s
and the `ViewportWidget` treemap's `<Native>` migration — that's `iris`/
`penumbra-ui-backend`'s own follow-up (a matching Lustre property + `StyleApplier`
mapping), not scoped further by this entry.

### Explicitly not requested

- **A general CSS-flexbox `flex-grow`/`flex-shrink` sizing model.** The concrete ask is
  narrower: exactly one leading child gets its natural size, everything after it shares
  the remainder. No known consumer needs arbitrary per-child grow/shrink ratios.
- **A `Disabled` gradient variant.** No known consumer wants one; adding it
  speculatively would be unused surface. Revisit only if a real consumer
  needs it.
