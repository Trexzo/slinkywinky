# R162 — original Minecraft 1.8.9 ordered input-callsite ledger

**Scope:** Read-only Java 8 / ASM 5.2 audit of the official Minecraft 1.8.9 `ave.s()V` method. The source belongs in `tests/java`, not in the Java instrument JAR. No original JVM is launched or modified, no input hook is attached, and no module performs gameplay actions.

## Verified Mojang provenance

- Vanilla JAR: 8,461,484 bytes, SHA-1 `3870888a6c3d349d3771a3e9d16c9bf5e076b908`.
- Extracted original `ave.class`: SHA-256 `430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82`.
- Method `ave.s()V`: 180 JVM method invocation instructions; 179 jump instructions; zero switches; three exception-handler entries.
- R162 input-related invocation ledger: **58 ordered source sites**. SHA-256 of its normalized ASM lexical callsite transcript:
  `6b58cf399b237ce7a84a84f1a62a8f5b56dc10e457f6217ea35aaadf99403e7f`.

The ledger includes mouse queue/button/wheel operations, keyboard queue/event/direct key reads, state/press binding dispatch, continuous held/press polling and GUI dispatch. It records each relevant instruction's relative method-invocation ordinal and a lexical phase: before mouse queue, after mouse but before keyboard queue, or after keyboard queue. The phases indicate **bytecode listing order, not control-flow dominance or reachability**.

The strict audit still finds 17 direct `Keyboard.isKeyDown` calls, 13 `avb.f` held-binding polls, three `avb.d` press-binding polls, three mouse-button-state reads and one mouse-wheel read. Queue substitution does not eliminate any of them.

## Count-preserving source mutation control

A separate, **in-memory-only** ASM visitor exchanges one `avb.f()Z` and one `avb.d()Z` call in the verified original `ave.s()V`. The previous R119 audit sees the same aggregate method counts (17 direct keys, 13 held polls, three press polls, one mouse queue); the invocation count and jump/handler totals also stay unchanged. R162's ordered transcript fingerprint changes, so the mutation is detected. The original class bytes are checked unchanged, and the modified bytes are refused by the original-source SHA pin.

R111's *in-memory-only* single `Keyboard.next` substitution was independently compared against the transcript. Canonicalizing the single substituted queue owner's name results in an **identical** input-call sequence fingerprint and the same branch totals. This is **evidence that the unsafe residual input consumers remain**, not an approval to deploy the rewrite.

## CI and release behavior

`R162InputOrderLedger` and `R162InputOrderLedgerSmoke` are compiled into `slinkywinky_render_audit.jar`, with fixture-only CTest and JAR-separation assertions. The production agent must exclude both. The official Mojang downloader runs R162 on the SHA-1-verified ephemeral JAR on the dedicated original-render-audit GitHub Actions workflow; it validates the exact source-order fingerprint and mutation controls. The game binary is never committed, uploaded as an artifact, or installed in a running game process.

This audit does **not** verify input-path control flow in the general case, OS foreground ownership, vanilla-key restoration or safe gameplay-input suppression. Those require an independent complete-method input plan and original-client acceptance. Mouse/keyboard owners remain dormant. PR #1 stays draft and unmerged.
