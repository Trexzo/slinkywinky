# R207 — Chat 1 native ArrayList client-shell foundation

**Owner:** Chat 1 / `reconstruction/r1-core-fidelity` / [draft PR #1](https://github.com/Trexzo/slinkywinky/pull/1). Chat 2 independently owns game modules and callbacks in draft PR #2.

## Deliverable: real GL11 HUD drawing from a runtime-state contract

Before R207, this branch had a diagnostic observation HUD and an extensive ClickGUI prototype, but **no separately named, active-module ArrayList**. In particular, `MenuState.preview_enabled` was explicitly a GUI demonstration flag; it was never a reliable runtime enable state. Rendering that bit as an "active module" would falsely advertise working modules.

R207 adds a first-party standalone ArrayList HUD model and an actual GL11 WGL drawing function, without changing the existing JNI-facing classes' binary layout:

- `include/slinkywinky/module_arraylist.hpp` + `src/module_arraylist.cpp` — versioned `ModuleArrayListSnapshot` and copied `ModuleArrayListState` inputs. A row is displayed **only** when `requested_enabled` and `active_in_world` are both true, the world is ready, and the source catalog name and generation match. Unknown/duplicate module identities, out-of-range input count, stale generation and contradictory enablement are rejected. Empty/out-of-world snapshots yield an empty ArrayList.
- `src/windows_menu_gl11.cpp` + `include/slinkywinky/windows_menu_gl11.hpp` — `draw_module_arraylist_gl11` renders the active names at the top right of the verified current WGL context with measured system-font widths, a translucent dark row background and an accent stripe. Font-width sorting and the 20px row pitch are **first-party interim design choices**, not established original-Slinky geometry. Input provenance, identity and viewport are checked before any drawing. GL matrices/flags are restored using the existing `ScopedFixedFunctionState`.
- `tests/test_module_arraylist.cpp` — Release-active acceptance for requested-vs-active distinction, valid empty/worldless state, exact 67-entry candidate catalog vs 66-entry native input bound, duplicate, unknown, inactive world, generation mismatch, contract-version mismatch, zero sequence, and separation from `MenuState.toggle_preview`.
- `tests/test_windows_menu_gl11.cpp` — actual isolated Windows WGL `Sprint` + `Keep Sprint` two-row paint, while requested-only `Timer` stays absent; the exact accent-bar pixel and GL state restore are asserted. Forged catalog identity or missing expected WGL context is rejected. Optional `SLINKYWINKY_R207_ARRAYLIST_PPM` captures the disposable test framebuffer, not desktop or original game.

R207 is a **working WGL HUD renderer** for the allowed state format, but it is **not yet a live, injected ArrayList in Minecraft**. The connection from Chat 2's real owner-thread module manager to this input and from the actual original Minecraft frame callback to this drawing function remains to be implemented and separately accepted. Neither a menu preview toggle nor synthetic test labels can substitute for that later wiring.

## Chat 2 integration contract

Chat 2 has already defined `include/slinkywinky/module_control_contract.hpp` on its branch (R210 candidate): `kModuleControlContractVersion=1`, `ModuleControlRequest{canonical_name,enable}`, and `ModuleControlState{canonical_name,requested_enabled,active_in_world,settings_override,world_generation}`.

Do **not** copy that header onto PR #1 and create a merge conflict. The R207 view type is deliberately a **copy-only transport adapter**:

| Source field on Chat 2's `ModuleControlState` | Chat 1 input |
|---|---|
| `canonical_name` | `ModuleArrayListState.canonical_name` (copied string) |
| `requested_enabled` | same Boolean |
| `active_in_world` | same Boolean; the only positive display admission |
| `world_generation` | same unsigned generation, validated against snapshot |
| `settings_override` | **not used** to decide whether a module appears |

The adapter/publication layer will additionally need `contract_version=1`, a monotonic `source_sequence`, `world_generation`, and a true `world_ready` from the legitimate owner. These inputs must be **published by Chat 2's admitted game/client-thread lifecycle**, not inferred from UI. R207 validates positive sequence and generation consistency per snapshot; monotonic sequence ordering and thread-safe publication across threads are future **integration-gate** requirements.

The 67 candidate ClickGUI descriptor names are not necessarily identical to the 66 source-derived native module registrations. A mismatch must be explicitly mapped with source evidence, not guessed or silently renamed. As long as there is no authenticated module-state publication, Chat 1 must display an empty ArrayList in the real client, not make up enabled rows.

### Ownership split for future development

**Chat 1 / PR #1:** source-faithful ClickGUI, ArrayList/HUD layout and paints, GUI event handling, settings views, profiles, owner-verified renderer admission, injection/attach/unload shell, and integration adapter once Chat 2's owner-thread snapshots are available.

**Chat 2 / PR #2:** native module manager, 66-descriptor registration, source-grounded runtime state, event subscriber binding, actual gameplay behavior/cleanup and acceptance, plus the authoritative `ModuleControlState` publication semantics.

**Shared integration acceptance:** (1) source-backed GUI module request; (2) Chat 2 owner-thread admission and actual active result; (3) copy-only state with monotonic sequence and generation; (4) Chat 1 ArrayList row appearing only while active; (5) settings and in-world event effect; (6) world loss, disable, unload retire both the behavior and row. No duplicate JNI interceptors or directly shared game object pointers. Both PRs stay draft until these gates are demonstrated.

## Verification and known limits

R206 already passed **97/97** tests from a clean `Desktop\Slinky-R1-Workspace\SlinkyWinky-R206-Build` tree, plus original 1.8.9 read-only acceptance. R207 added the 98th portable module-list fixture and a real WGL draw path. **Verified on the connected PC:** a clean stable-path Windows x64 Release build and **98/98 CTest PASS**, including the new `r207_module_arraylist_runtime_state_only` and `isolated_wgl_menu_state_restore` cases, Java/JNI and native agent attach. The disposable WGL test printed `R207_ACTUAL_WGL_RUNTIME_ARRAYLIST_ROWS_AND_GL_RESTORE_PASS`. The PNG below is a **direct actual GL_BACK readback**, not an image generation or Minecraft screenshot:

- [R207 active-module ArrayList fixture](screenshots/r207-active-module-arraylist-wgl.png) — size 2,867 bytes; SHA-256 `6e608825862f90e5cd357861c10a7db6e8d8b1945b62b505e88a08d86aef41be`. It shows `Keep Sprint` and `Sprint`, ordered by measured width; requested-only `Timer` is omitted, and the accent stripe's pixel at (666,17) is exactly RGB (243,93,18).

The original Minecraft JVM acceptance was **not rerun specifically for R207**, because R207's ArrayList is not yet connected to any game agent frame; an original read-only launch would not exercise it. R206's separately verified original read-only acceptance is still the most recent relevant baseline. No real module gameplay effect or actual injected ArrayList rendering is claimed.

The client is **not** yet 100% reconstructed. The 66 native descriptors, 171 event subscriptions, previous 16 offline module policies and 22 modeled bindings are evidence of catalog/partial modeling, not 66 working injected features. This ArrayList renderer does not introduce any world mutation or covert gameplay input.
