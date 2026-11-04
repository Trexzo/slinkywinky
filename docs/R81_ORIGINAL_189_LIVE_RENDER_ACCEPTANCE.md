# R81 — supervised original vanilla 1.8.9 render-evidence acceptance

**Status:** manual-only, disabled by default. No game started by the harness and no in-game native menu drawing enabled.

The R73/R75/R78 modes have offline fixture CI but **still require real original-game acceptance**. The older `tests/windows_minecraft_tick_acceptance.py` launches a Forge/Gradle development client and validates a separate tick path; it must **not** be treated as evidence for original vanilla renderer WGL ownership. R81 introduces a strictly separate verification script for a manually started, already-running original Mojang Minecraft 1.8.9 Java 8 process.

## Operator preparation

1. Back up the game instance, start the **unmodified original vanilla 1.8.9** normally with Java 8 and verify it renders before testing. **Do not use Forge, ForgeGradle, OptiFine or a modified game JAR.** R73 will independently verify the exact original loaded `ave` and `bfk` class SHA-256 before either callback mode is considered.
2. Confirm the original game JVM is the only Minecraft JVM running. Get its numeric PID from the native loader's `list` command. Do **not** run acceptance against friends' instances or a machine/process not under your control.
3. Record target JVM **standard output** to an existing append-only file. The target-side `System.out` messages are the receipts. A launcher-only `ATTACH_RESULT=SUCCESS`, `latest.log` without these messages, or evidence from an earlier run does **not** satisfy the test.
4. Review the plan, which cannot attach even if given a real PID:

```powershell
python tests/original_189_live_render_acceptance.py `
  --loader "C:\path\to\slinkywinky_loader.exe" `
  --jdk8 "C:\path\to\jdk8" `
  --pid 12345 `
  --target-stdout-log "C:\path\to\game-jvm-stdout.log" `
  --mode context
```

Only after separate operator approval, run the same command **with** `--execute`. Optional `--report "C:\path\to\r81-evidence.json"` creates a local sanitized, create-only receipt summary. Never point it at a report that must be preserved or overwritten.

## What an authorized execution checks

The harness **does not start Minecraft**. It first invokes the external loader's `list` command, requires **exactly one** running JVM identified by the vanilla main class `net.minecraft.client.main.Main`, requires that its PID equal `--pid`, and refuses an additional Forge/GradleStart launcher candidate.

For `--mode context`, it performs these diagnostics **sequentially**:

| Step | Original-only loader argument | Target JVM receipts required |
|---|---|---|
| 1 | `--render-preflight` | Exactly one fresh `R73_LOADED_ORIGINAL_VANILLA_189_RENDERER_PREFLIGHT_PASS` |
| 2 | `--render-callback=8` | Exactly one fresh `R75_ORIGINAL_RENDER_CLASS_RESTORE_SHA256_PASS`, `R75_RENDER_CALLBACK_BOUNDED_PASS accepted=8`, **and** `R81_ORIGINAL_RENDER_ADMISSION_RELEASED_PASS` |
| 3 | `--render-context=8` | The same two fresh R75 receipts **plus** `R78_READ_ONLY_WGL_CONTEXT_STABLE_PASS attempts=8 current=8 absent=0 invalid=0 changed=0 stable=true` and **finally** `R81_ORIGINAL_RENDER_ADMISSION_RELEASED_PASS` |

`--mode preflight` runs only step 1. `--mode callback` runs steps 1–2. All three diagnostics remain bounded by the production Java agent's existing eight-frame sampling and cleanup/restore requirements. The CLI sets only the required passive/original render opt-ins; it removes the synthetic Attach bypass and experimental gameplay hook opt-in.

**Do not initiate the next Attach until the post-release marker appears.** The renderer cleanup prints restoration and sample receipts *before* releasing the global instrumentation gate. R81 adds the new `R81_ORIGINAL_RENDER_ADMISSION_RELEASED_PASS` marker **only after** that release is confirmed, and the harness requires the receipts in order. This avoids an immediate follow-up Attach racing with the prior renderer cleanup.

**Fail closed** if any command rejects, if an exact receipt is absent/duplicated/out of order, if the target reports restoration uncertainty or callback failure, if target stdout rotates/truncates/is replaced, if a different JVM appears, or if the original process is no longer listed after the diagnostic sequence. A second Attach is never attempted after an unverified first/previous step. The script does not attempt to remove a leaked transformer or override a locked failed session.

Evidence is read starting at the file's **byte length immediately before each Attach**; older pass markers do not count. A supplied target stdout stream must not be redirected together with the external loader stdout, because those are independent proof levels.

## Limitations

Even a clean R81 result is **only source-specific diagnostic receipt evidence**. It cannot verify that the Minecraft game window was visually healthy after each stage, that GLFW/LWJGL or WGL state was completely restored, that the in-game ClickGUI is functional, or that an injectable client matches commercial Slinky. The native in-game menu remains disabled. An operator must separately visually inspect original game rendering and stability.

Automated CI runs `tests/test_original_189_live_acceptance.py` to test the plan-only path, strict receipts, negative markers, stale/replaced target logs, JVM ambiguity and invalid parameters, **without starting Java, Attach, Minecraft or native graphics**. This does not replace the supervised live run. Keep PR #1 in draft.
