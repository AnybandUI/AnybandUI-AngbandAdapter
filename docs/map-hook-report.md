# Generic drawing hooks — 28 September 2026

Local Angband branch: `codex/minimal-adapter-surface`.
Commit: `d6c648288` (not pushed). Base: `4.2-release`, pinned 4.2.6.

This is a further, bounded architectural reduction. It is not the previously
suggested outer-limit design, nor a completed fresh end-to-end parity validation.

## Result and footprint

| Engine/build changes against release | Added | Removed | Total |
| --- | ---: | ---: | ---: |
| Original 4.2.6-anybandui | 589 | 97 | 686 |
| Previous reduced branch | 395 | 62 | 457 |
| Current branch | 374 | 54 | 428 |

The current branch changes 50 engine/build files. Including the existing
15 upstream sound-test lines: 51 files, +389/-54, total 443. This pass removes
29 changed lines overall; the engine/build total is 37.6% below the original
fork, and 6.3% below the preceding reduced implementation. Adapter-owned tests
and protocol implementation are outside those upstream numbers. No generated
patches, substituted engine source files, memory scanning or interposition are
hidden from the count. Two unrelated whitespace changes were restored to stock.

## Architecture

The semantic map already used direct engine observations. This change makes
that boundary more generic and places the wire representation entirely in the
adapter:

- `grid_data` carries the queried grid's coordinates.
- `map_draw_hook` receives known grid data and four drawing layers: terrain,
  trap, object and actor. Those layers come from Angband's existing renderer,
  including its transparency, colour, camouflage and occlusion decisions.
- The adapter's `anybandui-map.h` builds its existing 13-field map records.
  Angband no longer declares or constructs `struct map_visual`.
- Four normal drawing call sites return to the release's original map query
  and drawing calls. The special `map_info_as_text` wrapper is removed.
- Minimap-only lighting overrides bypass the observation hook. Otherwise they
  could overwrite the canonical map cache with artificially lit cells. A
  dedicated native regression test covers that distinction.
- Read-only camera queries keep the existing known-map boundary and seeded
  hallucination rendering. They neither update map memory nor consume gameplay
  RNG nor change a monster's drawing colour.

The protocol, full-v1 capabilities and gameplay calculations are unchanged.
No new terminal scraping was introduced. Existing terminal fallback screens
remain available; replacing every such screen is a separate, larger feature
migration and has not been claimed here. AnybandUI production code did not
need a compatibility change; its native transport test was extended.

## Validation on this revision

Passed:

- Native external-adapter build (`build/minimal-native`).
- Ordinary native Win32 frontend build with the adapter disabled
  (`build/reduced-stock-win32`).
- All five adapter-owned in-process checks, inspected by assertion output:
  drawing layers and observer coordinates; web occlusion and absent layers;
  read-only RNG and monster colour; known-map/memory isolation; minimap-only
  lighting isolation.
- `git diff 4.2-release --check`.
- All three exported patches applied to a fresh archive of the pinned release.
  All 51 changed files exactly matched the committed branch after line-ending
  normalization.

The first memory-isolation test incorrectly expected the read-only query to
reveal a newly seen but not yet memorized square. The test was corrected to
assert the existing known-only behaviour, then verify that ordinary drawing
records the terrain. No engine behaviour was relaxed to make that test pass.

Not passed / blocked:

- The broader upstream unit target stopped at a linker failure creating
  `unittests/object/pile/pile.exe`; this is not a fresh complete unit-suite pass.
- AnybandUI's extended native transport test compiled but could not link
  `game/anybandui-transport-tests.exe`. It was absent and no running process
  held it open. The user confirmed that disappearing files should be treated
  as Bitdefender `Atc4.Detection` blocks. That affected target was not retried
  after confirmation, renamed or rebuilt elsewhere to bypass the block.
- Consequently, the newly added live-session checks (birth, observed-versus-
  queried map comparison, six tilesets, camera, viewport and unchanged turn)
  have not run. There is no new end-to-end AnybandUI pass on this revision.
- Earlier full integration/parity results in `reduction-report.md` describe
  the previous revision, not fresh validation of this one.

No computer use, security exclusions, antivirus disabling, process memory
inspection, restored quarantined scripts or replacement subprocess harness
were used. Native test execution uses Angband's in-process unit framework.
The exact behaviour causing the antivirus detection remains unestablished.

Compiler and failed-link logs are under `build/map-hook-evidence`.

## Assessment

The existing rendering and input hooks offer a credible way to keep the
upstream API generic. This pass demonstrates a modest further reduction;
it does not substantiate the earlier 100–180-line outer-limit estimate.
Getting much smaller would require additional design work at the targeting,
input-context and observation boundaries, with sufficient integration coverage
before accepting it. Copying Angband's target loop or renderer wholesale into
the adapter would make the diff smaller while adding maintenance obligations;
that was not done simply to improve the line count.

The new patch series matches the current branch. Previously prepared source
copies and distribution ZIPs are older artifacts and must not be presented as
builds of this revision. End-to-end parity remains the delivery gate.