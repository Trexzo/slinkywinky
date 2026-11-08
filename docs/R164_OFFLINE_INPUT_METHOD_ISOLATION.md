# R164 — offline full-method GUI-mode input cut

**Scope:** first-party Java 8/ASM 5.2, test-only and read-only original-class investigation. Files live under `tests/java/dev/slinkywinky/attach`. The candidate guard and its `R164OfflineModalGate` exist only in `slinkywinky_render_audit.jar`; none of these classes is in the injectable Java agent.

## Motivation

R119 and R162/R163 established that original Minecraft 1.8.9 `ave.s()V` contains far more vanilla input consumers than the `Mouse.next()` and `Keyboard.next()` loops. A mouse-only or keyboard-only replacement cannot provide exclusive GUI ownership. The original method has 58 source-relevant input callsites, including 17 direct key-state reads, 13 held-keybind polls and three press-keybind polls.

R164 tests a **hypothetical entry cut** for the complete method rather than rewriting individual queues. A test-only `shouldQuarantineInput()Z` predicate is inserted before the original instructions:

```
if (R164OfflineModalGate.shouldQuarantineInput()) {
    return; // bypass ave.s()V only, not other game methods
}
// existing vanilla ave.s()V, otherwise unchanged
```

The predicate is a fixture-controlled Java boolean, **not** an authenticated render/game-thread foreground, input-ownership or restoration receipt. No production code calls this weaver or installs the result. There is no claim the entire game's input is quarantined; other input consumers outside `ave.s()V` have not been audited.

## Positive and negative controls

The disposable Java 8 fixture loads a generated `R164TinyInputTarget` using an isolated child classloader. With the fixture gate false, the original method increments a counter; with it true, the early return leaves the counter unchanged; setting it false again restores the original behavior.

For the exact SHA-pinned original Mojang 1.8.9 `ave.class`, the test creates *only an in-memory candidate* and checks the inserted `invokestatic → ifeq vanilla → return` prefix. All 58 source input callsites remain in the same order, original category, signature and lexical phase, with one extra call ordinal from the test gate. The original body gains only three instructions and one new branch; original protected-region exceptions and method consumer counts remain unchanged.

Negative controls invert the gate branch, remove the early return, supply a missing candidate, pass unmodified original bytes, and attempt to weave an already transformed class. Each must refuse. The only 'restoration' tested is a byte-exact copy of the stored original source; loaded-class retransformation rollback is **not** established.

## Packaging and release gate

The full Java 8 attach-packaging suite asserts the R164 classes exist in the read-only audit JAR and are absent from `slinkywinky_instrument.jar`. A CTest fixture covers executable gate behavior without any original game process. The independently configured original-Mojang GitHub Actions workflow runs the strict original-class test and rejects any new / missing consumer or reversed/missing gate instruction.

**Status:** offline design and tests only. R164 does not restore live mouse routing, original game input-mode ownership, the in-game ClickGUI, or any of the remaining 67 catalog modules. The draft PR remains unmerged. A live hook requires a separately authenticated runtime input owner, correct render/game-thread dispatch, exhaustive out-of-method input audit, a reversible original-bytecode restoration mechanism, and original Minecraft acceptance before it can be enabled.
