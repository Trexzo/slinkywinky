# R84 — Windows target JVM identity fencing for original Minecraft acceptance

**Status:** manual-only acceptance safety; does not enable in-game menu rendering.

## Issue

The original vanilla 1.8.9 acceptance harness already required exactly one advertised `net.minecraft.client.main.Main` JVM and a numerical PID before each diagnostic. **A PID is not a durable process identity:** after a crash or restart, Windows may recycle that numeric PID for a different process. A new JVM could advertise the same class name, while its output or previously captured receipts are mistaken for the old run. The existing append-only target-stdout check does not alone establish process continuity.

## R84 changes

- Before any Attach, the manually invoked Python harness opens the selected Windows process **read-only** with `PROCESS_QUERY_LIMITED_INFORMATION`, requests its kernel `GetProcessTimes` creation `FILETIME`, closes the handle immediately and retains the nonzero birth timestamp in memory.
- The harness compares the current kernel creation timestamp against that original value **after each listing and before Attach**, **immediately after the external Attach receipt**, **after verifying the fresh target-JVM receipts**, and **after the final listing**.
- If the JVM has exited, the process cannot be inspected, or the PID now points at a process with a different creation time, R84 fails closed. It does not proceed to the next R75/R78 diagnostic or try to reset/restored a suspected old session.
- The Windows query does **not** inject, stop, start, suspend, signal, enumerate memory, change any process, modify Minecraft files or require elevating permissions. It never runs in the default nonexecuting plan-only path.
- Identity checks complement—do not replace—R73 exact original class SHA admission, R75 verified restoration, R78 read-only WGL admission, R83 preflight gate-release confirmation, and the exact target/stdout receipts introduced in R81/R82.

## Regressions and scope

`tests/test_original_189_live_acceptance.py` mocks the read-only birth query in its Windows fully synthetic execution tests so CI never needs a real Minecraft PID. An R84 negative control returns the **same PID and vanilla main-class listing** but changes the kernel creation timestamp just before the next stage, and requires a failure **without attempting the R75 callback Attach**. A separate unit check refuses a mismatched birth timestamp. The existing staged mock tests still prove that the R73 → R75 → R78 flow succeeds when the process identity is stable and all target receipts are valid.

This is a race-reduction guard, **not proof against a PID swap occurring during the tiny interval between the final pre-Attach check and the external Java Attach call**. The Java Attach helper and production original-class SHA admission remain essential independent protections.

**No real original-1.8.9 runtime acceptance has been performed.** The desktop remained offline during R84 development. PR #1 stays draft/unmerged and no native menu draw path is enabled in the original game.
