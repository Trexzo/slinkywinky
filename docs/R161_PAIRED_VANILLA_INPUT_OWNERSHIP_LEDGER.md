# R161 - Paired original input ownership and restoration: fixture proof

The source-verified original Minecraft 1.8.9 input owner is ave.s()V, pinned at SHA-256 430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82. R119 establishes that mouse and keyboard queue redirection alone is unsafe: direct keyboard reads and held-keybinding consumers survive.

## Implemented

Fixture-only R161InputCoverageLedger records the minimum 16 admission obligations: original mouse queue, button reads, wheel reads, keyboard queue, raw key reads, pre/post key states and presses, held/pressed keybind polling, GUI dispatch, physical release, foreground, bytecode rollback and independent drain of both queues. The Java 8 smoke test requires the exact source-pin string, a monotonic epoch, a single owner thread, both bound queues and all 16 synthetic coverage bits. Removing any one bit rejects entry. Close requires both queues back in VANILLA and empty input backlogs.

This coverage ledger resides ONLY in tests/java. Synthetic integers, flags and the source-pin string are not live-game attestations. The pinned-class audit remains an independent negative gate and R161 does not patch ave.class.

R161PairedInputRollbackSmoke exercises existing dormant MouseQueueOwner and KeyboardQueueOwner with disposable org.lwjgl.input.Mouse and Keyboard fixtures. It validates paired modal entry, duplicate event filtering, bottom-left-to-top-left coordinate conversion, withholding from vanilla, separate release draining, restoration of normal source queue metadata, partial-owner refusal, stale input quarantine, monotonic epochs, foreign thread denial and coordinated focus-loss refusal.

These are positive tests of an isolated two-queue lifecycle, NOT proof of complete Minecraft input ownership: original ave.s()V still has direct key-state reads, GUI dispatch and keybind polling outside the queues. No original Minecraft process, WndProc hook, Java class transformation, JNI input dispatch or gameplay module is enabled.

The agent JAR must include only the dormant owner classes, excluding mock org.lwjgl.input classes and the R161 test ledger. R119/R160 source checks continue to prevent live activation.

## Next live acceptance requirements

Authentic evidence must cover all original ave.s()V input consumers and full original bytecode restoration. Verify client foreground, held-release drain, input event ownership, simultaneous mouse/keyboard epochs, focus-loss recovery and complete absence of gameplay input leaks in an isolated *original* Minecraft 1.8.9 process before enabling pointer routing. Draft PR #1 remains unmerged.
