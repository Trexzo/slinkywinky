# OpenSlinky pinned-native reference integration (R170 / independent track)

## Purpose and boundaries

The original `main` branch currently contains only the README; SlinkyWinky's
working C++ and Java integration is on the open draft PR #1 from
`reconstruction/r1-core-fidelity`. This integration branch forks the R169 head
(`3b1d1478901d069c1a870ee0881804e8f8d5fe9c`) and deliberately does not
modify its native runtime, loader, menu or gameplay modules.

The external project `Skid-Technology/OpenSlinky` is linked as a **pinned Git
submodule** at commit `fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4`.
This is a full navigable source reference, not a claim of code ownership, a
verified working runtime, or a source redistribution grant. No extracted
commercial binary/data tables/assets or unknown injector are copied into our
repository. The reference has no visible LICENSE file at the pinned revision.
Do not copy, vendor, package or merge the recovered source/assets without
resolving the rights and provenance questions.

## Inspect locally (no execution of reference code)

    git submodule update --init references/OpenSlinky
    python3 tools/audit_openslinky_reference.py --json-out build/reference_audit.json

The audit checks module source coverage, recovered symbol-map sizes, catalog
correspondence and the module-registration discrepancy. The regular SlinkyWinky
CMake builds remain unchanged. The workflow checks out the external source but
**does not compile or execute the DLL/injector**.

## Confirmed static observations at the pin

- 66 per-module C++ logic files in `src/logic/**`.
- 1,338 function symbol aliases in `docs/symbol-map.json`.
- 7,788 global symbol aliases in `docs/global-symbol-map.json`.
- The new C++ manager uses `0x43` (67) registered entries, including
  0x218 bytes copied from its descriptor table.
- An independent previous static review of the candidate decrypted PE found
  0x42 (66) entries / 0x210 bytes in its corresponding module manager.
- The candidate original PE's official provenance/version is not verified.
- Native hook installation, GUI lifecycle, module events, in-game behavior,
  clean unloading and 1:1 equivalence are **not** proven by this integration.

## Parallel development ownership

- **R-series branch / PR #1:** original Minecraft 1.8.9 runtime, client input,
  retained GUI, independent tests and integration safety.
- **OpenSlinky integration branch:** external native source inventory, static
  data/logic comparison, explicitly documented divergences and isolated
  reference validation. Do not touch R-series runtime files on this branch.
- Merge only after independent review, permission/provenance clearance for
  any actual code reuse, and passing the original branch CI.

## Next acceptance gates

1. Resolve upstream source/assets licensing and original-binary provenance.
2. Pin both C++ and Java candidate references and record cryptographic hashes.
3. Construct address-level source-to-decrypted comparison for startup, hooks,
   GUI handlers, settings and module-registration control flow.
4. Build fixed-state decision/event traces without invoking live gameplay.
5. Run isolated original-client render/input tests and safe teardown checks.
6. Keep any remaining differences recorded as deviations, not as 1:1 claims.
