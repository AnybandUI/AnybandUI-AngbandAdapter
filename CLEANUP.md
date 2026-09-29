# Folder cleanup — 29 September 2026

The adapter's active source, patches, tests, tools, vendored dependency and
experiments remain in place. All 430 original source/configuration files were
checksum-verified unchanged after cleanup. The project is still not in source
control; no repository was initialized and no commits were made.

The former multi-gigabyte build/archive collection was reduced to approximately
351 MiB of readable files. Most of the remainder is deliberate recovery storage:

- recovery/source-before-cleanup.zip: conventional verified source backup.
- recovery/historical-files.zip: deduplicated historical sources, packaged source,
  saved profiles, data and evidence removed from build/dist (about 280 MiB).
- build/minimal-native/game: original, manually tested engine and data.
- build/external-native/game: freshly built engine and passing map-test executable.
- build/manual-test-profile, build/gui-profile, build/vanilla-package-profile:
  profiles left at their original locations.
- build/verification-20260929-standalone: current reports, logs and preview.

Obsolete compiler output and executable copies were deleted. Historical files
outside the retained directories are recoverable using recovery/restore_file.py;
see recovery/README.md for details. Historical documentation may still mention
paths now represented inside that archive. No quarantined helpers were restored.

The installed AnybandUI engine and normal user saves were not modified. Its
executable still matches the preserved manually tested engine. The retained fresh
map-test executable passed 5/5 after cleanup.

Build caches have been removed. Reconfigure before the next build; the adapter
README's configure command can recreate build/external-native with its retained
game output. Previously unreadable test directories were left untouched (132
paths); their unknown contents are excluded from the measured size.

See recovery/cleanup-results.json for actual deletion totals and failures, and
recovery/archive-hashes.json for archive checksums. Keep recovery until the
project and any required historical work have durable backups/source control.
