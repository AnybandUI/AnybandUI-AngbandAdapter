# The SDK in sdk/ is a pinned, reviewed snapshot for this engine build.
# Normal builds use it directly: cmake -S . -B ../../build/vanilla-adapter.
# migration-history.ps1 records the original investigation, not the build path.
# A future engine version needs a deliberate port of the presentation sources
# and SDK, followed by the semantic tests; do not just replace frontend-build.h.
throw 'Use CMake with the pinned SDK. See README.md for the reproducible build and test commands.'
