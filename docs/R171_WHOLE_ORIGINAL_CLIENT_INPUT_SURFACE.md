# R171 — whole-original-client input surface, not just `ave.s()V`

**Target:** `Trexzo/slinkywinky` / `reconstruction/r1-core-fidelity` / draft PR #1. This is an **offline Java 8/ASM 5.2 source audit only**; no game is launched, transformed, injected or manipulated.

## Why the existing modal input plan is insufficient

R162–R165 checked the original Minecraft 1.8.9 `ave.s()V` input method and a simulated whole-method entry gate. That is useful but insufficient: **original Minecraft has additional Mouse, Keyboard and keybinding polling callsites outside that method**. The original renderer and UI paths can still read device state even if `ave.s()V` itself were bypassed.

R171 independently inspects **every class** in the exact official Minecraft 1.8.9 JAR (8,461,484 bytes, SHA-1 `3870888a6c3d349d3771a3e9d16c9bf5e076b908`), not just the previously studied input class. Every invocation to static LWJGL2 `org.lwjgl.input.Mouse` and `Keyboard` APIs is collected, as are the exact obfuscated `avb` key-binding mutation and polling signatures. Results are lexically sorted by exact caller method and instruction ordinal and SHA-256 hashed. These are **static API references**: some control cursor state or provide names rather than reading active button state; the report does not imply every recorded method performs a gameplay action.

## Verified original 1.8.9 evidence

| Source-inspection metric | Result |
|---|---:|
| Exact source classes scanned | **2,507** |
| All matching invocation sites | **146** |
| Sites in `ave.s()V` | **50** |
| Sites outside `ave.s()V` | **96** |
| Direct LWJGL Mouse/Keyboard sites outside `ave.s()V` | **86** |
| Mouse API sites across JAR | **42** |
| Keyboard API sites across JAR | **74** |
| Key-binding poll sites across JAR | **26** |
| Key-binding state/press dispatch sites across JAR | **4** |

This audit counts **50** matching direct input/keybinding calls inside `ave.s()V`; the earlier R162 ledger counted **58** broader GUI/input-related sites because it also included GUI-dispatch and other categories. The counters measure different, explicitly defined scopes.\n\nSorted full-input-surface SHA-256:

`b029538b90e0e23947c9224779028679f0faced032f5a469b07f69b8859dd11e`

Verified outside-source examples:

- **`bfk.a(FJ)V` — original render method:** `Mouse.isButtonDown(I)Z`, `Mouse.isInsideWindow()`, `Mouse.getX/Y()`, cursor grabbing and positioning. Rendering can still inspect mouse state.
- **`avh.a(Lavb;)Z`:** `Mouse.isButtonDown(I)Z` and `Keyboard.isKeyDown(I)Z`, separate from `ave.s()V`.
- **`awi.p()V`:** `Mouse.getEventButton()`, `getEventButtonState()`, `isButtonDown(I)Z`, wheel events — a distinct GUI input consumer.
- **`ave.Z()V`:** `Keyboard.getEventKey()`, `getEventCharacter()`, `getEventKeyState()`, `isRepeatEvent()` in a separate method.

These are precise *obfuscated original-source* method references, not asserted deobfuscated semantic equivalences.

## Regression/negative control

The Java 8 test generates a minimal `ave.s()V` plus a separate `bfk.a(FJ)V` fixture. Replacing one mock renderer's `Mouse.isButtonDown` invocation leaves the `ave.s()V` source unchanged, **but R171 detects the change in the cross-class digest**. The same negative control is applied in memory to the exact original Mojang `bfk.class`, and the original bytes are verified unchanged. A missing/non-original client JAR is refused. The pinned original audit requires the exact 2,507-class/146-site fingerprint.

The fixture test belongs in CTest as `r171_java8_whole_jar_input_surface_fixture`; the full Mojang JAR scan runs independently in `.github/workflows/original-189-audit.yml`. Both classes live only inside `slinkywinky_render_audit.jar`, never the injected `slinkywinky_instrument.jar`.

## Implementation decision

**Do not enable the R164 `ave.s()V` early-return idea as a purported exclusive Minecraft GUI input hook.** At least the original renderer and additional utility/UI methods still have direct input calls. Even replacing all 146 direct callsites alone would not prove safe ownership of transitive accesses, OS input, shared key state, held keys or original loaded-class restoration.

The next runtime integration work must address:

1. Complete ownership of the original game, GUI and renderer input reads, with source-to-runtime evidence for which calls remain live during an overlay.
2. Physical key/button release and per-window foreground control, including focus loss and invalid viewport.
3. Actual original game-thread dispatch to the first-party retained C++ menu, without simultaneous vanilla gameplay actions.
4. Verified **live class restoration and input-state recovery** after close, fault, update and JVM exit.
5. Original Minecraft 1.8.9 acceptance in a real game world, including interaction, focus loss, restoration and explicit no-double-dispatch checks.

**Status:** R171 proves a material existing blocker and rejects an unsafe shortcut. It does not itself activate mouse clicks, change gameplay, complete all 67 modules or prove commercial Slinky parity. The draft PR remains unmerged.
