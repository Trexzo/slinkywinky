# R163 — original Minecraft 1.8.9 input-control-flow graph

**Scope:** Java 8 / ASM 5.2 read-only audit of the exact Mojang `ave.s()V` input method. No Minecraft process is launched or modified. The first-party analysis code is test-only and must be excluded from the injectable agent.

## Source and graph provenance

The previous R119/R162 gates still require the exact Mojang vanilla 1.8.9 client JAR (size 8,461,484; SHA-1 `3870888a6c3d349d3771a3e9d16c9bf5e076b908`), exact original `ave.class` SHA-256 `430a17aa1911bcd0251aa4dcecb013b6c5b251bb5d0122c7305de0e36dc33e82` and prior ordered input-call fingerprint `6b58cf399b237ce7a84a84f1a62a8f5b56dc10e457f6217ea35aaadf99403e7f`.

R163 builds a conservative static control-flow graph from every bytecode instruction in `ave.s()V`. It resolves branch labels, lexical fallthrough, return/throw exits, and try/catch edges. It then computes a SHA-256 transcript of exact instruction opcodes, operands/owner identity (normalizing only the dormant R111 keyboard-queue substitution), and resolved successor edges.

**Original graph:** 1,009 instructions, 179 jumps, three exception handlers, nine conservative exceptional edges, and 58 input-related invocation sites. All 58 are conservatively reachable from method entry, including 17 direct `Keyboard.isKeyDown` calls, 13 `avb.f` held-binding polls and three `avb.d` press-binding polls. Original graph SHA-256: `2b9173842b6b507e45190da633e9c83d5f9d0e5d2969ff181995b8970fd97256`.

This is **static over-approximate reachability**. It does not prove that each path executes at runtime, model exception throwing precisely, establish dominance/control dependence, or establish that raw keys or held bindings can safely be suppressed.

## Negative control and restoration

An in-memory-only ASM visitor inverts the first supported conditional branch in original `ave.s()V`. The R119 call totals, R162 ordered callsite digest, invocation count, branch count and exception-handler totals remain unchanged. The R163 control-flow transcript changes and the altered class fails the original SHA admission. Original bytes remain unchanged.

A separate original-class R111 Keyboard.next-only dry-run has the same canonical R163 CFG fingerprint as untouched vanilla `ave.s()V`, demonstrating that it does **not** remove residual input consumers. The rollback test constructs a fresh byte-exact copy of the original resource and revalidates the R162 and R163 pins. This is **offline byte copying only**, not restoring class bytes inside an original game JVM.

## Packaging, CI and remaining release gate

R163 analysis and smoke test classes are compiled into `slinkywinky_render_audit.jar`, not `slinkywinky_instrument.jar`. The R163 Java 8 CTest fixture exercises a synthetic conditional-branch negative control. The official original-Mojang GitHub audit runs the exact SHA-pinned Java 8 graph test on an ephemeral original JAR and verifies the strict fingerprint, in-memory branch inversion, and queue-only equivalence. Test/agent separation is independently asserted by the Java attach test.

**No runtime input hook is added.** Mouse and keyboard owners remain dormant. R163 is not full original-client input-ownership acceptance, input rollback, or restoration of all 67 modules. A deployed GUI input gateway still requires authenticated whole-method vanilla input-path suppression, foreground and physical-key release tests, complete restoration of loaded game bytecode, and supervised original Minecraft 1.8.9 acceptance. Draft PR #1 stays unmerged.
