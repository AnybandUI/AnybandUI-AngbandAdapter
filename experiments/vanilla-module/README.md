# Vanilla-branch native frontend experiment

Status: working MSVC/x86 prototype; official release-toolchain integration is NOT verified.

The engine checkout is `angband`, branch `codex/vanilla-frontend`, based exactly on
`4.2-release` / `f3082213b73f3e463e3d0d60bff4b00462beae6e`. The changes are in the
working tree, not a commit. `evidence/angband.patch` contains the entire candidate,
including new files. `provenance.json` records hashes of that patch and the tested
binaries. No previous modified engine archive or core object is linked.

## What this proves

An ordinary Windows Angband executable can retain its built-in frontend and also
load an explicitly selected, build-matched native frontend. Our DLL uses exported
engine C functions and data, existing input hooks and events, and a small set of
additional observational/context hooks. It supplies the existing AnybandUI full-v1
protocol. Presentation code resides in our DLL rather than in the engine patch.
This is normal DLL loading and imports, with no memory scanning or injection.

It does NOT make an already-published stock executable compatible. Stock
4.2-release has neither this loader/export contract nor the added semantic hooks.
The tested executable is a locally built candidate. The intended official-binary
experience requires upstream acceptance and inclusion in an official release.

## Complete upstream footprint

27 files: 265 added + 8 removed = **273 changed lines**, measured with
`git diff --numstat 4.2-release`, counting new files as well as existing ones.

- 172: module loading, executable exports, build fingerprint and build wiring.
- 97: command/quantity context and presentation observations.
- 4: separate MSVC optimized-division fix in obj-power.c, reproduced with a
  deterministic negative-power object. This is included, not hidden in the total.

The official MinGW/autotools build additions account for 16 of the 273 lines.
They have not been compiled here. The tested MSVC route accounts for the other
257 changed lines. This is a measured candidate, not a proof of an absolute lower
bound or of upstream acceptance.

## Cost moved to our side

The adapter owns 39 presentation C files / 37,551 physical lines, largely copied
or adapted Angband presentation code, plus its protocol implementation and SDK.
Those implementations were migrated from earlier experiments, but the executable
contains only the fresh baseline plus the enumerated patch. This explicitly trades
our maintenance burden for a smaller upstream footprint. Engine gameplay remains
in the executable. Structured map presentation reads known engine data; it does
not recover the map by scraping the terminal. Existing terminal fallback is kept.

This is a private, build-matched C ABI, NOT a stable plugin API. A release needs its
matching headers, import library and adapter DLL. Compiler/options/layout changes
require rebuilding and reviewing the SDK. Runtime build-ID matching rejects an
incompatible descriptor. A DLL is trusted native code and loads before its ID is
queried; this check is compatibility checking, not sandboxing. Loaded callbacks
remain valid until process exit.

## Verification on the final MSVC candidate

- Native session + deterministic pricing regression: 270 checks, 29 requests.
- Store/document/map scenario: 3,154 checks, 32 requests.
- Actual AnybandUI Connection scenario: 12 requests, 23 frames.
- Save loading: 15 checks, 3 requests.
- Birth cancellation: 14 checks, 3 requests.
- Production DLL over ordinary stdin/stdout: recorded real-client transcript,
  12 successful responses and 11 events; full-v1, birth, prompt/target cancellation,
  committed turn and save/close. No protocol errors, exit 0.
- Wrong-build and missing DLLs: exit 2, expected diagnostics.
- AnybandUI client regressions passed, including module manifest validation.
- Actual AnybandUI application rebuilt successfully.
- Engine git diff whitespace check passed.

Counts vary with generated dungeon/store contents. Logs and the real-client
request transcript are in evidence/. The production DLL contains no embedded
session/client tests. No computer-use automation was used. Known quarantined
transport-test and pile executables were not run.

The actual UI connection test is in-process. The separate stdin/stdout replay tests
the real executable and production DLL. Together they exercise both sides, but do
not constitute an automated test of SDL spawning this new module path through the
full GUI. The GUI was built, not driven. These tests are substantial regression
coverage, not exhaustive proof of every advertised capability.

## Run the staged package

Candidate package: `../../build/vanilla-package/` contains angband.exe, supporting
DLLs, anybandui.dll, lib/, and engine.anyband.json. It is explicitly named as a
minimal-patch candidate, not an official build.

From PowerShell in the repositories directory:

```powershell
& .\AnybandUI\build-ui-native\game\AnybandUI.exe --engines-dir .\AnybandUI-AngbandAdapter\build\vanilla-package --user-dir .\AnybandUI-AngbandAdapter\build\vanilla-manual-profile
```

## Build

Use the VS 2026 x86 developer environment, MSVC 19.51.36231, CMake and Ninja.
From the repositories directory, the engine configuration is:

```
cmake -S angband -B AnybandUI-AngbandAdapter/build/vanilla-modules -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DSUPPORT_WINDOWS_FRONTEND=ON -DSUPPORT_BUNDLED_PNG=ON -DSUPPORT_BORG=OFF
cmake --build AnybandUI-AngbandAdapter/build/vanilla-modules
cmake -S AnybandUI-AngbandAdapter/experiments/vanilla-module -B AnybandUI-AngbandAdapter/build/vanilla-adapter-production -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build AnybandUI-AngbandAdapter/build/vanilla-adapter-production
```

The pinned SDK build identifier must equal the engine's generated identifier;
configuration deliberately fails otherwise. Do not blindly overwrite it to force
a match. Source/toolchain changes require SDK review and regression testing.
For the test DLL, use the same configure command with a separate build directory
and `-DFRONTEND_SESSION_CHECK=ON -DFRONTEND_CLIENT_CHECK=ON`. Its client scenario
uses the actual sibling AnybandUI sources and already cached dependencies.
`prepare.ps1` intentionally refuses to regenerate from an old worktree;
`migration-history.ps1` documents the original migration only.

## Unfinished release work

The checked-in official Windows release workflow uses Ubuntu/MinGW/autotools,
not this tested MSVC path. The autotools additions still need a clean release
build. The current adapter recipe also uses MSVC options and its import-library
format; it must be ported/revalidated for a MinGW-produced executable. No official
release archive, SDK publication workflow, other-platform loader, or GUI-driven
end-to-end test is claimed here. Upstream must also decide whether exporting its
private C ABI and supporting trusted build-specific modules is acceptable.

A failed compiler setup filled Docker WSL's small distro filesystem with about
61 MB of task-created temporary data. After explicit user approval, only those
three temporary paths were deleted; deletion and freed space were verified.
No filesystem repair or removal of Docker/user data was performed. The setup
scripts beside this document are investigation history and must not be rerun.
