# R119 — Source-pinned vanilla Minecraft 1.8.9 input-ownership denial

**Target:** Trexzo/slinkywinky / `reconstruction/r1-core-fidelity`, draft PR #1.
**Scope:** offline audit and synthetic tests. This revision does **not**
install or arm a real Minecraft keyboard hook.

## Original source and provenance

The exact Mojang vanilla Minecraft 1.8.9 JAR already used by the
project's supervised tests was inspected **read-only** and authenticated:
- Original client JAR size: **8,461,484** bytes
- Original client SHA-1: `3870888a6c3d349d3771a3e9d16c9bf5e076b908`
- Original `ave.class` SHA-256:
  `430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82`
- Exact input method: `ave.s()V`. Only one original game method with this
  signature is admitted by the R119 verifier.

The `decrypted files.zip` archive supplied by the user is the source
for GUI layout and module descriptors in R115–R118; the independent
Mojang `ave.class` source pin is the basis for **this** input safety
finding. The audit does not execute the decrypted DLL or its injector.

## Actual original input method, not inferred from the GUI

The read-only ASM visit and `javap -c -p` confirm the following **within
`ave.s()V`**, in source program order:

| Input operation | Exact call count | Consequence |
|---|---:|---|
| `org.lwjgl.input.Mouse.next()` | 1 | Mouse event loop is processed before the keyboard queue |
| `Mouse.getEventButtonState()` | 3 | Independent mouse event state reads |
| `Mouse.getEventDWheel()` | 1 | Independent mouse wheel event handling |
| `Keyboard.next()` | 1 | One keyboard event queue owner |
| `Keyboard.getEventKey()` | 2 | Normal keyboard event metadata |
| `Keyboard.getEventKeyState()` | 3 | Normal keyboard event down/up metadata |
| `Keyboard.isKeyDown(int)` | **17** | Raw polled key states outside the `next()` call, inside the method |
| `avb.a(int,boolean)` | 2 | One before and one after the keyboard queue (mouse and keyboard bindings) |
| `avb.a(int)` | 2 | One before and one after the keyboard queue |
| `avb.f()` | **13** | Post-queue continuous held-keybind polling |
| `avb.d()` | **3** | Post-queue keybind press polling |

All **17** `Keyboard.isKeyDown` calls appear after the
`Keyboard.next` call. The original loop also calls the GUI owner's
`axu` methods. Thus replacing `Keyboard.next()` alone, as the
experimental R111 bridge does in memory, cannot prove that
the rest of Minecraft's input handling is suspended.

**One-call rewrite proof:** R119 invokes the R111 exact-source,
SHA-pinned, one-call-only in-memory weave. It then independently
re-inspects the transformed class: the raw `Keyboard.next` call is
replaced by exactly one `KeyboardQueueOwner.next`, but **every**
other count above remains unchanged. No transformed class bytes are
installed in any JVM. The original class bytes are unchanged.

## New tests and runtime separation

- `tests/java/dev/slinkywinky/attach/OriginalInputOwnershipAudit.java`
  is test-only source. It requires valid class magic and a single
  `ave.s()V` owner; its full strict original method audit accepts only
  the pinned `ave.class` SHA-256 and exact structural counts. It
  deliberately **refuses** queue-only modal admission if any game
  mouse/input/keybind path survives. Even an empty synthetic method is
  not affirmative proof of a safe complete input-owner/restore cycle.
- `OriginalInputOwnershipSmoke.java` first exercises generated
  malformed, wrong-hash and wrong-owner fixtures. With a supplied
  original vanilla JAR, it verifies the real `ave.class` and re-inspects
  the R111 in-memory patched version; the patched version remains
  explicitly unsafe for exclusive gameplay-input ownership.
- `R112KeyboardTarget.java` has two new **fixture-only** counters
  before/after its synthetic keyboard loop. The existing isolated
  Java 8 classloader test verifies both counters still increment during
  modal queue withholding. This is executable evidence of an unsafe
  *queue-only* isolation assumption, **not** an original-game input test.
- The Java 8 `native_agent_attach` CTest runs the synthetic R119
  denial and an instrument-JAR manifest check. The runtime JAR must
  contain the existing dormant `KeyboardQueueOwner` /
  `KeyboardPageRelay` classes but must **not** package the disposable
  `org.lwjgl.input.Keyboard` test mock or R119 test/audit classes.
- `tests/test_r119_live_input_guard.py` rejects introduction of a
  direct queue-only weaver invocation or a `KeyboardQueueOwner.arm`
  / `bind` call in the production `src`, `include`, or
  `loader/java` source trees until full input ownership is proven.
  This is a source-level regression gate, not a live performance test.

R119 also removes duplicate references to the R118
`KeyboardPageRelaySmoke.java` test source in CMake.

## Missing evidence before live input can be admitted

1. A verified full-method input ownership design covering **both**
   mouse/keyboard event queues, all direct key-state access and all
   held-/press-binding consumers. A single queue substitution is
   insufficient.
2. An independent, version-pinned game-class admission test before
   any bytecode transformation, plus an exact restore/rollback receipt.
3. A bounded input drain and proof that GUI-owned key-up/button-up
   events cannot reappear as held gameplay controls.
4. Separate live 1.8.9 keyboard/mouse focus-loss tests and recovery
   tests **before** exposing input interception to normal users.

Skipping all of `ave.s()V` is a potential offline experiment, not
yet a safe or verified fix: it also includes mouse, GUI and other game
tick behavior. R119 does not implement or recommend deploying such an
unproven method hook.

## Verification and release status

- The standalone GUI source and R118 input navigation remain intact.
- Original-game class analysis and in-memory rewrite: **PASS** on the
  exact local 1.8.9 Mojang JAR.
- Complete Windows x64 Release build: **PASS**.
- CTest **31/31 PASS**. The Java 8 attach test runs the new synthetic
  residual-dispatch denial and verifies input fixtures are absent from
  the agent JAR.
- Python unittest **57 tests run, one skipped, zero failures**;
  includes two new negative live-hook source admission checks.
- Original `ave.class` audit against the SHA-pinned local 1.8.9 JAR:
  **PASS**, including in-memory queue-only reweave and denial.
- Separately supervised original 1.8.9 Java 8 JVM, **PID 17752**:
  R94 scripted operator show/hide/stop **PASS** and normally exited.
  This is **not** physical mouse/keyboard or live input interception
  acceptance.
- GitHub workflows must be checked independently after push.
- PR #1 is deliberately **draft and unmerged**.
