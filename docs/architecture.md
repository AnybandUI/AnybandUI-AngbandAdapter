# Architecture and upstream boundaries

```text
AnybandUI application (unchanged full-v1 consumer)
               | versioned JSON over stdin/stdout
Engine process | no DLL loading or memory inspection
  external adapter: wire messages, identity, revisions, native presentation,
                    tuning, artwork policy, journal, keybindings, packaging
               | Angband-specific source interface
  Angband: rules, RNG, knowledge, legal actions, existing command/input/event APIs
```

The interface between adapter and Angband is a source-level integration compiled
against a pinned engine revision. It is not a stable binary ABI or a protocol the
Angband maintainers must adopt. A future engine release can require adapter work;
that work stays here. The frontend protocol and complete capability requirement
are unchanged. The same engine/save identifiers preserve existing compatibility.

## Reductions implemented

Inventory and equipment presentation callbacks are installed through the existing
public command table. The engine's keymap expansion, menus, command prerequisites
and normal item action paths remain in charge. When the existing native-browser
negotiation is disabled, the original functions still run. No new fallback or
reduced integration mode was introduced.

Rest prompt context is attached by wrapping the existing rest command callback,
which still parses and validates the response. Direction context wraps the
existing direction-input callback. This removes core-only state that described
the adapter's presentation, without duplicating movement or rest rules.

Artwork import filtering lives in the adapter. The engine exposes its existing
preference parser factory; the adapter admits artwork directives and constrained
includes, while the real parser retains conditional expressions and visual
mapping interpretation. It does not import commands, inscriptions, colours or
window flags into a presentation query. A regression exercises contaminated
artwork preferences and verifies keybindings and gameplay state are unchanged.

The external build entry point is generic: `ANGBAND_EXTERNAL_FRONTEND` names a
directory with `frontend.cmake`. Angband supplies its executable/core targets;
the frontend adds its entry point, dependencies and staging. Ordinary platform
frontends remain separate builds. There is no AnybandUI name, cJSON dependency,
manifest or network fetch in the upstream patch. The adapter vendors its small
JSON dependency for reproducible offline builds.

## Engine changes deliberately retained

Read-only known-map queries prevent hidden-information disclosure and gameplay
RNG consumption. Presentation observation preserves actual terminal visuals,
including hallucination, rather than deriving them from a second rules engine.
Structured descriptions and character rows reuse existing engine calculations.
Object inspection exposes its existing numerical helpers and prose section
boundaries; the adapter owns titles, combat rows and JSON layout. Level-feeling
strings stay engine-owned, while joining them into frontend text lives outboard.
Store sessions receive the existing menu and invoke its original action handler,
including stock selection, purchase/sale and bookkeeping. The adapter supplies
the chosen row after delayed input flushing and formats confirmation text from
the engine-computed price.
Birth and spell browsing have direct callers outside the general command table,
so their substitution hooks remain. World-coordinate mouse input uses the ordinary event path with an explicit
coordinate-space flag and wider coordinates. That replaces separate click-at,
aim-at and targeting relocation implementations. Screen-coordinate mouse events
retain their original conversion and edge scrolling.

Combat outcomes, teleportation versus movement, projection geometry, confirmed
targets and death boundaries cannot be reliably recovered from final snapshots.
They retain explicit observations through Angband's event system. Monster
walking emits paired point events around the actual swap; teleport departure
and arrival use point events at the original visibility boundaries. The adapter
pairs movement observations and constructs animation data, including the same
visibility and hallucination filtering. It does not infer moves from snapshots. The sound cue
event is separate from legacy playback preference; the engine unit suite checks
both behaviours. Quantity and effect context remain borrowed for the synchronous
input call that needs them.

Packing those declarations into one header would not reduce these obligations.
Screen scraping, runtime patching, copied pricing/combat rules, and dropping
capabilities were rejected. Replacing the complete input system or asking for a
stable plugin ABI would be a larger upstream refactor, not a small integration.

## Review series

1. `01-correctness.patch`: wide object-power arithmetic, cleanup after cancelled
   purchases, intended direction for confused mouse movement, MSVC UTF-8 option.
2. `02-frontend-interface.patch`: queries, observations and remaining native
   interaction boundaries, including sound-cue unit assertions.
3. `03-external-build.patch`: generic external frontend build entry point.

`tools/export_patches.py` measures the complete delta from the pinned release,
not just the most recent change. Counts include test changes; owned adapter code
is reported separately. A fresh archive plus strict patch application verifies
that there is no reliance on the developer's Angband working tree.

## Constraints

The engine remains single-threaded. The existing tuning validator temporarily
publishes parser output and restores it at synchronous input boundaries. The
equipment comparison similarly relies on careful state restoration. These are
explicit adapter dependencies; extraction does not turn them into thread-safe
or version-independent APIs. The private implementation headers preserve the
single translation unit's state ownership; they are not installed public headers.

Upstream uses C99 in its actual CMake targets. New adapter code is formatted with
four-column tabs, K&R function braces, spaced operators and an 80-column target.
Existing engine code is not reformatted wholesale merely to match the adapter.

The current map boundary uses generic drawing layers; see [the drawing-hook update](map-hook-report.md). The adapter owns map_visual and wire-record assembly.
