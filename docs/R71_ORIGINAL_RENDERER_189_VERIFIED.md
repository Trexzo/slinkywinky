# R71 — verified Mojang vanilla 1.8.9 renderer identification

**Date:** 2026-11-02 · **Draft PR:** #1 · **Status:** original classfile audit verified, **not** a live render hook or working in-game GUI.

## Previously unresolved gate

R64–R70 had only public deobfuscated `EntityRenderer.updateCameraAndRender(float,long)` references and generated Java fixtures. Without the original shipped Mojang 1.8.9 JAR, there was no independent evidence of the exact obfuscated renderer method. The authorized Windows desktop remains offline, so live-game tests are still blocked.

R71 resolves **the original bytecode identity** without the desktop by running a separate GitHub Actions workflow that uses Mojang's published 1.8.9 metadata and downloads the original client **only temporarily on the Windows runner**.

## Confirmed original Mojang client identity

- Release: `1.8.9`
- Original client size: `8,461,484` bytes
- Original client SHA-1: `3870888a6c3d349d3771a3e9d16c9bf5e076b908`
- Original obfuscated Minecraft method containing the render invocation: `ave.av`
- Original instance renderer class: `bfk`
- Original renderer method: `bfk.a(FJ)V` — accepts `float,long` and returns `void`.
- Original `ave.class` SHA-256: `430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82`.
- Original `bfk.class` SHA-256: `8a4ce93bbf272398ea298d2ed363f75211dd469fcd3b66682a387e4738a419a2`.

These are outputs from the actual original bytecode verified by the existing ASM 5.2 `RenderCallsiteAudit`, not inferences from MCP deobfuscated names. They describe the **exact vanilla 1.8.9 client only**, not modded clients, other Minecraft versions or any commercial Slinky binary.

**Source workflow:** https://github.com/Trexzo/slinkywinky/actions/runs/41424516271 — successful original JAR audit, method identification, classfile digests and repository cleanliness.

**Published metadata:** https://piston-meta.mojang.com/v1/packages/d546f1707a3f2b7d034eece5ea2e311eda875787/1.8.9.json

## Workflow and data handling

- `tests/official_189_render_audit.py`: downloads metadata from the official `piston-meta.mojang.com` version manifest, rejects mismatched release ID, expected client SHA-1/size and non-Mojang download origins. Only `launcher.mojang.com` and `piston-data.mojang.com` are allowed client sources. The download is size-limited while streaming and its SHA-1 is verified before running Java.
- `.github/workflows/original-189-audit.yml`: independently compiles the Windows Java 8 read-only verifier, downloads the game into a short-lived Python `TemporaryDirectory`, runs `RenderCallsiteAudit`, verifies the *unique* obfuscated renderer candidate and both pinned classfile digests, then requires a clean Git workspace. It never uploads or commits the game JAR, classfiles, transformed bytes, or any vendor files.
- `tests/java/dev/slinkywinky/attach/RenderCallsiteAudit.java`: scans `ave.class`, requires a field-backed `INVOKEVIRTUAL (FJ)V` from the same Minecraft method, verifies a public instance target method on its renderer class, and rejects zero or ambiguous candidates. R71 adds SHA-256 fingerprints of the unmodified Minecraft and renderer classfiles.
- The separate original-client audit is **not** a required release workflow or evidence of correct in-game behavior. The standard native injector tests remain separate and must stay green.
- Python `__pycache__` from the first audit's syntax-check step initially triggered the workspace cleanliness negative control despite the bytecode audit passing. The syntax-check step was changed to use `ast.parse` (no cache files), and the second official-client audit run passed completely.

## What R71 does *not* prove

The real game has not been launched or modified in R71. There is no Minecraft graphics-context callback, no LWJGL OpenGL compatibility proof, no working in-game ClickGUI, no gameplay module implementation and no claim that the recovered Slinky behavior is complete. Matching these exact vanilla class hashes would be necessary, but not alone sufficient, to authorize a future live render hook. The next stage should use the immutable original `ave` and `bfk` fingerprints to test **test-only** ASM transformation of the actual `bfk.a(FJ)V` method offline (without redistributing code), then separately inspect real 1.8.9 render-thread and GL state admission on an authorized desktop before enabling in-game drawing.

PR #1 remains draft and unmerged.
