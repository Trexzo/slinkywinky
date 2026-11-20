# R256 — Recovered Hitboxes and Ghost Hand event-8 decision models

Date: 2026-11-20. Chat 1 draft PR #1; PR #2 untouched.

## Original pinned source

Reviewed **Skid-Technology/OpenSlinky** at exact public source commit
\`fd9c5922da34dd59c5145bf4c897b79ff1bbd5a4\`:

- \`src/logic/combat/hitboxes.cpp\` and \`.h\`: module combat
  category 0, descriptor reach-query callback slot \`+0xc8\`
  (event 8). It marks the query active and copies its configured
  per-side expansion. The first-party model prevents negative,
  nonfinite or >16-block expansion (an **intentional** bounds
  deviation, not pixel/byte parity).
- \`src/logic/block/ghost_hand.cpp\` and \`.h\`: block category 3,
  same descriptor event 8 active-marker callback, plus a separate
  \`ghost_hand_should_ignore_entity\` helper. That helper requires
  the module to be enabled, optionally checks physical mouse state,
  and categorizes 1.8 item IDs into fists, swords, tools/shears,
  buckets, flint-and-steel, cobweb or other. For player entities it
  applies **bot first**, then friend/teammate flags, then enemy
  preferences. A non-player uses the independent non-player flag.

R256 implements these as NEW independently written, typed C++17
\`recovered_entity_query_policies.hpp\` decisions, with module enable
state strictly held by \`RecoveredEntityQueryOwner\`. It checks owner
thread, generation, world readiness and event number on every proposal
and exposes source-only requested/active state through the existing
R207 ArrayList snapshot validator. Neither the owner nor the GUI
preview can install a Minecraft/JVM hook.

## Tests

Always-on \`r256_source_hitboxes_ghost_hand_reach_query_event8\` checks
both native registration identities and original GUI category/name,
complete item-ID groups, 512 flag+option+mouse combinations including
source bot/friend precedence, exact event-8 gating, selected-vs-active
snapshot, world loss/reentry, stale generation, foreign thread and
permanent retirement.

**Scope:** These are two additional offline callback policies, not
two newly live game modules. They are not yet wired to PR1
\`GuiModuleControlHandoff\`; Reach and Piercing implementations and
their shared source event arbitration are next. Hitboxes and Ghost Hand
are both offline simulation models, not directly executed
OpenSlinky code.

The original source has 66 registered native module identities
(plus one extra GUI-only candidate). Full asset/client build,
remaining callbacks and original Minecraft runtime parity are
still incomplete. PR #2 is unchanged.

## CI
Populate with exact commit results after GitHub checks.
