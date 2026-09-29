> Historical first-extraction report. Superseded by [the reduction report](reduction-report.md).

# Adapter implementation and validation report

Built against Angband 4.2.6, branch 4.2-release at
f3082213b73f3e463e3d0d60bff4b00462beae6e. Validation: 27–28 September 2026.

## Result

A working independently owned engine package implements the unchanged full-v1
contract. All capabilities remain mandatory. The graphical application consumes
the same protocol and engine/save identity. No application implementation changes
were required; its two documentation references were updated. The original
Angband checkout is unchanged. This folder has not been initialized as a Git repo.

The adapter owns transport, serialization, presentation policy, dependency,
manifest, tests and release packaging. Angband owns gameplay and supplies generic
queries, events and synchronous interaction hooks. The build uses Angband's own
source lists through a small external frontend CMake entry. See architecture.md
for the interface decisions and retained dependencies.

## Measured upstream surface

The former implementation changed 55 existing engine/build files (+589/-97),
and 90 files overall including owned integration files (+7245/-98).
The new patch series changes 51 engine/build files (+600/-77), plus one existing
unit-test file (+15): 52 files, +615/-77 total. Measurements are relative to the
entire pinned release, not an incremental comparison against the integration.
The machine-readable breakdown is surface.json.

Four files are back to stock: ui-command.c/.h and ui-knowledge.c/.h. Existing
command and direction callback interfaces replace special presentation state.
Artwork filtering moved outboard, retaining the engine preference parser.

Raw line churn is essentially unchanged (686 versus 692 lines including the new
test); clearer formatting and explicit tests offset the removed code. The win is
separating maintenance responsibilities and reducing specialised core hooks,
not claiming a dramatic numerical reduction. Full native support still needs
genuine gameplay observations and queries across several subsystems.

## Validation evidence

- Final x64 adapter engine: 932/932 assertions across 84 Angband unit suites.
- Standard Win32 frontend with adapter disabled: built and passed the same
  932/932 assertions, testing the normal frontend configuration as well.
- All 66 original engine integration tests passed (36.088 seconds on the final
  run). Coverage includes birth, saves, inventory, stores, spells, targeting,
  death/replay, tuning, camera, tiles, known information and query RNG purity.
- Three extraction regressions passed: imported keymaps, stock browser path,
  and contaminated artwork preferences not changing gameplay or bindings.
- Full-v1 contract identity, all capabilities, startup endpoints and clean
  shutdown passed against the actual application's contract.
- Actual C++ client transport ran town and dungeon sessions. Input-to-state
  medians were 19.145 and 37.391 ms; p95 values 20.1513 and 38.0364 ms.
  Receive work on the UI thread was p95 2.6344 and 3.3682 ms. These are local
  measurements, not general performance guarantees.
- Application client, audio, Direct3D 12 GPU, asset and no-engine offscreen
  rendering checks passed. A real depth-one engine state was also rendered
  through the application preview renderer and visually inspected offscreen.
- Original versus extracted engine replayed the same saved character: 15
  identical gameplay views, inventory/equipment events, rest cancellation,
  all six tilesets, camera, waiting and a level transition. Ephemeral item and
  creature revision IDs were excluded from the comparison.
- A fresh archive of the pinned release accepted all patches with strict
  whitespace checks. Its complete file contents matched the tested source,
  allowing only Windows line-ending differences. Source and patch fingerprints
  guard reuse of prepared trees.

Logs are in build/verification-20260927-222152; parity fixtures and offscreen
render are in build/parity. The strict unit runner was added after discovering
the upstream runner can exit successfully despite failed assertions. New sound
cue assertions now verify both independent cues and legacy sound preferences.

## Packaging

tools/package.py produces a standalone engine directory and ZIP with runtime,
data, manifest, licensing notices, file checksums and exact corresponding source.
No personal saves or settings are included. README.md documents reproducible
builds and installation. Package relocation validation is recorded separately
in build/package-validation.json and its associated logs.

## Upstream prospects and remaining limits

This improves the proposal from asking Angband to own another complete frontend
to asking it to support useful source-level extension boundaries. Acceptance is
still uncertain: no maintainer has reviewed or endorsed this work. A single PR
requiring every hook and official full-support endorsement would remain a
difficult sell. A numerical acceptance percentage would not be evidence based.

Submit independent correctness fixes first, then small justified query/event
changes with focused engine tests, and the generic external build entry. The
interface patch here is a review inventory, not an ideal single upstream PR;
split it by subsystem. Offer to maintain the adapter and compatibility CI.
Demonstrate normal frontend compatibility and avoid asking upstream to version
the JSON protocol or promise a stable binary plugin ABI. Full-v1 stays mandatory
in this adapter while individual upstream improvements can land incrementally.

Only the Windows configurations above were validated. Linux/macOS builds,
sanitizers, long campaigns and future Angband releases were not verified. The
integration is source-version-specific, single-threaded, and retains private
implementation headers and temporary state restoration for tuning/comparison.
It is not a stable public SDK. The suite is substantial evidence of preserved
support, not proof that every gameplay combination is bug free. No interactive
computer-use testing is claimed; further validation used automated/offscreen
checks at the user's request.
