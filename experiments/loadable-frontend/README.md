# Loadable frontend investigation — measured prototypes

28 September 2026. Two working Windows prototypes, not an upstream-ready patch or a claim of complete feature-parity certification.

## Result

The existing adapter can run in a separately loaded DLL while all Angband engine code and state remain in the executable. The real AnybandUI Connection/BirthPanel scenario works through that boundary. A Windows GUI-subsystem executable containing the existing graphical frontend can select the DLL before graphical initialization. An independent reference frontend also works through the same loader and uses stock game-event callbacks.

The end-user model is therefore technically feasible: a future official Angband executable can load a compatible frontend DLL supplied with AnybandUI. The executables built here are experimental custom builds, not official releases. Upstream would have to adopt and enable the mechanism in its releases.

The smaller implementation is a build-matched named-export interface. It is not a small stable API: the export generator exposes 2,316 names from the core archive, and the adapter consumes 357. That compatibility obligation is the major tradeoff.

## What was built

### Explicit typed table

`engine-api.h` declares 357 slots: 275 function addresses and 82 addresses of globals/callback slots. `engine-api.c` supplies them from the engine executable. The DLL contains the existing integration plus cJSON, not an engine copy.

The DLL receives that table only after the loader verifies the descriptor version, build identity, table size and run entry. Module ownership lasts for the process lifetime because engine hooks can retain callbacks. One game per process remains the supported model.

The table header and initializer alone occupy 784 physical lines in this proof. This is a useful baseline but an unattractive route for minimizing upstream code. Generating the table does not make its size or maintenance disappear.

### Named exports

`export-engine.ps1` enumerates externally linkable definitions from the engine archive at build time. CMake supplies the generated `.def` file to the executable's linker. It produces 2,316 exports on both tested architectures.

`export-loader.c` loads the selected DLL and checks its descriptor. Its lookup callback uses ordinary GetProcAddress on the current executable. The adapter builds its typed bindings privately from those deliberately exported names. There is no process-memory scanning, injection, binary patching, symbol interception or source rewriting of Angband.

The upstream-facing contract is in `export-api.h` and `frontend-stream.h`. It contains no inventory schema, targeting protocol, cJSON types or AnybandUI identity. The prototype passes generic stream callbacks for reuse by the native tests; transport policy could instead be owned entirely by a production module.

`reference-plugin.c` contains no AnybandUI code. It resolves the stock event registration/signalling functions, registers a callback, signals it, removes it and checks it is not called again. This proves the loader is usable by an independent module, not that it is a complete stable SDK for arbitrary clients.

## Actual sizes, with the accounting kept separate

At the first measured source snapshot:

| Component | Physical source lines |
| --- | ---: |
| Typed table header and initializer | 784 |
| Named-export loader | 40 |
| Named-export interface header | 16 |
| Shared stream declaration | 12 |
| Windows startup wrapper | 61 |
| Wrapper compiling the existing WinMain under another name | 3 |
| Export enumeration script | 18 |
| Generated export declaration file, each architecture | 2,317 |

The named-export loader/interface totals 68 lines; startup and export generation are additional. Build/fingerprint logic, tests, packaging and platform support are also additional. The CMake file contains both experiments and test targets; it is not a minimal proposed upstream build patch. `evidence/surface.json` records the final file-level counts.

The generated export file is an output, but its 2,316 exported names are explicitly part of the interface cost. A short generator does not establish a small API. The adapter-private table/bindings remain large; they are supplied with AnybandUI rather than shipped as an Angband frontend implementation.

**No complete sub-500-line upstream solution has been demonstrated.** These proofs use engine revision d18ac150d so that the loading boundary could be tested without simultaneously changing gameplay/presentation facilities. That revision's full difference from 4.2-release is 57 files, +803/-351 = 1,154 changed lines including the test. It is the previously rejected larger experiment, retained as a test reference, not the proposed reduction baseline.

The next reduction implementation must return to the smaller f3cd9246c baseline (428 changed lines including the test; the previously quoted 413 excluded it), or stock release, and re-establish every required behavior. These loader measurements cannot be substituted for the whole patch against release.

## Behavior and validation

The adapter was copied into the isolated prototype. Its engine references were rebound to explicit pointer-table accesses. A few local variable/member names were disambiguated; strings, game calculations and protocol behavior were preserved. `prepare.ps1` records that adapter-only transformation. It is experimental assembly tooling, not a proposed upstream source transformation.

Both table and named-export designs passed the existing native scenarios:

| Scenario | Table | Named exports |
| --- | --- | --- |
| Birth, map/camera, tilesets, targeting, typed prompts, unknown/known wand aiming, throwing, committed turn, save/close | 29 requests | 29 requests |
| Store purchase/sale, home stash/retrieve, affordability/cancellation/inscriptions, structured documents, map RNG/knowledge checks | 32 requests | 32 requests |
| Load saved session and close | 3 requests | 3 requests |
| Cancel birth | 3 requests | 3 requests |
| Actual AnybandUI Connection and BirthPanel | 12 requests, 23 frames | 12 requests, 23 frames |
| Wrong build identity and wrong interface size | Rejected before run | Rejected before run |

The Windows GUI-subsystem executable also passed the 29-request native scenario with each interface. Its test-only `--frontend-test` route supplies in-process fixture callbacks; it does not simulate user clicks or prove the normal production pipe session end-to-end.

The regular `--frontend` route was separately exercised with the independent reference module, including output through the stream callback, and with an incompatible module. The latter returned status 72 before running the module.

The existing graphical frontend is compiled into the executable and remains the no-option fallback. It was not launched or interactively tested. No computer use was performed.

Counts of assertions vary with generated map sizes: about 300 checks for a gameplay scenario and about 3,150 for the store/presentation scenario are not thousands of independent test cases. Logs in `evidence` retain the actual counts and requests.

The dynamic library import inspection shows runtime/system imports, not a second engine library. All engine calls are supplied by the host table or deliberate executable exports.

## Compatibility and responsibility

This is intentionally an exact-build experiment, not a promise that a DLL for one release will work with another build of that release. The metadata hashes core build artifacts, engine headers, public plugin declarations, compiler identity/version, word size and the selected configuration. The typed-table variant additionally hashes its ordered table declaration. The named-export identity does not depend on AnybandUI's private table.

This conservative fingerprint will reject some compatible rebuilds. A production SDK needs a documented identity scheme and published matching headers/build metadata. Distro variants and compiler/runtime choices must be addressed; a source version string is not enough. Module selection belongs in AnybandUI so users do not have to locate DLLs manually.

The module is trusted native code with access to engine internals, not a sandbox. Existing engine allocation/free functions remain paired across the boundary. The DLL stays loaded while callbacks may refer to it. Cross-session reentrancy, hot unloading and multiple games within one process are not supported by this experiment.

Official responsibility would include loader/startup, build exports and SDK identity, necessary engine observation hooks, release packaging, documentation and verification. Our responsibility would include the adapter DLL, protocol, AnybandUI behavior and compatibility support for official builds. If official packages omit the loader or exports, the end-user objective is not achieved.

## Remaining limits

- The final minimal engine hook set has not been implemented or measured.
- Broad combat/death/postmortem/play-again, every spell/device/recall interaction and long travel campaigns were not recertified here.
- Full real-client child-process transport testing was not rerun. Previously quarantined helpers/executables were not restored or retried.
- Windows x64 native hosts and the Windows x86 graphical executable path were tested. Linux/macOS, signing, official distribution and antivirus acceptance are unverified. No new quarantine was observed during these successful builds/tests.
- The private typed bindings use C23 `typeof` for the prototype. The loader/public declarations do not inherently need it. A production implementation must respect the engine's chosen C standard and house style.
- Native modules still need direct observations for transient combat/movement/teleport events, reliable prompt context and read-only map presentation. A loader does not eliminate those requirements.

## Reproduction and retained locations

Canonical source/evidence copy: `AnybandUI-AngbandAdapter/experiments/loadable-frontend`.

Actual verified build workspace: `C:/Users/developer/.codex/visualizations/2026/09/27/01a0e4cd-f1d4-76d0-b047-4aefecb75513/frontend-plugin`.

The proof consumes the existing `Adapter/build/direct-frontend` x64 engine archive and `Adapter/build/direct-stock` x86 core objects. Those must match the source revision above. `prepare.ps1` assembles the adapter-private bindings from the current reference object; run it again if those inputs change.

From a matching Visual Studio native tools environment, configure this directory with CMake/Ninja into `build` (x64) or `build-x86` (x86). The `.def` output is generated as a build dependency. The x86 main targets are `angband-exports`, `export_frontend`, and `reference_frontend`; the unused console fixtures are excluded from its default build because the Windows sound implementation belongs to the graphical frontend.

For native scenarios set ANGBAND_TEST_FRONTEND to the absolute frontend.dll or export_frontend.dll path, then run frontend-session.exe or frontend-export-session.exe with --data-dir and a new disposable --user-dir. DIRECT_SESSION_STORE, DIRECT_SESSION_LOAD and DIRECT_SESSION_CANCEL_BIRTH choose the additional existing scenarios. Loading reuses the corresponding saved profile.

AnybandUI has opt-in Windows-only test targets `anybandui-plugin-ui-tests` and `anybandui-export-ui-tests`, selected with ANYBANDUI_PLUGIN_PROBE and the existing ANYBANDUI_ADAPTER_BUILD. Production client behavior and packaging are unchanged. The pre-existing edits to test_transport.cpp were not modified.

## Recommendation

Reject the explicit upstream table as the leading minimization route: it recreates a large interface inventory inside Angband. Keep the named-export design as a concrete candidate for intentionally build-matched frontend modules. It makes the official-executable distribution model possible without making Angband ship our adapter or protocol.

Do not call this a tiny stable API or declare the overall patch reduced. First port this mechanism to the smaller engine baseline, remove terminal-controller dependencies where behavior permits, and measure the complete release diff. The acceptance question for upstream is whether it wants build-matched native frontend modules at all; if not, the built-in semantic frontend remains the alternative and its full implementation must be counted.
