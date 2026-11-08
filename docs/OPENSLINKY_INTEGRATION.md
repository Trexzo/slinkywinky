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


## R171: optional read-only candidate binary verification

The new `tools/verify_candidate_binary.py` independently checks the user's
**local** candidate archive, without unpacking it onto disk or running any
embedded executable or script. From the ZIP, it checks the SHA-256 of the
decrypted `.text` and `.rdata`, compares both against the embedded DLL's
raw PE sections, hashes the mapped 176-byte Reach instruction segment,
and reads the manager allocation/count/copy/loop values from pinned instruction
locations. The whole `.text` section must match its pinned hash before
those instruction offsets are interpreted. It then parses the pinned C++
manager's descriptor count, table capacity and copied byte count.

Run on a private machine that already has the user's decrypted reference:

```sh
python3 tools/verify_candidate_binary.py \
  --candidate-zip "/private/decrypted files.zip" \
  --source references/OpenSlinky \
  --json-out build/private_candidate_audit.json
```

This records only numerical results and hashes, never raw bytes. **Do not
upload the private ZIP or any other commercial image to GitHub Actions.**
The CI suite uses synthetic generated PE fixtures for verifier unit tests and
does not possess the private decrypted archive.

Locally verified on the supplied candidate: section consistency passes,
the manager shows **66** entries and **528** copied bytes, while the source
shows **67** entries and **536** copied bytes. The default command reports
this known discrepancy with exit 0, allowing inspection of its JSON; append
`--strict` to return exit **2** for non-parity. Exit 1 indicates input or
verification failure. Nothing in this check proves official provenance,
completed native injection or real gameplay equivalence.



### Exact descriptor-table reconciliation (R171)

Reading the candidate decrypted DLL's `.rdata` at RVA `0x14f810`
produces 66 native descriptor pointers. Resolving every consecutive `.quad`
in the C++ `ptr_to_aim_assist_module_descriptor` array through its
`docs/global-symbol-map.json` produces 67 pointers. The 66 original
pointers match in exact order **after removing one new C++ pointer**:

- **C++ index 17:** `chest_stealer_module_descriptor`, RVA `0x2f5288`.
- All 66 remaining pointer values are identical to the candidate decrypted
  table at their respective positions (66/66; 0 mismatches).
- This confirms a specific insertion, not a wholesale table reconstruction
  failure. It does **not** imply all 66 module handlers or GUI settings execute
  identically to the original.

The original manager code at `0x28000` allocates/counts 66 descriptors and
copies 528 bytes. The present upstream manager allocates/counts 67 and
copies 536 bytes, with matching 67-entry queue capacities. A parity patch
would have to update the C++ pointer table and *all corresponding* manager
count, copy-size, loop, and queue-capacity expressions together, followed by
new link/runtime tests. Merely changing `manager->count` to 66 or dropping
the last entry is **not** sufficient: Chest Stealer was inserted at index 17.

The optional private binary verifier now detects this insertion mechanically
and rejects any unexpected table reordering or more-than-one-entry drift.
CI tests the source parser and comparison logic with generated fixtures;
**the actual 66/66 comparison ran locally** with the user-supplied candidate
archive and is intentionally not reproducible by public CI without that
private input.

