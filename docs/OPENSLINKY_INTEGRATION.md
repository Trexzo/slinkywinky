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



## R172: native event-dispatch and module-toggle safety audit

A second independent offline verifier, `tools/audit_openslinky_dispatch.py`,
examines the pinned C++ manager and optionally reads the user's private
candidate `decrypted files.zip`. It **never executes the original DLL**.
The optional private path validates the full extracted native `.text` SHA-256
and five specific disassembled function windows at RVAs `0x28480`–`0x28600`
before accepting any instruction-level observation.

To reproduce privately:

```sh
python3 tools/audit_openslinky_dispatch.py \
  --source references/OpenSlinky \
  --candidate-zip "/private/decrypted files.zip" \
  --json-out build/private_dispatch_audit.json
```

`python3 tools/audit_openslinky_dispatch.py` without the private ZIP is a
**source-only** audit, which is what public CI runs. CI also exercises
`tests/test_openslinky_dispatch_audit.py` with synthetic fixtures and does
not receive original code bytes, the private ZIP, or a runnable native image.

### Independently observed event-manager differences

| Boundary | Candidate decrypted original instruction window | Pinned C++ source | Classification |
|---|---|---|---|
| Event dispatcher (`0x28480`) | Reads event type directly, immediately indexes handler table at manager + `0x210` with type × 8 | Checks `type >= 27` before indexing | **Safety hardening** (retain; native would risk out-of-bounds access) |
| No-handler query (`0x284d0`) | Directly indexes the same handler table, without a range check | Treats `type >= 27` as handled | **Safety hardening** (retain, though out-of-range behavior differs) |
| Toggle queue (`0x284f0`) | Queues entries without checking the allocated list capacity | Rejects `*n >= 0x43` | **Safety hardening** (retain; preventing overflow is important) |
| Toggle application entry (`0x285d0`) | Begins by retrieving the manager and pending enable count | Returns early if world/player is missing | **Lifecycle hardening**; potentially changes when queued toggles apply |
| Registered module count | Original constructor uses 66 | Current constructor uses 67 | **Fidelity discrepancy** (see R171 table proof) |

The source contains **27** possible event slots and a null-terminated
handler-dispatch loop. Existing guards are not to be removed just to
imitate an unsafe original failure case. To get closer to legitimate
behavioral parity, compare valid event traces (`0..26`), module toggle
ordering and initial-world lifecycle in an isolated harness. Invalid
event types and excessive queue counts should remain fail-closed.

**Verification scope:** This checks the identities of five disassembled
native function windows and matching source-level constructs. It does
not establish cycle-level equivalence, all instruction semantics, native
injection success, safe unload, game-world packet ordering, or provenance
of an authenticated commercial original.



## R173: complete original .data section and relocation audit

A new read-only verifier, `tools/audit_openslinky_data.py`, checks the
*assembled data definitions* against the locally held private decrypted
candidate. It parses x64 COFF sections, symbols and absolute relocations;
it never links a DLL, starts Minecraft or loads an injector.

Pinned upstream's `src/data/data.s` can be assembled on a machine with
LLVM/Clang without any Windows runtime:

```sh
clang --target=x86_64-pc-windows-msvc -c \
  references/OpenSlinky/src/data/data.s -o reference_data.obj
python3 tools/audit_openslinky_data.py \
  --source references/OpenSlinky \
  --object reference_data.obj \
  --json-out build/source_data_audit.json
```

That **source-only** run is CI-compatible and confirms structural facts
(section length, absolute relocations, and append-only size). Do not publish
the assembled COFF object or the original decrypted binary; CI publishes
only the metadata-only JSON.

To compare against the private candidate locally, add
`--candidate-zip "/private/decrypted files.zip"` to the Python invocation.

### Independently verified results

| Measurement | Candidate original | Pinned new C++ source |
|---|---:|---:|
| Original `.data` section | **136,712** bytes (`0x21608`) | All original bytes represented |
| Source-assembled `.data` length | — | **136,840** bytes (`0x21688`) |
| Additional source data | — | **128** bytes (`0x80`) |
| 64-bit pointer relocations in original-sized prefix | **2,438** | **2,438/2,438** correct address targets |
| Non-pointer byte differences in original-sized prefix | 0 | **0** |
| Relocations in appended source suffix | — | **4** |

Method: Compile the pinned `data.s` to a COFF object without linking it.
Every relocation in the first `0x21608` bytes is verified by resolving
its COFF symbol using the published original-RVA symbol maps, taking local
object symbol offsets and relocation addends into account. The original
`.data` is separately authenticated against the fixed candidate SHA-256
`8aee98d473f5092686ea3ed080828bbb02500e4bbd8d89130a3ce7751176dea5`.
Both byte arrays have their *relocated pointer slots masked* before
comparing the remaining bytes; all 2,438 pointer values are then compared
separately against resolved original image addresses.

**Result: exact original-prefix data parity after relocation, not an
unconditional byte-for-byte executable match.** The 128-byte suffix
corresponds to new Chest Stealer options appended after the original
section; it is not native original evidence and requires separate
functional review. The 66-to-67 module registration discrepancy is
independent of this original-prefix data match.

The proof is strictly **static data parity** for one candidate, not proof of
in-game GUI, packet, module logic or lifecycle equivalence, or of verified
official Slinky source provenance.

