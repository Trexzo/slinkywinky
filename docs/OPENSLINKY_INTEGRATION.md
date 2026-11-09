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



## R174: independent native event-replay model (first executable integration component)

`reference_compat/` introduces an **originally authored standalone C++17
event-replay component**, separate from the R-series runtime and from the
third-party DLL. It models the most concrete, verified dispatcher behavior:

- 27 indexed event types and an upper bound of **66 candidate-original modules**;
- five categories, event subscription and descending per-slot priority;
- event **4** as a cancellable pre-disable notification;
- event **5** as a toggle notification (enable batch then disable batch);
- handler-list rebuild before enable notifications and **after** disable
  notifications, so a module can receive its last disable event;
- world-and-player availability gate, queueing while the game is at its menu;
- rejecting duplicate queued toggles, out-of-range event types and
  invalid module descriptors, plus terminal stop/queue drain.

Build without Minecraft, Java, the upstream DLL, native data, or Windows SDK:

```sh
cmake -S reference_compat -B build-reference -DCMAKE_BUILD_TYPE=Release
cmake --build build-reference --config Release
ctest --test-dir build-reference -C Release --output-on-failure
```

The existing `CMakeLists.txt` and R-series source are unchanged. The
OpenSlinky audit workflow now runs this independent C++ target on both
Windows and Ubuntu, in Debug and Release, using checked failures instead
of compile-disabled `assert` expressions.

**Important differences from a 1:1 native engine:** The original callback
sort uses `qsort`, so **tie ordering is not proven**. This replay model
uses a deterministic descriptor-order tie-breaker, which is a test
convention rather than evidence of native behavior. It deliberately
rejects duplicate/contradictory pending requests and stops delivering
events on teardown; neither is asserted to match every native edge case.
The model carries no JNI/JVMTI hooks, packet transformations, native
module implementations or gameplay actions. Passing its tests validates
only the explicit event and lifecycle contracts, not 1:1 gameplay.

Next: derive a 66-module callback subscription/priority fixture from the
private candidate DLL *locally* and use this model to replay a complete
catalog without uploading the decrypted program or importing unlicensed
third-party source.



## R175: real 66-module event fixture and executable replay

The local-only exporter `tools/export_candidate_event_manifest.py` reads the
user-supplied candidate DLL's **verified** decrypted `.rdata` and `.data`
sections. It reads the 66 native module descriptors at the original
`.rdata` table (RVA `0x14f810`), identifies their names, categories, initial
enabled flags and every indexed callback pointer at descriptor slots
`+0x48, +0x58, ..., +0x1e8` (27 event slots), including signed
per-event priority from each slot's companion `+8` field.

It emits a **local** TSV metadata fixture and a JSON summary, not the
DLL, native instructions, data tables, executable module callbacks,
pointer addresses or assets. The candidate ZIP is hash-checked before
extraction.

```sh
python3 tools/export_candidate_event_manifest.py \
  --candidate-zip "/private/decrypted files.zip" \
  --tsv-out build/private_candidate_modules.tsv \
  --json-out build/private_candidate_modules_audit.json

cmake -S reference_compat -B build-reference -DCMAKE_BUILD_TYPE=Release
cmake --build build-reference --config Release

# On Linux:
./build-reference/slinkywinky_event_fixture_runner \
  build/private_candidate_modules.tsv --expect-candidate

# On Windows, use build-reference/Release/slinkywinky_event_fixture_runner.exe
# when using the Visual Studio multi-config generator.
```

**Never commit the generated private TSV or decrypted ZIP.** GitHub CI
contains only a separate, synthetic three-module TSV and regression tests
for malformed original addresses, duplicated pointers, invalid categories,
and the fixed private SHA-256 gates.

### Candidate-derived and locally replayed counts

| Module category | Original candidates |
|---|---:|
| Combat (`0`) | 17 |
| Move (`1`) | 10 |
| Visual (`2`) | 15 |
| Block (`3`) | 7 |
| Utility (`4`) | 17 |
| **Total** | **66** |

The original candidate declares **171 non-null handler subscriptions**
across **27 events**. The C++ event fixture runner registered all 66
modules, applied their enable requests only after world admission and
verified the resulting **per-event callback histogram** matched:
`[24,5,6,10,3,25,7,4,4,3,3,2,1,5,10,6,3,9,20,1,1,3,2,1,1,5,7]`.

**Chams** and **No Hurt Cam** appear in the original descriptor catalog
but have no native event callback in the 27-slot descriptor range.
This does **not** establish that their effects are missing; their behavior
might be implemented via engine/Java hooks outside the per-module dispatch
table and needs separate analysis.

**What this proves:** The independently authored replay engine can
load and schedule exactly the original candidate's module registration
and event subscription metadata, including priority integers. What it
**does not prove:** That any of the 66 module algorithms, JNI/JVMTI
hooks, packet processing, GUI rendering, and native teardown work
inside Minecraft. The candidate original's official commercial
provenance is not authenticated.



## R176: original-derived callback ordering and five-stage lifecycle replay

Added `reference_compat/tools/audit_candidate_lifecycle.cpp` as an
**independent C++17 test oracle**. It reads the existing metadata-only TSV
produced by R175; it neither compiles nor runs OpenSlinky or any original
DLL. Existing R-series runtime, CMake and loader files remain untouched.

For every one of the 27 event slots, the tool validates:

- **Exact subscription membership:** each enabled, subscribed module appears
  once and only once; each disabled or unsubscribed module appears zero times.
- **Priority constraints:** callback order is non-increasing by the recovered
  per-slot signed priority. Ties are intentionally not validated against an
  original qsort order (the original source provides no stable tie guarantee).
- **Dispatch receipt:** each callback is delivered once, in the same order
  as the model's current handler list; no additional recipients are admitted.
- **Lifecycle boundaries:** repeat all 27 assertions at initial registration,
  after all 66 modules are admitted into a world, after disabling every
  odd-indexed module, after disabling all modules, and after reenabling
  every third module.
- **Event-5 semantics:** verify the notification count and state flag
  before handler-list rebuild in both disable batches; enable notifications
  use the newly rebuilt handler list.
- **World/player admission:** deny toggles before world and player both
  exist, preserving pending requests; no callbacks after terminal shutdown.

For the private candidate TSV, the all-enabled phase also checks the
independently extracted 27-slot histogram (171 callbacks in total).

To run on a machine already holding the local private fixture:

```sh
cmake -S reference_compat -B build-reference -DCMAKE_BUILD_TYPE=Release
cmake --build build-reference --config Release
./build-reference/slinkywinky_event_lifecycle_test \
  build/private_candidate_modules.tsv --expect-candidate
```

When using a Visual Studio multi-config generator, substitute
`build-reference/Release/slinkywinky_event_lifecycle_test.exe`.

**Verification:** Locally, R176 passed on both the synthetic 3-module
fixture and the original-derived private **66-module / 171-callback**
manifest, with GCC and Clang at nonoptimized and optimized build levels.
The isolated CTest suite covers the synthetic successful replay plus a
**negative duplicate-subscription fixture**, which is expected to fail.
The Windows and Ubuntu reference compatibility CI builds both Debug and
Release. Only the synthetic fixtures are committed; the private candidate
TSV and decrypted ZIP are not published.

**Boundaries:** These tests verify the independently authored replay
model's behavior under actual candidate **metadata**. They do not run
the real module callback code, nor establish original native hook
fidelity, Minecraft gameplay behavior, pixel-perfect GUI, or official
commercial provenance. The event-5 ordering is derived from the
candidate C++ reconstruction and remains a candidate until equivalent
traces from an authenticated original are available.


## R177: execute one first-party module callback through the recovered event path

The first concrete **module-execution bridge**, `reference_compat/include/slinkywinky/module_execution_bridge.hpp`,
uses R174's bounded `EventReplay` to route a candidate event to real
first-party SlinkyWinky code. The current binding is **Reach**:

- Candidate native descriptor offset `+0xc8` corresponds to **event slot 8**.
- A `Reach` metadata record with combat category and event-8 subscription
  can explicitly bind to `slinkywinky::ReachController::evaluate` from the
  existing `src/core.cpp`, compiled into the standalone R177 test target.
- The adapter only runs explicitly bound implementations, even when other
  metadata entries claim subscriptions. It reports the exact count of
  implemented event bindings instead of treating registrations as working
  modules.
- The event envelope contains an **offline** typed Reach input/settings and
  optional distance result; it cannot access a game process, JNI, rendering,
  packets, or physical inputs.
- World and player availability, valid event number, payload type, active
  callback reentrancy, duplicate binding, and stop state all fail closed.
  A thrown callback clears the active dispatch frame.
- The test executes and checks the first-party 9-tick cooldown, output
  distance, callback priority order, Reach disable/re-enable, world-state
  reset, malformed input and safe teardown. It also verifies that the
  same event's other registered, unimplemented modules are **not**
  automatically interpreted as functional.

The tests build through `reference_compat/CMakeLists.txt`, where the existing
`../src/core.cpp` is linked directly (not copied, forked or substituted
with the third-party code):

```sh
cmake -S reference_compat -B build-reference -DCMAKE_BUILD_TYPE=Release
cmake --build build-reference --config Release
ctest --test-dir build-reference -C Release --output-on-failure
```

**Acceptance claim:** One first-party offline decision model is invoked
through a metadata-backed event slot and lifecycle gate. This is the
beginning of an execution interface, *not* the original 66 module
implementations, original DLL code, JNI/JVMTI callbacks, working gameplay
modification, or 1:1 Slinky equivalence. Slot mappings are based on
the pinned candidate, whose commercial provenance is not authenticated.

**Next measurable gate:** Register the full private 66-module TSV into this
bridge and count explicit, tested executable bindings separately from the
171 native callback subscriptions. Then add a second independently authored
offline module adapter and differential event fixtures without taking over
the other chat's runtime branch.

## R178: full original catalog to first-party execution coverage

R178 adds a strict metadata-only TSV loader at `reference_compat/include/slinkywinky/candidate_catalog.hpp`.
It validates module identity and order, categories, signed priorities, 27 event slots and duplicate subscriptions.
In original-candidate mode it requires exactly 66 module records, 171 native descriptor callback subscriptions,
2 descriptor modules without event callbacks, and the 27-slot reference histogram.

The independently written `ModuleExecutionBridge::coverage()` exposes native registrations separately from
actually installed first-party offline callback bindings. `bind_reach_from_catalog()` locates the module by
its validated name, combat category and event slot, never by an assumed numeric position.

The executable `reference_compat/tools/audit_implementation_coverage.cpp` loads the complete catalog,
binds the existing first-party Reach decision function, runs its offline event 8 callback, validates
world admission, cooldown, disable behavior and per-event registration coverage, and emits numbers-only JSON.
CI uses a synthetic three-module fixture and two negative fixtures for duplicate subscriptions and an
incorrect Reach slot. Windows and Ubuntu, Debug and Release, are exercised through existing reference CI.

Independent local inspection of the private candidate confirms Reach at descriptor index 54,
combat category 0, exactly one callback at event slot 8, priority 0. The fixture loader does not
hardcode that descriptor index.

To reproduce on a private machine after generating the TSV with the R175 exporter:

```sh
cmake -S reference_compat -B build-reference -DCMAKE_BUILD_TYPE=Release
cmake --build build-reference --config Release
./build-reference/slinkywinky_implementation_coverage_audit \
  build/private_candidate_modules.tsv --expect-candidate \
  --json-out build/private_implementation_coverage.json
```

Visual Studio multi-config builds use `build-reference/Release/slinkywinky_implementation_coverage_audit.exe`.

### Implementation accounting

| Measure | Status |
|---|---|
| Declared modules | 66 |
| Modules with native descriptor callbacks | 64 |
| Registered native event subscriptions | 171 |
| Actually bound first-party offline callback implementations | 1 (Reach) |
| Native event subscriptions without an implementation binding | 170 |
| Original algorithms executing in an injected Minecraft client | Not demonstrated |
| Pixel-perfect GUI, native hooks, packet callbacks and teardown parity | Not demonstrated |

These counts measure registration coverage, not an equal-weight percentage of client functionality.
The Reach decision runs in an offline simulated event envelope only. The private reference ZIP and
generated metadata fixture remain local. Unlicensed upstream code and proprietary assets are not
copied to this repository. PR #1's independent Minecraft runtime is untouched.

## R179: second independent offline module decision — No Jump Delay

Independent review of the user's locally held decrypted candidate confirms
**No Jump Delay** at original descriptor index **45**, movement category **1**,
event slot **17**, signed event priority **100**. Its native handler is
a per-player-update operation that asks the game to clear the jump delay
when the in-game flag is set.

The new first-party `reference_compat/include/slinkywinky/no_jump_delay_controller.hpp`
contains a pure policy: do nothing out of game; while in game, propose
`jump_ticks = 0`. It never accesses the JVM or modifies Minecraft.

`ModuleExecutionBridge` now has typed event-17 `JumpDelayTickPayload`
and explicit `bind_no_jump_delay_from_catalog()` registration. Only an
actually bound offline callback can write the proposed result. World
and player admission, event-slot validation, category/identity validation,
callback priority, per-module enable/disable and shutdown still apply.

R179 also expands the existing full-catalog coverage audit: the private
66-module, 171-callback manifest now validates the presence of No Jump Delay,
binds both Reach and No Jump Delay by name/category/slot, runs both
independent policies and verifies their disable paths. A synthetic
four-module fixture and negative wrong-category/wrong-event fixtures
are exercised by CTest on Windows and Linux, Debug and Release.

| Coverage metric | R178 | R179 |
|---|---:|---:|
| Original candidate module descriptors | 66 | 66 |
| Recovered native callback subscriptions | 171 | 171 |
| Explicitly bound first-party offline module decisions | 1 (Reach) | **2 (Reach, No Jump Delay)** |
| Remaining native subscriptions without offline implementations | 170 | **169** |
| Live original native algorithms executed in Minecraft | Not verified | Not verified |

This is not an equal-weight percentage of gameplay features. The newly
implemented policy uses high-confidence candidate behavior but does not
establish byte-level or real-game equality of the original engine-side
Java field update. No decrypted DLL, original executable bytes, external
source files or game hooks have been committed.

Offline full-candidate repro (after separately exporting your private
R175 TSV):

```sh
cmake -S reference_compat -B build-reference -DCMAKE_BUILD_TYPE=Release
cmake --build build-reference --config Release
./build-reference/slinkywinky_implementation_coverage_audit \
  build/private_candidate_modules.tsv --expect-candidate \
  --json-out build/private_implementation_coverage.json
```

The Windows Visual Studio multi-config executable resides under
`build-reference/Release/`. The integration PR stays draft and no
changes are made to the parallel R-series runtime branch.

## R180: third independent offline module decision — Null Move

The **private original candidate's native descriptor table** identifies
`Null Move` at registration index **49**, category **move (1)**, event
slot **16**, priority **100**. Another movement feature, `Instant Stop`,
also uses event 16 but has priority **200** at original index **31**;
this establishes a future priority-order integration test target.

The first-party `reference_compat/include/slinkywinky/null_move_controller.hpp`
contains an independently authored pure four-key state machine. It
observes effective game-pressed states, original physical key-held flags,
and tick identifiers for forward, left, back and right. When an effective
key is newly pressed while its opposite remains pressed, it proposes
releasing the opposite. On release, it can propose restoring an opposite
key if physically held. It processes keys in the candidate's forward,
left, back, right order. It does not change real game keys or attach
to a Minecraft process. With both keys first pressed simultaneously,
this deterministic processing order is a test convention and does not
establish a true hardware timestamp winner.

`ModuleExecutionBridge` now supports a typed `NullMoveTickPayload` at event
**16**, validates exact module name, category and subscription, denies
world/player-unavailable dispatch, rejects mismatched or multiple typed
event payloads, and isolates the pure controller's state. Disable and
shutdown stop invoking the callback. The controller can be reset when
world state changes without losing other modules' decision state.

The original private candidate exporter now pins all three verified
module descriptor contracts:

| Module | Index | Category | Event slot | Priority |
|---|---:|---:|---:|---:|
| Reach | 54 | Combat (0) | 8 | 0 |
| No Jump Delay | 45 | Move (1) | 17 | 100 |
| Null Move | 49 | Move (1) | 16 | 100 |

The full original 66-module catalog coverage checker binds all three
first-party offline policies, executes events 8, 17 and 16, validates
their typed output and disable behavior, and reconciles registration
counts. Synthetic positive and wrong-category/event fixtures are
integrated into Windows/Ubuntu CTest in Debug and Release.

| Native registration coverage | R179 | R180 |
|---|---:|---:|
| Original candidate modules | 66 | 66 |
| Original declared callback subscriptions | 171 | 171 |
| Explicit first-party offline module decision bindings | 2 | **3** |
| Original registrations without implementation binding | 169 | **168** |
| Original native algorithms verified running in Minecraft | No | No |

Running the private 66-module TSV test requires the local R175 metadata
export and is intentionally excluded from public CI. Public source
tests use invented synthetic fixtures; no proprietary candidate bytes
or source assets are committed. R180 is an isolated offline policy
addition, not a pixel-fidelity, injector, networking or live-game
acceptance result. This PR stays draft and does not modify the other
chat's R-series runtime branch.

## R181: Instant Stop movement optimization and priority-ordered composition

The private decrypted candidate has a fourth verified native descriptor:
`Instant Stop` index **31**, movement category **1**, event **16**,
signed priority **200**. Its event runs before `Null Move` (index 49,
event 16, priority 100), which matters when both request changes to
the same effective movement-key state.

A new first-party, **offline-only** C++ `InstantStopController` models
three internal phases: idle, watching forward, and one tick of
counter-keys. When the forward input is released on the ground, it
computes the predicted remaining horizontal motion for no counter
input, back-right, back-only, and back-left, and chooses the strictly
smallest speed squared. Its independent math uses the recovered candidate
friction and normalization constants as numerical references but is
**not** claimed instruction- or float-rounding-identical to the
commercial original. The policy returns only proposed key overrides.

After the counter-input tick, the controller proposes restoring back,
left, and right to their physical held states. On loss of the game
world, it fails closed and clears its state; this cleanup differs from
some original read-only out-of-game paths and is intentional safety
hardening. It never presses a real key or mutates Minecraft memory.

`ModuleExecutionBridge` now supports typed `InstantStopTickPayload`
on event 16. Unlike disjoint Reach and No Jump Delay events, a movement
frame may contain both Instant Stop and Null Move payloads. The bridge
requires their in-game state, forward state, and physical key snapshot
to agree before dispatch. The higher-priority Instant Stop proposal is
applied to the **lower-priority offline Null Move input** before
Null Move evaluates it; the resulting requested key-state changes are
kept in memory and never applied to an actual client. Contradictory
typed payloads, bad event slots and missing world/player state are
rejected.

The original-candidate metadata exporter now pins four contracts:

| Candidate module | Original index | Category | Event | Priority |
|---|---:|---:|---:|---:|
| Instant Stop | 31 | Move (1) | 16 | 200 |
| No Jump Delay | 45 | Move (1) | 17 | 100 |
| Null Move | 49 | Move (1) | 16 | 100 |
| Reach | 54 | Combat (0) | 8 | 0 |

R181's first-party native test covers all three counter choices,
one-tick restoration, ground/forward gates, invalid numeric values,
shared event priority, typed snapshot consistency, lifecycle and
shutdown. The full 66-module catalog audit now registers and runs
four first-party *offline* decisions; a synthetic six-module fixture
and three negative fixtures are added to public CTest on Windows and
Ubuntu in Debug and Release.

| Coverage metric | R180 | R181 |
|---|---:|---:|
| Candidate module descriptors | 66 | 66 |
| Recovered native callback subscriptions | 171 | 171 |
| Independently implemented offline decision bindings | 3 | **4** |
| Native subscriptions without first-party implementation binding | 168 | **167** |
| Verified native Minecraft implementation | No | No |

The private original-derived TSV, decrypted DLL, licensed code and
executable assets remain outside GitHub. None of this creates a live
injectable module or establishes real in-game movement behavior.
PR #2 remains separate, draft and unmerged; the other chat's runtime
code has not been modified.

## R182: No Slow — fifth independently authored offline module decision

The locally held decrypted candidate identifies **No Slow** at descriptor
index **46**, movement category **1**, event slot **21**, priority **200**.
The registered callback operates after Minecraft has applied the vanilla
0.2 movement slowdown. Its item-specific multiplier selects bow (ID 261),
one of five Minecraft 1.8 swords (267/268/272/276/283), or other usable
items (consumables). The original .data/.rdata hashes and event metadata
are validated locally by the private candidate fixture exporter.

The new first-party pure controller `no_slow_controller.hpp` computes a
proposed replacement for already-slowed strafe/forward input, multiplying
both by the selected setting divided by 0.2. It issues no proposal when
the player is not using an item or no held item is present. Rejecting
nonfinite, negative and overflowing values is **deliberate hardening**,
not an assertion of exact parity with the original unguarded arithmetic.

`ModuleExecutionBridge` now routes typed No Slow inputs on native event 21,
validates name, movement category and priority 200, prevents conflicting
typed payloads, and respects world/player gating, enabled state and teardown.
All outputs remain pure offline proposals. No Java/JNI, packets, input
hooks, injected code or Minecraft field writes were introduced.

The full 66-module, 171-callback original-derived catalog audit now has
five *independently implemented offline* module bindings: Reach, No Jump
Delay, Null Move, Instant Stop and No Slow. **166 native callback
registrations still have no executable first-party binding.** These
counts track descriptor subscriptions rather than complete gameplay
functionality, so they are not a client-completion percentage.

The synthetic fixture CTest covers a five-policy catalog, category and
event/priority negative controls, item categories, invalid input handling,
and event-21 enable/disable and shutdown. It is run as part of the
isolated Windows/Ubuntu Debug/Release reference audit. The user's private
original-derived TSV, reference binary and upstream implementation are
never committed. PR #2 remains draft and separate from PR #1.

## R183: No Hit Delay — sixth independent offline module decision

The original private candidate descriptor identifies **No Hit Delay**
at index **42**, category **utility (4)**, event **14** and priority **0**.
The corresponding reference callback is a pre-attack check. When the
attack click is active, JVM handle ready, and left-click cooldown
counter is greater than zero, its normal mode proposes clearing that
counter to zero. In selective mode (1), a missing target or a target
whose type ordinal is 0 produces an arm-swing-only proposal instead.
If the player is already using an item, even that swing is suppressed.
Target hits still propose clearing the cooldown in selective mode.

These decisions are independently implemented in the pure
`no_hit_delay_controller.hpp`. It has no JVM access, gameplay event
injection, attack modification, counter writes, or packet effects.
Typed `NoHitDelayTickPayload` is only accepted on event 14, requires
the registered module's utility category and native priority 0,
obeys world/player and module state gates, and is cleared on teardown.

R183's synthetic C++ test covers the normal/conditional modes, misses,
target hits, item-use swing suppression, click-counter guards, invalid
typed event, wrong category/event/priority, enable/disable, and shutdown.
The complete private-catalog metadata audit now accounts for **six**
independent first-party offline callback bindings out of **171**
recovered native subscriptions, leaving **165 without implementations**.
Windows and Ubuntu Debug/Release CTest runs only invented synthetic
metadata; the decrypted candidate and extracted binary assets stay local.

Native execution, module algorithm parity and in-game combat behavior
are **not yet established**. This integration PR remains draft and
does not modify the other chat's live Minecraft runtime branch.

## R184: No Use Delay — seventh independent offline module decision

The original candidate's decrypted module descriptor at index **48**
registers **No Use Delay** in utility category **4** at event **18**
with priority **0**. The pinned public reconstruction describes its
late-tick callback as a stateful item-use countdown with a hotbar-slot
change reset and the optional `allow_desync` setting.

An independently authored `NoUseDelayController` now stores its
remaining item-use count and previously seen selected hotbar slot.
In game it first restores the effective use-key state to the physical
key state when the selected slot changes; it then checks the effective
key state sampled at the beginning of the tick. If this starts a
new item use, it records the reported ticks remaining. On subsequent
ticks it decrements that value and proposes releasing the use key
when the count reaches zero. With `allow_desync`, it also proposes
release when the client reports zero use ticks remaining.

When outside a game it clears only the use countdown (the original
observed branch leaves the selected-slot state untouched). It may
be explicitly reset at a world boundary by the offline bridge.

All outputs are proposed `std::optional<bool>` key states. No real
click is generated and no Minecraft/JNI method, packets or native
game state are touched. Event-18 binding checks its module name,
utility category and priority 0, with world/player gating, invalid
typed-frame rejection, enable/disable and shutdown fencing.

R184 adds a synthetic nine-module event catalog with seven first-party
offline decisions plus invalid category/event/priority fixtures.
Original reference metadata is separately pinned and checked by
the private candidate exporter (index 48, event 18, priority 0).

| Metric | R183 | R184 |
|---|---:|---:|
| Candidate module descriptors | 66 | 66 |
| Original native event subscriptions | 171 | 171 |
| Independently implemented offline decision bindings | 6 | **7** |
| Registrations without first-party offline implementations | 165 | **164** |
| Verified live Minecraft module behavior | No | No |

The synthetic C++ regression and full catalog tests are wired to
Windows/Ubuntu Debug/Release CI. No private extracted binary, original
candidate module code or proprietary assets are committed. This PR
remains draft and separate from the other chat's development branch.

## R185: Item Use Fix — paired before/after offline decision callbacks

The decrypted original candidate identifies **Item Use Fix** at
descriptor index **35**, category utility **4**, with two native
event subscriptions: before **14** and after **15**, both priority **0**.
Unlike previous single-callback modules, the independent offline
`ItemUseFixController` maintains one saved hitting-block state.

On a real (non-skipped) right-click before callback, it records whether
the game was hitting a block. If so it proposes clearing that flag
before item use. On the after callback it proposes restoring the
saved flag, unless the hook was marked as skipped. As in the observed
candidate implementation, a skipped before does not replace the
saved value and after does not automatically clear the saved flag;
a new real before overwrites the flag. A separate world-reset method
clears it explicitly when requested.

`ModuleExecutionBridge::bind_item_use_fix()` validates **both**
original subscriptions before registering either callback. Missing
event 15, invalid event/category/priority, or a duplicate registered
handler rejects the pair instead of reporting a half-working module.
The typed offline event envelope supports Item Use Fix and No Hit Delay
together on event 14; the native qsort priority-tie order is
**not** claimed to be deterministic. Event 15 admits Item Use Fix only.

The 66-module private-catalog coverage now expects **8** independent
offline module decisions and **9** implemented callback bindings
(Item Use Fix occupies two subscriptions). Of the **171** recovered
original callback registrations, **162** still lack first-party bindings.
R185 tests include a synthetic ten-module catalog, wrong-category,
wrong-event, wrong-priority and missing-after rejection fixtures,
and direct two-phase callback/lifecycle tests for Windows and Ubuntu.

Nothing has been attached to Minecraft: the state writes are only
proposals, not actual JVM changes. The decrypted original ZIP,
module executable bytes and upstream code remain outside GitHub.
The other chat's R-series branch is not modified.

## R186: Ping Fix — independent incoming/outgoing packet-event decisions

The user-supplied decrypted original candidate identifies **Ping Fix**
as module descriptor index **51** in utility category **4**.
It registers two callbacks, **incoming event 2** and **outgoing
event 3**, both with signed priority **-1000**. The pinned C++
reconstruction classifies the inbound `S00PacketKeepAlive` and outbound
`C00PacketKeepAlive` packet types respectively, proposing event
`state = 3` (drop) when the corresponding packet type matches.

The independently authored first-party `PingFixController` represents
the decision as an **offline optional numeric event-state proposal**.
It neither reads nor modifies a packet, hooks a socket, cancels real
traffic, accesses a JVM or runs inside Minecraft. Wrong-direction
keep-alive classes and unrelated packet types produce no proposal.

The isolated `ModuleExecutionBridge::bind_ping_fix()` verifies both
event subscriptions and priorities **before** binding either callback.
It rejects missing events, wrong priority, category and double-binding.
The two typed frame handlers remain separately enumerable in
`ExecutionCoverage`. The model's existing world/player presence guard
remains a deliberate fail-closed condition; its exact equivalence
to early-login native packet events has **not** been demonstrated.

R186 adds a synthetic eleven-module fixture, positive C++ packet
direction and lifecycle tests, and five negative metadata fixtures.
The full original-derived private manifest still contains **66**
modules and **171** descriptor subscriptions. The offline bridge now
has **9** first-party implemented module decisions with **11**
executable callback bindings (both Item Use Fix and Ping Fix use
two slots), leaving **160** registration slots unimplemented.

Windows and Ubuntu Debug/Release tests use only synthetic fixtures.
The private source DLL and decoded data never enter GitHub. This
is not evidence of a working live packet hook or full original
client equivalence. PR #2 remains draft and separate from PR #1.

## R187: No Item Release packet state and shared outgoing event-3 priority

The private original candidate's descriptor **44** (utility category
**4**) declares one outgoing callback on event **3**, priority **0**.
Ping Fix occupies the same event at priority **-1000** and thus runs
later in the native descending-priority dispatcher.

The independent offline `NoItemReleaseController` models the candidate's
one-shot release lock: a C08 use-item packet with an eligible item
arms it according to mode (0 swords, 1 non-swords, other values both),
while bows (261) and fishing rods (346) are always excluded. Missing
item stack/item cancels a previous lock. A subsequent C07 digging
packet consumes the lock regardless of status; status **5** proposes
packet event state **2** (cancel once). Other packet classes, including
C00 keep-alive, leave the lock unchanged. These are data-only proposals;
this code does not cancel game packets or touch Java network objects.

`ModuleExecutionBridge` validates the exact name, utility category,
event 3 and priority 0 before installing its sole callback. Typed
outgoing frames can contain **both** No Item Release and Ping Fix,
provided their packet class views agree. The shared event executes
No Item Release before Ping Fix and rejects contradictory C00 labels,
other mixed event types, missing world/player state and shutdown calls.

Synthetic C++ tests cover item modes, all sword IDs, bow/rod exclusion,
missing items, C07 one-shot cancellation, non-release C07 consumption,
unrelated keepalive preserving the lock, shared packet priority,
invalid metadata, disable and teardown. A twelve-module synthetic
manifest and negative fixtures run in the Windows/Ubuntu Debug/Release
reference workflow. No original DLL bytes or proprietary sources are
committed.

The original candidate still contains **66** module descriptors and
**171** native callback registrations. The isolated bridge now has
**10** independent first-party offline module decisions and **12**
implemented callback bindings, leaving **159** subscriptions without
first-party implementations. No functioning live packet interception
or in-game parity is claimed. PR #2 remains draft; the parallel
runtime PR #1 remains untouched.

## R188: Anti Debuff — independent event-18 visual policy

The user-provided decrypted candidate has been locally verified
against the pinned `.data` and `.rdata` SHA-256 hashes. Descriptor
index **2** identifies `Anti Debuff` in **visual category 2** with
one callback on native **event 18 at priority 0**. The pinned
OpenSlinky reference describes two independently selectable visual
effects: clear an active blindness potion effect's duration to zero,
and set both current and previous portal-wobble values to zero.

The original first-party `AntiDebuffController` returns only typed
offline **proposals**. Blindness duration is proposed as 0 only if
the blindness setting is enabled **and** an active blindness effect
exists; nausea/portal proposals are emitted only when nausea is
enabled. No Minecraft fields, entities or potion effects are
actually accessed or changed. No original executable instructions
or unlicensed third-party source are copied into this branch.

The `ModuleExecutionBridge` validates the exact module name,
visual category, event 18 and signed priority 0 before binding.
The typed event-18 envelope can contain both Anti Debuff and
No Use Delay callbacks. Both have native priority 0; native qsort
tie ordering is **not** claimed or asserted. Other conflicting
typed payload combinations, absent player/world, disabled
module state and shutdown remain gated.

R188 tests exercise independent setting combinations, missing
blindness effect, shared event 18, teardown, and invalid
category/event/priority metadata. A thirteen-module synthetic
catalog with three negative fixtures is run in the independent
Windows/Ubuntu CTest matrix (Debug and Release). The private
candidate-exporter also pins descriptor index 2 in metadata-only
checks without publishing the original derived 66-module TSV.

Full-candidate accounting: **66 module descriptors**, **171**
native event registrations, **11 independently authored offline
module decision policies**, **13 implemented callback bindings**,
and **158** remaining callback registrations without a binding.
These numbers measure binding inventory, **not** the percentage
of completed client gameplay, rendering or GUI functionality.
PR #2 remains draft and separate from the other chat's runtime
branch; no live Minecraft acceptance is inferred.
