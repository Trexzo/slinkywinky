# R243 — Clutch prior-block ray preflight (reference branch)

Date: 2026-11-20. SlinkyWinky draft PR #2. No changes to PR #1.

## Original reference checked

The user's private `decrypted files.zip` was inspected in the session,
without executing the original DLL. The decompressed `.rdata` section has
SHA-256 `61eaa6bad74fd5cf0d374198506ec4f87fa042b5cf673e37ae01e87eb336706a`.
At section offset `0x18280`, the source distance prefixes are
`1,6,19,43,79,123,163,167`; all 167 following packed offsets have SHA-256
`04a82e7f286d1163d7a515290190c67631005df9c984604ee582e8985784da48`.
These are private source-data verification facts, not code embedded in Git.

## Behavior added

`clutch_prior_ray_preflight` performs phase-one **value-only** evaluation:

1. Validate bounded player geometry, range, motion/rotation inputs and
   evidence. If a minimum-height ground probe is occupied, return without
   requiring candidate block samples, matching the source's early exit.
2. Check whether the previous planned block was eligible and solid.
3. On a **copy** of the configured source random-walk bank, compute the
   original side-face randomized 3D target.
4. Only request source-world ray evidence if the aim passes both maximum
   angular cost and strict 4.49-block reach.
5. Return the precise from/to ray endpoints and expected block identity.
   Do not invent a ray hit, advance the owner's RNG, rotate the camera,
   press a key or place any blocks.

## Regression gates

The existing native original-Clutch candidate test now checks:
- Positive reachable previous candidate and bytewise-equivalent phase-two aim
  after applying a **verified null hit**.
- Obstructed previous block is not retained.
- Distant previous block requests no ray and falls back to source candidate scan.
- Incomplete prior solidity, prematurely supplied ray evidence and unobserved
  candidate blocks are refused.
- Occupied ground short-circuits without sampling candidate IDs.
- Preview does not mutate source RNG, even if it needs a ray.

## Limits and next stage

This is not a live JNI ray transaction. R239/R241 Java native preview still
refuses previous-block snapshots, and that refusal must remain until a native
transaction explicitly binds a phase-one receipt to **one** sampled tick,
world/player tokens, prior block and actual game-thread ray result.
A replayed tick, different world, extra ray or stale owner must fail closed.
After that integration, separately implement and validate the source rotation,
use-click, hotbar and restoration stages. Do not claim a functioning complete
Clutch module or 1:1 original Slinky parity from this preflight.

The decrypted archive, recovered proprietary instructions and original assets
remain outside the public repository. Both PRs stay draft/unmerged.
