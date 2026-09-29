# Validation

## Available checks

- `tests/map.c`: five native checks covering drawing layers, web occlusion,
  read-only RNG/monster colour, known-map memory isolation and minimap lighting.
  Build the `anybandui-map-tests` target and run it from its `game` directory.
- `tests/test_package.py`: isolated packaging checks using temporary fixtures.
  These do not launch an engine or exercise gameplay.
- Angband's own unit targets cover engine behavior. Inspect printed pass counts:
  its runner can exit successfully despite failed assertions.
- AnybandUI startup negotiates engine identity and Anyband Protocol capabilities.
  A successful handshake alone does not certify every capability.

- `tests/sound.c`: Windows real-device smoke checks for official MP3 playback,
  missing files, preference loading and repeated shutdown/startup. Build
  `anybandui-sound-tests` and run it with `-v` from the build's `game` directory.
  It may play a short sound; inspect the printed 3/3 result as well as exit status.

## Recorded baseline

On 29 September 2026, a clean MSVC/NMake build of the standalone engine and map
checks succeeded. Its map executable passed 5/5. Earlier that day the existing
engine binaries passed 931 tests across 83 suites; that was not a fresh complete
unit rebuild. UI client/GPU checks and rendering a captured adapter snapshot also
passed. The user manually verified gameplay and save/quit/relaunch/reload with
the installed standalone package.

Fresh complete live transport, gameplay-parity and integration automation remains
unverified. Several earlier helpers and the transport/object-pile targets were
affected by an unresolved Bitdefender quarantine. Broken wrappers depending on
those missing helpers have been removed; they were not recreated or bypassed.

Windows is the validated platform. Other platforms, long campaigns and future
engine revisions require additional validation. Historical prototypes and their
reports are available in Git history, not as evidence for the current build.
