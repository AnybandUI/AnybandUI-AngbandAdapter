# AnybandUI Angband adapter

An Angband-specific implementation of [Anyband Protocol](https://github.com/AnybandUI/AnybandUI/tree/main/protocol)
(`anyband-protocol`). The adapter compiles with the Angband core into one engine
executable; the desktop UI communicates with it through JSON over stdin/stdout.

The supported engine base is Angband 4.2.6 at the commit pinned in `upstream.json`,
plus the patches in `patches/series`. This is a source-level integration, not a
DLL plugin or a stable engine ABI. The save family is `angband-4.2.6`.

## Build

Requires Python 3.12+, Git, CMake and a C compiler. The tested configuration is
Windows with the Visual Studio x64 developer shell and NMake. From this directory,
with an Angband repository in `../angband` containing the pinned commit:

```powershell
python -B tools/prepare.py --repository ../angband
cmake -S build/engine -B build/native -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=RelWithDebInfo "-DANGBAND_EXTERNAL_FRONTEND=$PWD" -DSUPPORT_BORG=OFF -DSUPPORT_SPOIL_FRONTEND=OFF
cmake --build build/native --target OurExecutable anybandui-map-tests
```

Preparation exports the pinned commit and applies the patches without changing
the original checkout. An existing prepared tree is reused only if its recorded
commit, patches and contents match. Choose another `--source` directory if needed.

## Run and install

Build AnybandUI separately, then point it at this engine. A separate user directory
keeps testing apart from normal saves:

```powershell
& ../AnybandUI/build/dev/game/AnybandUI.exe --engines-dir build/native/game --user-dir build/test-profile
```

To create a distributable engine package, including runtime libraries, game data,
license notices and corresponding source:

```powershell
python -B tools/package.py --build build/native --source build/engine --output dist/angband-4.2.6-windows-x64
```

The output directory must be new. Copy the package into `engines/` beside
`AnybandUI.exe`. Install only one package with the same engine/save identity.
Frontend settings and engine saves live outside the installation directory.

## Sound

On Windows, the engine plays Angband's official sounds directly. Enable **Use
sound** in AnybandUI's **Settings > Game rules** for the current character.
The setting is saved with the character. Keep `lib/sounds` and
`lib/customize/sound.prf` in the engine package. Playback uses Windows system
libraries; no additional audio DLLs or AnybandUI audio pack are needed.

## Checks

```powershell
Push-Location build/native/game
./anybandui-map-tests.exe -v
Pop-Location
python -B -m unittest discover -s tests -p "test_*.py"
```

Check printed native pass totals as well as exit status. See
[validation](docs/validation.md) for coverage and outstanding integration checks.

## Layout

- `src/`: adapter implementation and presentation policy.
- `frontend.cmake`: external frontend build entry.
- `patches/`: engine fixes, interfaces and generic build integration.
- `anyband-protocol.json`: copy of the frontend-owned contract.
- `vendor/`: pinned cJSON dependency and license.
- `tests/`: native map and packaging regression tests.
- `tools/`: source preparation, patch export and packaging.

[Architecture](docs/architecture.md) describes ownership and engine dependencies.
Retired prototypes and historical cleanup reports remain in Git history; they are
not part of the supported build. Build output and recovery backups are ignored.

GPLv2. Angband retains its licensing; cJSON is MIT-licensed. See `LICENSE`,
`docs/angband-copying.rst` and `vendor/cjson/LICENSE`.
