# AnybandUI Angband adapter

This project is the canonical Angband implementation of AnybandUI's full-v1
protocol. It owns the C adapter, vendored cJSON, engine manifest, packaging and
engine patch series. The desktop UI is built independently in `AnybandUI`.

The adapter is compiled into `angband-anybandui.exe` alongside the Angband core.
It is source-version-specific: the upstream base is 4.2-release commit
`f3082213b73f3e463e3d0d60bff4b00462beae6e`, recorded in `upstream.json`.
All capabilities are mandatory. Saves remain in the `angband-4.2.6` family.

## Current installation

The locally tested build is `build/minimal-native/game`. Its executable and game
data are installed in `../AnybandUI/build-ui-native/game/engines/angband-4.2.6`.
Launching AnybandUI normally selects this standalone-adapter package. The old
embedded engine and older frontend experiment were removed from discovery.

The installation preserves engine identity and normal save locations. The manual
test character remains separately in `build/manual-test-profile`; no saves were
moved between profiles. To continue that profile, run from this directory:

```powershell
& ../AnybandUI/build-ui-native/game/AnybandUI.exe --engines-dir build/minimal-native/game --user-dir build/manual-test-profile
```

## Build

The sibling Angband working tree now contains the engine interfaces from the
current patch series, with the embedded adapter removed. From a Visual Studio
x64 developer shell in this directory:

```powershell
cmake -S ../angband -B build/external-native -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo "-DANGBAND_EXTERNAL_FRONTEND=$PWD" -DSUPPORT_BORG=OFF -DSUPPORT_SPOIL_FRONTEND=OFF
cmake --build build/external-native --target OurExecutable anybandui-map-tests
Push-Location build/external-native/game
./anybandui-map-tests.exe -v
Pop-Location
```

Use a fresh build directory. The installed, manually tested executable has not
been replaced by an untested rebuild. On a separate pristine checkout of the
pinned upstream commit, apply the three files in `patches/series` in order first.
`tools/prepare.py` can prepare an isolated source snapshot from a local repository;
choose a new `--source` destination rather than reusing stale prepared sources.

For packaging a successfully built engine, `tools/package.py` takes explicit
`--build`, `--source` and new `--output` paths. It includes exact source, data,
runtime libraries and notices. Install the resulting package under the UI's
`engines/` folder. Install only one package for the same engine/save identity.
The desktop frontend ZIP remains engine-free.

## Validation and limits

On 29 September the existing binaries passed five adapter map tests and 931
engine test cases in 83 suites. Client and GPU checks passed, and a previously
captured adapter state rendered successfully. The user also manually verified
actual gameplay and save/quit/relaunch/reload with this standalone engine.
See `build/verification-20260929-standalone/REPORT.md` for recorded evidence.

The migrated source subsequently built cleanly using NMake in build/external-native;
its fresh map test executable passed all five checks. The earlier Ninja configure
attempt stalled during compiler setup. The latest complete
live automated integration run has not been repeated: Bitdefender quarantine
remains unresolved for several helpers and the transport/object-pile test targets.
Do not restore or recreate quarantined tools as a workaround. `tools/verify.py`
references unavailable helpers and is not currently a usable verification entry.
Earlier counts in historical reports are not fresh certification of this build.
Only Windows has been validated.

## Ownership

- `src/`: current adapter and presentation policy.
- `frontend.cmake`: external build entry and vendored JSON dependency.
- `patches/`: engine correctness fixes, generic interfaces and external build support.
- `full-v1.json`: copy of the frontend-owned protocol contract.
- `tests/` and `docs/`: regression sources and architectural evidence.
- `experiments/`: historical alternative integration prototypes, not the supported build.

The retired embedded source, local documentation edits, old build directories and
installed packages are preserved in `build/retired-embedded-20260929/`. They are
outside runtime discovery and are not inputs to the current build. Historical
`dist/` ZIPs predate this migration and must not be used as the current package.

GPLv2; Angband files retain their licensing and cJSON retains MIT licensing.
See LICENSE, docs/angband-copying.rst and vendor/cjson/LICENSE.
