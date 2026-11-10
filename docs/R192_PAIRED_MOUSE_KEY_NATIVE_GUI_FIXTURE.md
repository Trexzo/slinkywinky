# R192 — paired mouse and keyboard interaction on the same rendered native ClickGUI (fixture only)

**Target:** `Trexzo/slinkywinky`, `reconstruction/r1-core-fidelity`, draft/unmerged [PR #1](https://github.com/Trexzo/slinkywinky/pull/1). Separate OpenSlinky C++ integration PR #2 is not touched.

## Defect and scope

R190's ledger tracked keyboard and all five mouse buttons in one offline model. R191 proved synthetic keyboard Tab navigation could change the retained native GL11 WGL GUI (with caller-owned external router, preserving the existing native WGL class layout). However R191's integration did **not** require mouse clicks to pass through the same ledger: the standalone test-only `pointer()` could change local GUI settings using its own synthetic exclusive-dispatch boolean without verifying the same handoff epoch or held keyboard state.

R192 adds `fixture_pointer_event` as an **offline-only** native WGL entry point, bound to exactly the same R190 ledger, R191 keyboard router, owner thread, epoch and painted GUI. **This is not called from production JNI or the Java 8 original-client agent.** The R160 safety source test explicitly forbids its name in those original-game entry points. No new original mouse/keyboard hook, queue interception, button-state mutation, packet, movement, combat action or click delivery is added.

## Joint handoff

- The paired left-button `Down` is accepted only if R190's test ledger is in `MenuFixture`, the R191 router is already active and bound to **the same MenuNavigation instance**, the exact current WGL context/DC/GL viewport is intact, the mouse is foreground-verified, a source-backed painted frame exists, and **all** model keyboard/mouse keys are released.
- The ledger records the synthetic left-button Down before the original local `MenuMouseGesture` accepts the coordinate from the last successfully painted native menu frame. Duplicate Down, orphan Up/Move and ambiguous states fail closed.
- `Up` requires an already tracked left-button press and a pending native mouse gesture. Only if both the ledger release and actual native hit-transaction succeed can the local first-party typed profile change. The change invalidates the painted input frame, so it must be repainted before another model mouse Down or navigation Down.
- A currently held keyboard key prevents mouse Down. A keyboard event during a held native mouse gesture is rejected by R191, revokes the shared ledger, and drops the held gesture and stale paint.
- Invalid or mismatched WGL binding, changed viewport, lost focus, malformed phase, rejected hit or explicit cancel also abort the fixture ledger and revoke local geometry. This is **not** a live Minecraft input release or ownership restoration.
- R190 `tracked_down(device,code)` exposes an owner-thread-only, bounds-checked read-only status for correct ordering; it never modifies or reads OS input.

## Native test scenarios

The existing disposable Windows WGL regression now exercises the **actual retained GUI instance**, rather than separately mocked models:

1. Exact source-indexed first-module toggle rectangle is calculated from `MenuRenderFrame`; a paired, synthetic Down/Up toggles the first Combat module and a validated native profile snapshot confirms the change.
2. After a true GL11 repaint, synthetic LWJGL2 Tab Down/Up navigates the **same GUI** from Combat to Move without losing the saved profile.
3. Close/drain rejects premature restore and accepts a separately supplied full restoration receipt. Both R190 permanent methods still refuse live Minecraft event dispatch and vanilla input suppression.
4. Wrong-menu router, orphan mouse Up, keyboard already held at mouse Down, mouse already held at keyboard Down, stale post-click painted geometry and a real post-press OpenGL viewport drift are all negative controls. An unrelated router is denied **without mutating** the right owner's session; failures on the correct owner revoke its local fixture and never mutate an uncommitted native control or produce gameplay events.
5. The separate portable R190 ledger test now checks correctly tracked synthetic key and mouse Down/Up edges, unknown input codes and owner thread rejection.

## Evidence and remaining blocker

The full **Windows x64 Release build passed** and the complete native / Java 8 / JNI / agent-attach **CTest suite passed 93/93 tests**, including the R192 combined ClickGUI mouse+key transitions and the R160 source guard. Separately, the isolated **original vanilla Minecraft 1.8.9** read-only acceptance passed with **six** restoration/admission stages, **64** retained render frames, **60** original source-visible slider draws, and **63** passive pointer samples. Its original-JVM exact OS-focus receipt was `foreground=0 background=63 refused=0 losses=0 regains=0 post_hits=0`; source-hover exact hits remained **0** and `native_click_delivery=disabled`. This was background-only original-client acceptance: it **does not test physical foreground input handoff**.

The R171 exact-source all-class audit still finds **146 static input API references across the original JAR**, including **96 outside `ave.s()V`**. These are a material blocker to claiming safe original-game input suppression. Real operator-verified foreground mouse and keyboard edges, complete consumer-path exclusion, actual original bytecode restoration and the full Slinky module/visual parity remain unproven.

**Live click dispatch remains disabled; PR #1 stays draft.**
