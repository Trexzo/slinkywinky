# R209 — retained JNI ArrayList publication and paint

**Ownership:** Chat 1 / `reconstruction/r1-core-fidelity` / draft PR #1.
**Baseline:** R208 commit `a993ee2`. PR #2 is not modified.

## Implemented production path

The existing `nativeDrawRetainedPreview` Java 8/JNI bridge already draws the retained ClickGUI on a source-admitted original Minecraft renderer/WGL thread. R209 connects the R208 `ModuleArrayListFeed` to that **same** paint callback. A published, validated active-module snapshot can now draw at the top right after the ClickGUI without a second Java renderer hook, a separate WGL context, or any menu-preview fake active flags.

A new private `RendererWglEvidence.nativePublishRetainedModuleArrayList(context,dc,width,height,sequence,worldGeneration,worldReady,names,requested,active)` entry accepts a complete owner-supplied copy:
- Only after the retained preview's first successfully painted original-owner frame, with matching current WGL context/DC/viewport and the same owner thread.
- Up to 66 source catalog names, no missing/oversized names, strict array-length parity, strictly increasing positive source sequence and nonregressing world generation.
- Uses R208's immutable copy, source-identity checks, active-world/request coherence and quarantine on malformed or replayed owner publication. A version-1 snapshot never treats the GUI `preview_enabled` bit as real module state.
- Inactive/no-world or uninitialized publication means zero active rows. The first retained frame is still a normal menu with no ArrayList.
- `nativeEndRetainedPreview`, `nativeDiscardRetainedPreview` and `nativeCloseOperatorMenu` retire and release the feed within the existing drain/teardown boundaries. Rendering performs no gameplay or input manipulation.

The new publisher is **private and intentionally not called by the current read-only acceptance driver**. It is not a substitute for Chat 2's authoritative module manager; Chat 2 must supply real, source-backed activation state from its admitted owner lifecycle in a later integration commit.

## New actual JNI acceptance

`tests/java/dev/slinkywinky/attach/R209RetainedArrayListSmoke.java` and extra test-only methods in the disposable R76 WGL fixture exercise the real JDK 8 class, JNI export and retained GL11 renderer. This is registered as `r209_retained_jni_arraylist_owner_and_pixel` in CTest.

- No-session and before-first-paint publications are denied.
- A foreign Java thread is refused.
- Valid owner publication paints **Sprint** in the actual GL_BACK frame; a pixel on the accent strip is exact RGB `F35D12`.
- Requested-only **Timer** has no second-row stripe.
- A world-loss publication makes the ArrayList empty after a fresh clear and paint.
- Duplicate sequence retires/quarantines the feed; even another repaint cannot revive stale rows.
- Clean owner release and post-release publication denial verified.
- The fixture creates its **own disposable WGL window**. It does not start/attach Minecraft, send input, intercept packets or prove original live gameplay effects.

## Remaining acceptance required

1. Chat 2 module manager -> source-backed `ModuleControlState` -> JNI publisher, without duplicating the game render hook.
2. In the original 1.8.9 client, verify an active module's real in-world effect and synchronous unload/disable; this is separate from menu, WGL and synthetic state tests.
3. Genuine ClickGUI mouse/keyboard exclusivity and restoration, plus profile UI and settings application.
4. Source-faithful ArrayList typography/animation; the current GL11 row pitch/accent are first-party interim choices.
5. Ensure one agreed 66-registration-to-67-GUI-candidate mapping before claiming a complete catalog.

Keep PR #1 draft/unmerged until those acceptance gates are met.

## Verified Windows results (November 14, 2026)

- Clean Windows Release incremental build with JNI/Attach: exit code 0.
- Targeted real JDK 8 JNI/WGL test: `R209_JNI_RETAINED_WGL_ACTIVE_ONLY_ARRAYLIST_PIXEL_PASS` and `R209_OWNER_SEQUENCE_WORLD_LOSS_AND_UNLOAD_PASS`; exit code 0.
- Full configured Windows Release CTest suite: **99/99 PASS**, 34.72 seconds, exit code 0.
- No original Minecraft game process launched for R209; the latest original-game read-only receipt remains from R206.
