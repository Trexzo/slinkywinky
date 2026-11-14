# Client shell ↔ module runtime integration contract (Chat 1 / Chat 2)

**Status:** R207, proposed and validated on isolated first-party models; **not yet integrated into Minecraft**.
**Chat 1 owns:** PR #1, `reconstruction/r1-core-fidelity`.
**Chat 2 owns:** PR #2, `integration/openslinky-fd9c592-reference` (PR #2 targets PR #1's branch).
Do not change the other's branch or copy source files across branches without an explicit integration commit.

## Shared version-1 semantics

Chat 2's authoritative candidate header is `include/slinkywinky/module_control_contract.hpp` (introduced in its R210 work), defining:

```cpp
struct ModuleControlRequest {
    std::string_view canonical_name;
    bool enable;
};
struct ModuleControlState {
    std::string_view canonical_name;
    bool requested_enabled;
    bool active_in_world;
    bool settings_override;
    std::uint64_t world_generation;
};
```

The request is **not** a toggle acknowledgement and is not an active-state snapshot. Chat 2's legitimate module manager alone decides whether a module was admitted, is active in a given world generation, and has the expected callbacks. The sender must never hand a JVM pointer, native source module pointer, or ownership token to Chat 1. Names, Booleans and integers are copied as values.

Chat 1's R207 `ModuleArrayListSnapshot` is a separate view-side transport DTO, not a duplicate definition of the authoritative PR #2 header. A future adapter copies each PR #2 state to `ModuleArrayListState`. It also supplies:

- `contract_version == 1`, matching Chat 2's declared version;
- strictly increasing `source_sequence` assigned at the owning publisher;
- current `world_generation`, incremented/retired by the legitimate runtime;
- `world_ready` from actual client/world lifecycle, not from ClickGUI preferences;
- a bounded set of **at most 66** native module registration states.

The PR #1 ArrayList model rejects duplicate/unknown source candidate names, conflicting generations, active-but-unrequested modules, any active module outside a ready world, invalid contract version, zero source sequence or more than 66 source records. It displays only states with `active_in_world && requested_enabled && world_ready`, **never** MenuState's `preview_enabled`. A request can be remembered while a world is absent; it must not appear active until Chat 2 reports a current, admitted in-world state.

The candidate UI inventory has **67 descriptors**, whereas the C++ native manager has **66 registrations**. Before enabling a full list, both tracks must pin a source-backed name/identity mapping, reject collisions and explicitly document unmatched descriptors. No array-index guessing or implicit name rewrites.

## Required integration transactions

1. A source-backed ClickGUI control requests an enable/disable with one canonical module name. The UI may show a pending state, but cannot claim success.
2. The native manager validates the request on its admitted owner, including world generation, module registration and callback subscriptions. Denial must be explicit.
3. Chat 2 publishes a complete **copy-only** enabled/active snapshot with monotonic sequence and its generation. Chat 1's consumer must reject stale/reordered or post-unload snapshots, and never execute game effects.
4. Chat 1 renders an ArrayList row only for a current **active-in-world** module. A request-only module stays hidden from the active list. The ArrayList draw owns the verified WGL context and restores all GL state.
5. On disable, world loss, module exception or unload, native callbacks retire before the corresponding row disappears; neither a stale snapshot nor a lingering UI toggle can bring it back.
6. End-to-end acceptance requires: GUI request → admitted runtime state → actual in-world module-specific effect → ArrayList visibility → disable/world loss → effect stops and row disappears → clean unload and vanilla input restoration.

## Hard boundaries

- **Chat 1:** menu navigation, source widgets, appearance, native GL11 ClickGUI/ArrayList, settings UX, profiles, safe loader/attach/unload shell, screenshot and interactive GUI acceptance.
- **Chat 2:** 66 original candidate module registrations, 171 callback-subscription reconstruction, settings semantics, world event behavior, native manager and source-verified module implementations.
- **Shared:** agreed immutable state publication and controlled integration testing; no direct Java/Win32 input interception until owner and release/restoration evidence exists.

**Never count** 67 catalog labels, 16 offline policies, 22 modeled callback bindings, Forge read-only ticks, an attached JVM, or synthetic WGL screenshots as 66 functional injected modules. PR #1's R207 renderer and PR #2's R210 contract are groundwork, not a completed live integration.

## Immediate acceptance target

Integrate **one verified module** (e.g. Sprint only if Chat 2 proves actual in-world effect and teardown) through the complete request/ack/state/render lifecycle above, before expanding to the remaining 66 descriptors. Keep the other branch unchanged until it is ready for an explicit integration commit.

## R209 Chat 1 concrete Java/JNI consumer boundary

Chat 1 now has the **render-side** publication slot in `RendererWglEvidence`:
`nativePublishRetainedModuleArrayList(context, dc, width, height, sourceSequence, worldGeneration, worldReady, canonicalNames, requested, active)`.

It is private, disabled in the existing production read-only acceptance schedule, and refused without a previously painted exact retained WGL owner session. Arrays represent **one complete state**, length <=66, with distinct canonical source names, requested and independently admitted `active_in_world` flags. R208's feed rejects replay/contradiction/world-generation regression; the next render callback paints the active rows from the same retained JNI session. Menu preview toggles are never a publisher. Session teardown retires the feed. A new Attach session needs a new feed and a new strictly-increasing sequence epoch.

**Chat 2 handoff:** The native module manager must produce actual owner-thread `ModuleControlState` activation, not synthetic labels. The final cross-branch adapter should copy all current states to this value-only entry, retire records on world loss/disable/unload, and never call into Minecraft during GL painting. If module-owner and WGL-render-owner threads differ, do not bypass the current owner check; coordinate a bounded copy-only handoff and call the JNI publisher from the admitted render boundary. No game pointers or direct shared callbacks should cross this interface.

R209's Java 8/JNI pixel test was completed with a disposable WGL fixture and no original Minecraft process. It is not yet proof of Chat 2 runtime publication or real module effect.

## R210 Chat 1 request-control handoff

PR #1 now provides `GuiModuleControlHandoff` (source-owner request, bounded one-slot transport, module-owner dequeue, validated acknowledgement/denial) and `WglRetainedMenuPreview::queue_focused_runtime_toggle`. The latter does not intercept real original Minecraft mouse/keyboard input and is not invoked by the original passive acceptance; it requires a separately authenticated input gesture before production activation.

Only source-confirmed `Sprint` may be requested. A pending GUI request does not modify `MenuState.preview_enabled`, the ArrayList feed, or game motion. Only a later owner-supplied complete `ModuleArrayListSnapshot` whose request ID, sequence, world generation and requested flag match is accepted/published. An actual `active_in_world` false is allowed even for requested true when the world is not ready. Denial leaves the last state intact; invalid/replayed acknowledgment revokes the feed. No PR #2 file is copied into PR #1. For a real integration, the admitted module owner must map the copied GUI command to PR #2's `ModuleControlRequest` and copy its `ModuleControlState` response back across the render boundary.

Full Windows CTest R210 acceptance passed **100/100**; this does not establish original-game physical input exclusivity, original Slinky parity, or real live module effects.
