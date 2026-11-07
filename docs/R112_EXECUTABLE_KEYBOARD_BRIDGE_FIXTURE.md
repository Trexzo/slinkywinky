# R112 — executable Java 8 keyboard bridge fixture

**Base:** `6c74090` R111 SHA-pinned original 1.8.9 keyboard bridge. This is an additive regression test on the existing unmerged draft PR #1. It does **not** enable live keyboard interception.

## Purpose and design

R111 already verifies one original `ave.s()V` bytecode callsite in memory, but the input gate was still tested separately. R112 exercises a **real transformed Java class in an isolated JVM** against the production-compiled R110 `KeyboardQueueOwner`.

`OriginalKeyboardBridgeWeaver` retains its exact original Mojang SHA-256 admission, pre/post-queue `avb` dispatch counts, unique `Keyboard.next()Z` gate, and same-signature ASM replacement. The one-call replacement is extracted into a single private helper, shared by original-source dry-run and a **test-only** method that strictly accepts `dev/slinkywinky/attach/R112KeyboardTarget`. No generic arbitrary-class transformation is authorized, and the latter can never be reached by a production loader mode unless somebody explicitly calls the test method.

`R112KeyboardBridgeSmoke` compiles and loads a generated `R112KeyboardTarget` into two disposable Java 8 classloaders using the test-only fake LWJGL2 keyboard. It establishes baseline vanilla queue behavior, applies the *same* one-call ASM replacement function, verifies that it changed only one instruction and no method signatures, then **executes** the rewritten class:

- With R110's gateway bound but modal input unarmed, vanilla key-down/up events are forwarded to the test class without altering the original event metadata.
- With a fresh foreground heartbeat, release attestation and isolated modal owner, the transformed `run()V` uses `KeyboardQueueOwner.next()Z`; the input owner consumes and reports key-down/up edges without delivering them to the test class. Repeated held keys do not duplicate actions.
- After a verified menu-close/release-drain transition, original queue events resume on the next `run()V` invocation.
- Negative controls reject mismatched Mojang source bytes, truncated/generated wrong bytecode, preinstrumented fixture, missing `Keyboard.next()`, and duplicated calls.

The Windows CMake test target compiles this fixture only into the disposable Attach/JVM test classes and registers it in `tests/attach_smoke.py`. The production Java 8 agent has only the first-party bridge and keyboard owner classes. It does **not** package the fake `org.lwjgl.input.Keyboard`, generated `R112KeyboardTarget`, the fixture runner, or proprietary Mojang bytecode.

## Tests and limitations

A fresh Windows x64 Release build passed the R112 executable classloader fixture and the original R111 offline SHA-pinned Mojang audit, confirming that the refactor retained its existing source contract. Full Release CTest **30/30 PASS** and Python **55 run, one skipped, zero failures**. A separate supervised isolated original Minecraft 1.8.9 JVM (PID **18824**) passed the existing R94 **scripted operator** show/hide/stop receipt, then exited; this is not a live R112 keyboard-input acceptance. JAR inspection confirmed six first-party bridge class entries in the instrument JAR and **zero** fake-LWJGL, synthetic-game, or proprietary Mojang class entries. The original Minecraft JAR remains unchanged and is not redistributed. Remote GitHub CI for the new commit must be checked independently.

**This is not physical keyboard or live Minecraft pass-through certification.** The actual original `ave.s()V` still contains `avb` pre/post queue keybinding calls. R112 does not install a ClassFileTransformer into a Minecraft game, does not alter OS keyboard state, does not prove suppression of pre-loop controls, and does not bind the C++ ClickGUI. Original-game input ownership, focus/viewport admission, reversible class retransformation and real click-through tests remain separate prerequisites. No Forge conversion, combat enablement, or anti-cheat bypass is introduced.
