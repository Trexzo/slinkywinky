# R90 — source-pinned HotSpot retransformation and live original-1.8.9 render acceptance

**Scope:** first-party native injectable client source and a manually authorized, isolated ORIGINAL Mojang Minecraft 1.8.9 Java 8 test. **No Forge, no other game JVMs touched, no in-game ClickGUI drawn, no gameplay-module controls, no network login.** PR #1 remains draft and unmerged.

## Why the preexisting original-game preflight failed

The verified original Mojang 1.8.9 JAR on the Windows test machine has SHA-1 `3870888a6c3d349d3771a3e9d16c9bf5e076b908` and SHA-256 `14f0d96d1a56fb4f5c3b2233d00699525893fe5ce3dcf181e7de59120595d298`. Source classes are independently pinned:

| Class | Original entry length | Original entry SHA-256 |
|---|---:|---|
| `ave` | 63,236 bytes | `430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82` |
| `bfk` | 35,133 bytes | `8a4ce93bbf272398ea298d2ed363f75211dd469fcd3b66682a387e4738a419a2` |

The first R90 test began Attach at the first LWJGL initialization line, before those classes were both loaded. It correctly failed closed.

After the isolated Java 8 JVM was launched with `-verbose:class` and the test waited for exact `[Loaded ave from file:/.../1.8.9.jar]` and `[Loaded bfk ...]` lines, the original preflight still rejected *raw class SHA*. Direct read-only diagnostics showed that Java 8 HotSpot's `Instrumentation.retransformClasses` provided both original classes at their **same byte lengths**, but changed roughly 22–29k serialized bytes. The raw SHA also changed between independent VM starts. The first instruction-transcript difference was method order: the JVM placed `<clinit>` ahead of another method in both classes. After sorting complete per-method and per-field ASM code-shape records, both loaded classes had **exactly the same execution-shape fingerprints** as the SHA-pinned original source.

## Source changes

- The R90 isolated original-client harness adds explicit `--run --render-context-acceptance` to start one offline, disposable vanilla Java 8 process with an isolated game directory and an exclusive, uniquely named stdout log. It refuses any other Minecraft JVM, requires exact source/classloader provenance, detects exact original-class load receipts, then runs the already established R81 three-stage renderer evidence checker with fresh target stdout.
- The R73 preflight retains strict original class **source SHA-256 and structural checks** on original `ave.class` and `bfk.class` resources from the same `1.8.9.jar` code-source location. It requires that each actually loaded/retransformed class have the same class header, fields, methods and per-instruction resolved ASM transcript as the pinned source, ignoring only VM member ordering, constant-pool layout and skipped debug/frame metadata. No unknown class identity or changed instruction may pass. An inspector registration/removal ambiguity remains fatal.
- The R75 finite callback verifies the source-pinned runtime renderer before any weave, applies its existing bounded Java-only `bfk.a(FJ)V` callback and rechecks original-source execution shape after removing the weaving transformer. The accurate new restoration marker is `R90_RESTORED_SOURCE_PINNED_EXECUTION_PASS`, rather than incorrectly claiming raw VM SHA equals the JAR entry.
- The existing R81 receipt verifier was updated to require the R90 restoration marker for both callback and WGL stages. The original PID-birth, exact loaded owner/thread, admission-release and fresh log checks remain in place.
- Offline class tests verify ASM reserialization is accepted, but source mutations and an injected NOP at the renderer entrypoint are rejected. No copyrighted classes, transformed binaries or customer data are written to Git.

## Verified connected Windows evidence

On the connected Windows computer, with installed Temurin OpenJDK 8u504 x64 and a local MSVC Release build:

- Complete native/Java8/JNI/Attach/disposable-WGL CTest: **25/25 PASS**.
- Python R81/R89/R90 receipt and negative controls: **22 tests PASS**, one platform-specific skip; vanilla evidence tests: **15/15 PASS**.
- Real original vanilla Java 8 render receipt run: **`R90_ORIGINAL_189_RENDER_DIAGNOSTIC_RECEIPTS_PASS`**, exact PID 35640. Evidence report was created in the isolated local workspace at `vanilla189-isolated/r90-render-receipts-35640.json`.
- R73 original source and runtime-shape preflight: PASS; original preflight admission release: PASS.
- R75: **8/8** callbacks on **Client thread#1**, then original-source execution shape restored and admission gate released: PASS.
- R78: **8/8 current WGL** samples, `absent=0 invalid=0 changed=0 stable=true`; source-pinned renderer restoration and release: PASS.
- The original Minecraft child process exited via the harness's own cleanup after all receipts were verified. No other Java processes or shared game directories were changed.

## Explicit limitations / next admission boundary

The verified evidence is **target JVM diagnostic receipt evidence**, not external inspection of a rendered game window, pixel-level stability, or an interactive in-game menu. The vanilla game did not receive any GL menu draw or combat input. The current code-shape check deliberately ignores certain non-executable classfile metadata, including debug and stack-map-frame encoding, and is not a claim of bit-for-bit equality of the VM's retransformation stream. The source JAR **is** still bit-for-bit SHA pinned.

Before production in-game ClickGUI drawing, introduce an independently testable first-party render-overlay admission with exact graphics-owner WGL checks, complete graphics-state restoration, safe disable, and a supervised visual-game acceptance run. Keep the draft PR unmerged until that happens.
