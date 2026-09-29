# Architecture

```text
AnybandUI desktop application
    | Anyband Protocol JSON requests, snapshots and events over stdin/stdout
Engine process
    adapter: protocol, input presentation, revisions, map records and feedback
    | Angband C functions, callbacks and events
    Angband: gameplay rules, RNG, knowledge, legal actions and saves
```

## Build and state ownership

`frontend.cmake` is included through Angband's `ANGBAND_EXTERNAL_FRONTEND` option.
It adds the adapter entry point and vendored cJSON to the engine build, then stages
its manifest and game data. Ordinary platform frontends use separate builds.

`src/main-anybandui.c` includes private feature headers in one compilation unit.
Those headers share session state; they are not public interfaces. Engine access
runs synchronously on the main game thread at input boundaries. One process owns
one game session. The UI receives scalar snapshots and identifiers, never pointers.
Request revisions, prompt contexts and item handles reject stale interactions.

The engine interface is source-version-specific. Updates to the pinned Angband
revision require reviewing both adapter dependencies and the engine patch series.
The independently versioned Anyband Protocol belongs to AnybandUI; every listed
capability is required.

## Input and presentation

Inventory, equipment and rest commands wrap the existing command table. Birth,
spells, item selection and other prompts use synchronous callbacks. Stores invoke
the existing menu action handler, retaining engine pricing and bookkeeping.
World-coordinate mouse events go through the ordinary movement/targeting paths.
Original terminal screens remain available for interactions still using them.

The engine supplies known grid data and terrain, trap, object and actor drawing
layers. The adapter assembles its map records. Read-only camera queries must not
reveal unknown information, update memory or consume gameplay RNG. Artwork imports
are restricted to presentation directives while reusing the engine preference parser.

Combat, projection, target selection and lifecycle events report actual outcomes.
The adapter pairs movement observations and assembles animation feedback, preserving
visibility and hallucination filtering. Final snapshots alone are insufficient to
distinguish walking from teleportation or recover projectile paths.

Descriptions, character rows and equipment comparisons reuse engine calculations.
Tuning validation temporarily publishes parser output and restores the live pointer;
equipment comparisons also depend on careful state restoration. These operations
are synchronous and are not thread-safe or independent of the engine version.

## Engine patches

Apply `patches/series` in order to the commit in `upstream.json`:

1. Correctness fixes: object-power arithmetic, cancelled purchase cleanup,
   confused mouse movement and the MSVC UTF-8 option.
2. Frontend interfaces: known-map queries, drawing observations, interaction
   callbacks and events.
3. Generic external-frontend build support.

Official Angband sound support and assets are preserved. The custom AnybandUI
sound events and playback system have been removed.

`tools/export_patches.py` regenerates this series and `docs/surface.json` from an
explicitly prepared engine tree. Measurements compare with the pinned upstream
release. They do not include adapter-owned implementation files.
