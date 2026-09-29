# Shared native frontend — implementation report

Implemented 28 September 2026. This report supersedes initial-experiment-report.md.

## Locations and revisions

- Engine: branch codex/direct-frontend, commit d18ac150d, in C:/Users/developer/source/repos/angband/.worktrees/direct-frontend.
- Base: 4.2-release, f3082213b73f3e463e3d0d60bff4b00462beae6e.
- Adapter: this directory. Its unchanged modules are included from ../../src; changed modules are isolated here. The adapter folder remains uninitialized as a Git repository.
- AnybandUI: commit c4acbf596 on reduction-experiment adds the opt-in real-client integration check. Pre-existing edits to test_transport.cpp were not included.
- The main Angband checkout remains on codex/minimal-adapter-surface at d6c648288. The original adapter, original fork and minimal patch series remain intact.
- Runnable engine: ../../build/direct-frontend/game/angband-anybandui.exe, with its data and engine manifest alongside it.

## Implemented architecture

### Native interactions and shared rules

The store controller now selects, requests quantity, confirms and submits CMD_BUY, CMD_SELL, CMD_STASH or CMD_RETRIEVE directly. It no longer calls a menu row handler, enters the stock chooser, or synthesizes its selection keystroke. Store entry invokes the complete native session before allocating a terminal menu.

The purchase-limit calculation was extracted from the terminal controller into store_purchase_limit(). Both frontends use the same affordability, device-charge rounding, inventory capacity and unknown-flavor rules. Pricing, carrying, stock capacity and transaction execution still use engine functions. Inscription safeguards are preserved through get_item_allow(), including cancellation before quantity selection.

The obsolete store_check_hook was removed. The store session hook no longer exposes struct menu.

Aiming now uses aim_get_direction(), a shared controller supplied with input and timed-scan callbacks. The native frontend owns the normal aiming input boundary. The engine no longer owns the textui_aiming flag. The original direction transitions, lazy diagonal input, old-target option and target validation remain in the shared implementation.

Targeting reads native events through its existing direct input boundary while retaining the engine's navigation and confirmation controller. Monster tracking is maintained. Detailed recall, pile browsing and ignore interactions deliberately retain the original controller/dialog path rather than being replaced with a partial implementation. Native birth continues through the existing adapter-owned birth controller.

### Structured engine documents

The existing textblock type now carries owned semantic spans, with sections, labels, values, text offsets and colours. Ordinary text and terminal rendering remain available. Concatenation copies metadata and adjusts offsets; freeing the source block does not invalidate the destination.

Item descriptions annotate their ordinary textblocks instead of invoking an adapter-specific section callback. The character sheet returns the same document type; both the terminal character screen and the adapter consume that document. Calculations and prose stay in the engine. The wire representation remains adapter-owned and compatible with AnybandUI.

### Shared read-only map presentation

ui-map-presentation.c contains map_present(), extracted from the terminal renderer. It returns the top glyph plus terrain, trap, object and actor layers. It reads engine knowledge/preferences, consumes caller-supplied hallucinated glyphs, and performs no RNG or monster-colour writes.

The terminal wrapper retains its legacy RNG and colour bookkeeping. The adapter uses private visual randomness with actual configured object/monster glyphs, stable for a coordinate within a turn. Native map queries therefore do not consume gameplay randomness; hallucination no longer falls back to arbitrary ASCII glyphs when a graphical tileset is selected.

Both native viewport and full camera query the same known-map presenter. The terminal-draw cache and map_draw_hook/map_reset_hook are gone. Level reset uses the existing level event. Travel's observed-pickup decision queries that same presentation.

### Snapshots and transient events

Persistent map data joins inventory, equipment and character data in snapshots at interaction boundaries. There is no separate terminal-observation cache to synchronize.

Direct events remain for combat feedback, motion, teleportation, projectiles, sound and interruption-sensitive pickup/travel. A final snapshot cannot reconstruct their ordered intermediate outcomes. Those engine event sites were retained intentionally, not replaced with message parsing or inference.

Message acknowledgement is exposed consistently in the top-level snapshot, including transitions where there is no native dungeon/store view.

### Transport and AnybandUI

The callback-based session library still supports an ordinary external executable. Normal close and birth cancellation return to its host. It remains one session per process; subsequent calls are rejected.

AnybandUI's production connection and rendering code remain protocol-compatible. A new optional integration target links the real Connection and BirthPanel code with the engine session library, exchanging actual protocol frames in one process. This provides live client/engine verification without child-process transport.

Production embedding is not enabled: Angband still has global state and process-wide fatal paths. The external process remains the normal production boundary.

## Measured footprint

All figures count additions plus deletions. They do not discount moved code.

| Engine revision | Engine/build files | Added | Removed | Changed lines |
| --- | ---: | ---: | ---: | ---: |
| Previous minimal branch d6c648288 | 50 | 374 | 54 | 428 |
| Initial direct-session experiment f3cd9246c | 51 | 354 | 59 | 413 |
| Shared native frontend d18ac150d | 56 | 788 | 351 | 1,139 |

Including the retained upstream test change: 57 files, +803/-351, 1,154 changed lines.
This implementation adds 727 changed lines to the prior 413-line experiment. The latest engine commit itself is 18 files, +543/-401.

The main reason is extraction rather than deletion: the new map component is 310 lines, while ui-map.c removes 251 lines relative to release. Store-rule extraction and generic document support also carry a real review cost.

This is a cleaner shared architecture, not a smaller initial upstream patch. The removed interfaces are specific, measurable improvements, but they do not justify claiming the whole patch shrank.

Patches:
- engine-from-release.patch: complete patch against 4.2-release.
- engine.patch: cumulative experimental delta against d6c648288.
- shared-components.patch: this implementation's delta against f3cd9246c.
- surface.json: exact per-file counts and revision pins.

The complete exported patch passed a reverse application check against the committed checkout.

## Verification and evidence

Final native runs:

| Check | Result |
| --- | --- |
| Engine/adapter session scenario | 29 requests and responses; 326 assertions |
| Native store plus document/map contracts | 32 requests and responses; 3,156 assertions |
| Saved-game reload | 3 requests and responses; 15 assertions |
| Birth cancellation | 3 requests and responses; 14 assertions |
| Actual AnybandUI Connection + BirthPanel + engine | 12 requests, 23 response/event frames |
| Existing AnybandUI client checks | Passed |
| Ordinary Windows Angband frontend | Built successfully |
| Game and store offscreen rendering | Rendered and visually inspected |

Assertion counts include repeated cell comparisons and frame validation, not thousands of independent gameplay scenarios. Town generation varies, so counts can vary between runs.

The gameplay scenario covers native birth, six tilesets and text mode, viewport/camera parity, unchanged turns for presentation operations, rest and quantity cancellation, known versus unknown device effect previews, nested targeting cancellation, throwing without activation-context leakage, a committed turn, save/close, and refusal of a second session invocation.

The shopping scenario covers cancelled and successful purchases, sales, home stash/retrieve, insufficient funds, a shared full-pack/unknown-flavor rule check, quantity cancellation, inscription cancellation, and usable game state after leaving. Fixtures use disposable profiles and deliberate engine setup, not real saves.

Document/map contracts cover metadata ownership after concatenation, preservation and coverage of prose, character fields, the terminal character renderer consuming the document, repeatable whole-level presentation, complete RNG-state preservation, remembered-terrain isolation, monster-colour preservation, trap/object/actor layers, web occlusion, and caller-supplied hallucinated glyphs.

The live client check exercises real contract negotiation, native birth submission through BirthPanel::act, client readiness and typed prompts, native targeting, committed turn, and save/close acknowledgement. It does not create an SDL process, pipes, desktop window or worker thread.

Evidence: ../../build/direct-frontend/evidence/native/
- session.log, store.log, load.log, birth-cancel.log
- client-session.log, client.log
- native-frontend-build.log, native-ui-build.log, native-stock-build.log
- game.json, store.json, game.bmp, store.bmp

No computer use, memory scanning, binary interception, antivirus exclusions or quarantined test-program retries were used. One sandboxed UI build stalled before compilation; the verified build processes were stopped and the same source/target built successfully with normal compiler access. No antivirus cause is asserted.

## Scope of the conclusion

The requested architectural changes are implemented and the listed integration checks pass. Full-v1 compatibility is negotiated by the actual client. This is not an exhaustive certification of every full-v1 behavior: the former complete child-process parity suite was not rerun.

Broad combat/death/postmortem/play-again coverage, all spell/device combinations, detailed recall variations, every store type, long travel/interrupt sequences, Linux builds and packaging-level antivirus acceptance remain outside this verification. No UI feature was intentionally removed to shrink the patch. Detailed stock interactions were retained where replacing them would risk partial behavior.

The minimal branch remains the candidate if immediate patch size dominates. This branch is the candidate if upstream is willing to review reusable frontend infrastructure. Neither branch establishes upstream maintainer acceptance.

## Reproduction

Build the experimental Angband checkout with:
    ANGBAND_EXTERNAL_FRONTEND=<adapter>/experiments/direct-frontend
    SUPPORT_BORG=OFF
    SUPPORT_SPOIL_FRONTEND=OFF

Build OurExecutable, anybandui-session-check and, for the client integration, anybandui_engine.

Run from build/direct-frontend/game:
    anybandui-session-check.exe --data-dir lib --user-dir <fresh-disposable-profile>

DIRECT_SESSION_STORE=1 selects native shopping/document/map checks.
DIRECT_SESSION_LOAD=1 reuses a profile containing the saved DirectSession.
DIRECT_SESSION_CANCEL_BIRTH=1 selects cancellation with another fresh profile.
DIRECT_SESSION_SNAPSHOT=<absolute-path> saves the final rendering fixture.
DIRECT_SESSION_STORE_SNAPSHOT=<absolute-path> saves a native storefront fixture.

Configure AnybandUI with ANYBANDUI_ADAPTER_BUILD=<adapter>/build/direct-frontend.
Build anybandui-session-ui-tests, then run it with --data-dir pointing to the adapter build's game/lib and --user-dir pointing to a fresh disposable profile. It creates NativeUI.

The optional target expects matching compiler/runtime/architecture for the two builds. Normal AnybandUI builds do not link the engine library. The current tested toolchain is MSVC x64 for the adapter/client checks and MSVC x86 for the bundled-PNG Windows frontend.

Render fixtures with the existing anybandui-ui-preview executable:
    anybandui-ui-preview.exe <fixture.json> <output.bmp> 1440 1000

Do not use real player profiles with the fixture scenarios.
