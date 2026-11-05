# R82 — exact target receipts and bounded simulated original-game acceptance

**Status:** offline CI fixture and safer manual-only acceptance checks; **not a completed live Minecraft test**.

## Why R82 was necessary

R81's original vanilla 1.8.9 acceptance harness used text-substring matching for target-JVM success receipts. This could falsely classify an extended or malformed value as the required result (for example `accepted=80` contains `accepted=8`, and `stable=trueee` contains `stable=true`). A copied loader success line or a prefixed/extra target log message also is not proof that the intended hook completed. These are false-acceptance hazards, not proof of a successful in-game operation.

## Changes

- `validate_receipt` now matches **full JVM stdout lines** with start/end anchors, not substring counts:
  - Exact R73 SHA-pinned original preflight marker.
  - Exact R75 pinned original-renderer SHA restoration marker.
  - R75 bounded result with **exactly** `accepted=8` followed by a nonempty bounded `owner=<Java thread>#<positive thread id>` field.
  - Exact R78 `attempts=8 current=8 absent=0 invalid=0 changed=0 stable=true` WGL sample summary.
  - Exact R81 **post-release** admission marker.
- One complete line of each expected type is required. A duplicate, unexpected stage, malformed/prefixed/suffixed pass-looking line, R75/R78 negative marker, external `ATTACH_RESULT=SUCCESS` contamination or out-of-order restoration/release remains fail-closed.
- Accepts normal Windows `CRLF` target output and owner-thread names containing spaces.
- Bounds the fresh target-JVM stdout window to **256 KiB per stage**. A larger or concurrently growing window fails instead of allocating or scanning unbounded logs. The existing stale/rotated/replaced log checks remain active.
- Extends `tests/test_original_189_live_acceptance.py` with exact-line collision and malformed-positive negative controls, CRLF acceptance, bounded log tests, and a **fully mocked Windows `--execute` orchestration** test using temporary files. The mock delivers R73 → R75 → R78 target receipts through a fake external loader, verifies the create-only sanitized report, and separately confirms that an R75 restoration-uncertain marker prevents the third Attach. **These tests do not start a JVM, Minecraft or any native DLL.**

## Operator scope remains unchanged

The actual `--execute` flag still requires explicit operator action, the only identified vanilla Minecraft 1.8.9 JVM, real Java 8 and native-loader paths, and an append-only target-JVM stdout log. The original Mojang fingerprint checks, callback limit, restoration policy and R79 drain remain unchanged.

**No live in-game menu drawing** is enabled by R82. A CI pass establishes that the acceptance harness refuses false receipts, not that the original game renders or that the injector is visually correct. Keep PR #1 draft/unmerged pending supervised original-game GL context and restoration inspection.
