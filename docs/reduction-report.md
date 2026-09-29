This report describes the preceding revision. See [the drawing-hook update](map-hook-report.md) for current changes and validation limits.

# Reduced Angband integration

Completed 28 September 2026 on local Angband branch
`codex/minimal-adapter-surface`, based directly on `4.2-release` at
`f3082213b73f3e463e3d0d60bff4b00462beae6e`.

## Result and location

The changes are committed in the actual Angband checkout, not only in an
exported source tree. The branch is clean and has not been pushed.

- `f4b82bcc9`: independent correctness fixes.
- `219ba26c4`: reduced observation and input interfaces.
- `085b2f81b`: generic external frontend build support (branch tip).

The external adapter's implementation and patch series have been updated to
match this branch. The full-v1 protocol, mandatory capabilities and save identity
remain unchanged. Its working executable and data are in
`build/minimal-native/game`. Existing ZIPs in dist predate this reduction and
must not be mistaken for this build.

## Measured change footprint

All figures compare the complete integration against the same pinned release.
They exclude adapter-owned code from the engine/build comparison.

| Engine/build changes | 4.2.6-anybandui | First extraction | Reduced branch |
| --- | ---: | ---: | ---: |
| Existing files changed | 55 | 51 | 50 |
| Added lines | 589 | 600 | 395 |
| Removed lines | 97 | 77 | 62 |
| Added plus removed | 686 | 677 | 457 |

The engine/build change volume is 33.4% smaller than 4.2.6-anybandui and 32.5%
smaller than the first extraction. Additions are 32.9% lower than the original
integration. Including the retained sound unit-test changes, the final branch
changes 51 files, +410/-62. See surface.json for the complete inventory.

This is a material reduction, not a claim of the theoretical minimum. Changes
still span many subsystems because accurate full support needs observations at
the point where the engine resolves actions. Formatting is readable; statements
have not been packed together to improve these figures artificially. No forced
includes, source rewriting, symbol interception or replacement copies of engine
translation units are used to hide changes from the diff.

## Architectural reductions

Object inspection now exposes existing knowledge-limited calculations and
section boundaries. Combat row construction, labels, titles and JSON layout
live in the adapter. Damage, blows and equipment effects still come from the
original engine functions; no duplicate combat rules were introduced.

Level-feeling formatting moved outboard. Angband retains the original display
function and its text tables, which the adapter reuses. Pure full-camera query
assembly also moved into the adapter while sharing the engine renderer.

Generic world-coordinate mouse events replace special click-at, aim-at and
target-relocation interfaces. The ordinary target and movement paths process
them. Screen-coordinate events retain their original conversion and edge
scrolling. The adapter owns explicit confirmation; the target loop retains its
normal cleanup. A new regression checks every map corner without accidental
edge dragging or spending a turn.

The store adapter receives the existing menu and invokes its action handler.
The added transaction wrapper is gone. Stock selection, quantities, legal
actions, pricing, command dispatch and post-action bookkeeping remain in the
original store implementation. The adapter supplies the selected row at the
actual input boundary, after the engine's delayed input flush. Confirmation
wording is adapter-owned; the price is computed by the engine.

Movement uses precise point events around the original monster swap, and at
teleport departure/arrival. Adapter code pairs those events and assembles
animation records with the same visibility and hallucination filtering. It
does not infer movement from snapshots. Engine-specific animation wrappers and
the specialised motion payload/dispatcher were removed.

The earlier reductions through existing command and direction callbacks remain.
Six files modified by 4.2.6-anybandui now match release: ui-command.c/.h,
ui-knowledge.c/.h, ui-context.h and mon-util.h. The generic coordinate change
adds a modification to ui-event.h, accounting for the net five-file reduction.

## Validation completed

- Adapter branch build: 932/932 assertions across 84 upstream unit suites,
  also checked by the strict assertion-count runner.
- Ordinary Win32 frontend, adapter disabled: built, with 932/932 assertions.
- All 66 full-support engine integration tests passed after the store input fix.
- Four adapter boundary regressions passed, including exact world coordinates,
  imported keymaps, original browser behaviour and filtered artwork preferences.
- Full-v1 identity, capabilities, startup endpoints and clean shutdown passed.
- A strengthened same-save comparison against the original 4.2.6-anybandui
  executable matched 50 complete selected gameplay views and movement feedback.
  It explicitly seeded moving creatures and requires walking events, so the
  movement check cannot pass without exercising walking. Ephemeral item and
  monster revision IDs are excluded; the engine computations and visible
  outcomes are compared. All six tilesets, camera mode, rest cancellation,
  browser events and a level transition are included.
- Actual AnybandUI C++ transport: passed town and first-floor sessions,
  movement, save/shutdown and process-reader cleanup on the successful rerun.
  Movement median/p95: town 19.1589/19.9033 ms; dungeon 36.8525/37.5016 ms.
- The actual AnybandUI offscreen renderer rendered the new engine's depth-one
  state successfully. The resulting image was inspected. No computer use or
  interactive GUI testing was used in this work.
- The reduced patches applied cleanly to a fresh archive of the pinned release.

The first store regression run found that prequeued selection was discarded by
delayed flushing; that was fixed and the full suite subsequently passed.
One transport run reported an interrupted walking fixture rather than a latency
failure. The successful rerun cleared nearby creatures via the existing wizard
command in the disposable benchmark profile. The unchanged gameplay suite and
the strengthened parity test still exercise creature behaviour.

Primary suite logs are under build/verification-20260928-084102. That results
file correctly retains the initial transport failure; the later successful
transport run is reported above rather than rewriting failed evidence. The
parity fixture and image are in build/minimal-parity. Additional build logs are
preserved in build/reduction-evidence.

## Antivirus interruption and remaining limits

The user reported Atc4.Detection quarantines affecting the Python build/test
helpers, including tracked AnybandUI readiness and contract scripts. Antivirus
protection was not disabled, and quarantined automation was not restored or
rerun. The exact behavioural trigger is not established by the detection label.
The repeated Python-driven process tests were stopped. README.md now documents
direct native build commands rather than depending on those missing helpers.

The extra clean archive build was interrupted before completion; it is not
claimed as a passed build. The actual branch build, gameplay tests, stock
frontend build, client transport and offscreen render listed above did complete.
No refreshed distributable ZIP was created after the interruption. Quarantined
test helpers are currently unavailable for rerunning the full automation.

Windows is the validated platform. Linux/macOS, sanitizers, long campaigns and
future Angband revisions remain unverified. The adapter is source-version-
specific and single-threaded. These changes preserve the tested full-v1
behaviours, not the previous fork's exact C API signatures. The adapter has been
updated with the new interfaces. Upstream acceptance is not assumed.
