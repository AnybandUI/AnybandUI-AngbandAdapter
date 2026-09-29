# Direct frontend experiment — 28 September 2026

## Result and location

This is a working architectural prototype, not a certified full-v1 replacement.

Engine branch: codex/direct-frontend, commit f3cd9246c.
Checkout: C:/Users/developer/source/repos/angband/.worktrees/direct-frontend.
The main angband checkout remains on codex/minimal-adapter-surface at d6c648288.
Both descend from 4.2-release, f3082213b73f3e463e3d0d60bff4b00462beae6e.

Adapter: this directory. The baseline adapter, baseline patch series and original fork remain available. engine.patch applies on d6c648288; it is the experimental delta, not a standalone patch for 4.2-release. surface.json measures the whole resulting branch against 4.2-release.

## What changed

The adapter is now a static session library with synchronous read, write, poll and fatal-stop callbacks. A small stdio host retains the external executable and JSON protocol. The actual engine can also run directly in a native test host, without spawning a child or creating pipes.

Normal session close saves and returns to the host. Native birth cancellation returns too. The frontend owns the outer lifecycle through Angband's existing startup, refresh, command and turn functions. The stock play-again reinitialization sequence is retained. Gameplay and turn processing remain in Angband.

The session API is deliberately one-shot per process: a second call is rejected. This does not yet make Angband a safely reusable embedded library. Global engine state, fatal quit paths and complete resource ownership still need work before putting it inside AnybandUI's production process.

A synchronous command-dispatch hook provides the adapter with the actual live command and resolved handler. The adapter derives known aiming effects within that scope, including nested targeting, and restores prior context on return. This replaces effect-context plumbing in engine command implementations; cmd-obj.c is now identical to 4.2-release. The handler identity matters: using a wand and throwing that same wand must not expose the same activation preview.

Quantity requests pass their item explicitly through an optional callback. Inferring every quantity subject from the active command was rejected because pickup and other nested operations do not necessarily have such an argument. The original quantity input remains the fallback.

Targeting has a synchronous input hook carrying location, mode and candidates. The adapter constructs its presentation state from borrowed data and the existing engine geometry routines. The original controller still owns targeting, look, recall, keyboard handling and confirmation rules. No target controller was copied outboard.

Twenty-two identical prototype headers were removed; unchanged functionality uses the baseline adapter modules directly. The experimental main module and two changed private headers are still separate copies to keep the baseline intact.

## Actual upstream footprint

Counts are additions plus deletions, not net lines, and include retained correctness and build changes. Fifteen added upstream test lines are excluded consistently from the engine/build column.

| Revision | Engine/build files | Added | Removed | Changed |
| --- | ---: | ---: | ---: | ---: |
| Original AnybandUI fork, previously measured | 55 | 589 | 97 | 686 |
| Previous reduced branch, d6c648288 | 50 | 374 | 54 | 428 |
| This experiment, f3cd9246c | 51 | 354 | 59 | 413 |

Including tests, this experiment is 52 files, +369/-59, 428 changed lines.
The reduction from the previous branch is 15 lines (3.5%); from the original fork, 273 lines (39.8%).
The experimental delta itself is nine files, +36/-61.
The number of touched files increased by one: cmd-obj.c returned to stock, but lifecycle access required ui-game.c and ui-game.h.

This is a substantial change in ownership and testability, but not a breakthrough in upstream patch size. It does not substantiate a promise of a tiny 100-line integration surface.

## Verification performed

Native MSVC builds succeeded for:
- The adapter executable and in-process session check.
- Angband's ordinary Windows frontend using the experimental engine, with no external frontend selected. This was a build check, not an interactive vanilla playthrough.

The final actual-engine scenario passed with 29 requests and 29 responses. Its 367 assertions include frame parsing and individual map-cell comparisons; they are not 367 independent feature tests. Coverage:
- Protocol handshake and native Human/Warrior birth.
- Observed-map versus read-only camera cell parity.
- Six tileset selections and return to text mode.
- Presentation changes leaving the game turn unchanged.
- Direct look/targeting entry and cancellation.
- Typed rest and explicit-subject quantity prompts, including cancellation.
- Unknown wand effect remaining hidden.
- Known Fire Balls wand exposing radius two.
- Nested targeting retaining effect context and restoring the outer aim prompt.
- Throwing that wand not inheriting its activation preview.
- A committed wait advancing the real engine and returning to readiness.
- Successful save/close returning normally to the host.
- Rejection of a second session invocation.

Separate saved-game reload passed (three requests, 15 checks), as did birth cancellation (three requests, 14 checks). Fatal-stop callbacks fail these tests if invoked unexpectedly. The inventory setup is a synthetic fixture created through engine object functions; requests and responses use the real adapter and engine.

AnybandUI's existing offscreen renderer consumed the final emitted state and capabilities and rendered a 1440x1000 image successfully. The image was inspected: map, status, inventory, messages and inspection content were present. This verifies real-state presentation compatibility, not a live AnybandUI transport session.

Retained evidence is in ../../build/direct-frontend/evidence:
session.log, load.log, birth-cancel.log, state.json, preview.bmp, build.log, stock-build.log.

No computer-use automation, memory scanning, process injection, antivirus exceptions, or retries of quarantined executables were used. The previously blocked child-process transport suite was not rerun. Successful native tests here do not establish the cause of earlier antivirus detections or guarantee future antivirus acceptance.

## What remains unproven

Full feature parity is not certified. The prior full-v1/transport suite was not rerun against this revision. Store transactions, complete spell/device coverage, automatic pickup, travel interruptions, combat/death/postmortem/play-again, sound, and all error/reconnection paths need broader regression coverage. Retaining their modules or controllers is not proof of parity.

The normal external executable builds, but no live AnybandUI-to-engine pipe test was completed in this experiment. Offscreen rendering and in-process protocol checks are explicitly separate pieces of evidence.

The adapter still uses the existing terminal frontend for some interactions. The new hooks avoid scraping for the contexts changed here; this is not a claim that all terminal dependencies have been removed.

## Outer limits and recommended direction

1. Keep the callback-based session boundary. It enables native deterministic integration checks now, and could support a different transport or eventual embedding without rewriting gameplay or presentation. For production today, the outboard executable still offers useful crash and global-state isolation.

2. Prefer direct semantic callbacks where the needed information exists transiently. Command execution, quantity subject and targeting input are demonstrated examples. Do not replace those with a universal untyped callback or infer missing knowledge from rendered text merely to lower line counts.

3. Split upstream review by responsibility. Correctness fixes, external frontend build integration, map presentation access, and input/session boundaries are separately understandable changes. A coherent review surface matters more than hiding all edits in a large new extension framework.

4. A truly tiny upstream patch would require a larger concession: maintaining replacements for substantial stock UI modules outboard, or persuading upstream to accept a broader frontend abstraction first. The former moves maintenance and parity risk into our repository; it does not remove that work. The latter may be the cleaner long-term design, but its initial patch will likely be larger. This experiment does not measure upstream maintainers' willingness to accept it.

5. Standard source-level access to engine data is sufficient for the current approach. Binary interception, memory scanning, hidden source rewriting, and runtime patching add fragility without solving the ownership problem.

The recommended result is to retain this experiment as an architectural candidate, expand its native parity scenarios, and only then replace the baseline. Do not promote it solely because its changed-line total is smaller.

## Reproduction

Configure Angband from the experimental checkout using:
- ANGBAND_EXTERNAL_FRONTEND=<adapter>/experiments/direct-frontend
- SUPPORT_BORG=OFF
- SUPPORT_SPOIL_FRONTEND=OFF

Build targets OurExecutable and anybandui-session-check.
Run from the build/game directory:
    anybandui-session-check.exe --data-dir lib --user-dir <fresh-disposable-profile>

The full scenario creates the save DirectSession. Reuse that profile only with DIRECT_SESSION_LOAD=1 for the load scenario. DIRECT_SESSION_CANCEL_BIRTH=1 selects birth cancellation; use another disposable profile. DIRECT_SESSION_SNAPSHOT=<absolute-json-path> retains renderer input. Never use a real player profile for these fixture tests.

The AnybandUI preview executable accepts that JSON path, an output BMP path, and width/height. No new test runner scripts are required.
