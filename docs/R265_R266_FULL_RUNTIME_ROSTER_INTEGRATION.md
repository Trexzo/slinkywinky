# R265 / R266 — 67-descriptor runtime roster with explicit owner provenance

**Branch:** PR #1 `reconstruction/r1-core-fidelity`, draft/unmerged.
**Linked reconstruction:** PR #2 `integration/openslinky-fd9c592-reference`, also draft/unmerged.

## Contract boundary

The original menu exposes **67 candidate descriptors**. The separately
verified native pointer-table has **66 registrations**; `Chest Stealer`
is an extra descriptor, not a registered native owner. The R211
`SprintObservationPreview` native owner currently publishes only
three genuine source states: `No Jump Delay`, `No Slow`, `Sprint`.

Prior implementations had an authoritative three-row
`ModuleArrayListSnapshot` and a separate 67-name GUI catalog.
Those views were never combined into a complete, explicitly
provenance-labeled roster. Presenting all 66 as disabled/active would
have invented information that the three-module owner does not possess.

## R265 — versioned roster *projection*, not a wire ABI change

`include/slinkywinky/module_runtime_roster.hpp` introduces a
render-neutral, value-only **projection version 2**, consuming the existing
source snapshot **contract v1** without changing its layout or Java/JNI
interface. It renders all 67 GUI rows in exact catalog order with:

- Canonical name, GUI category, candidate registration and implementation
  capability inherited from the verified native catalog.
- `DescriptorOnly` (Chest Stealer), `Unobserved`,
  `NativeOwnerProbe`, and `OfflinePolicy` provenance states.
- `source_reported` independently from `requested_enabled` and
  `owner_reports_active`; no unreported name is labeled disabled.
- The original sequence, world generation and owner-reported world-ready
  state, copied as immutable values.

An incompatible source contract, invalid sequence, duplicate name,
malformed active/requested bits, wrong world generation, descriptor-only
state or a purported authoritative state for an unsupported PreviewOnly
module is rejected. Offline-policy rows are never silently called
live Minecraft modules. The existing `ModuleArrayListFeed` retains
its original monotonic sequence and retirement behavior.

This projection has no authority to activate module behavior.

## R266 — exact current PR #2 owner integration

The new `tests/test_r266_current_pr2_full_roster.cpp` links the actual
PR #2 `SprintObservationPreview`, PR #1 native source snapshot adapter
and the versioned roster projection, under the existing R261 exact-source
checkout and SHA pin. It verifies:

1. Three genuinely reported native states outside a world, including
   Sprint's remembered requested=true but active=false.
2. Actual native owner world entry and Sprint activation, without
   inventing active rows for the other 63 GUI entries.
3. No Slow native enable and authoritative source republish, preserving
   all unobserved descriptor states.
4. World loss clearing reported active states without deleting selected
   settings, and final owner retirement removing the roster.

The portable `r265_runtime_roster_67_66_source_provenance` test also
covers all 67 row identities, explicit offline provenance, unsupported
registered module rejection, invalid source version/sequence, duplicate
names, malformed state, world-loss and feed retirement.

The R261 cross-branch workflow now runs this as an **eighth** integration
CTest in addition to the prior seven. Historical R247 pinned-source
tests are unchanged. GitHub CI must pass on the final source revision
before the result is considered fully verified.

## Functional boundary / unfinished work

No new Minecraft gameplay module, injection action, packet interception,
OpenGL drawing, OS input hook, or profile serialization was implemented.
The original native manager still exposes **three** owner controls,
plus **three independent offline policy controls** elsewhere in PR #1.
The new 67-row view is an explicit availability/provenance model,
not proof of 66 playable modules.

Future steps require a versioned **native provider interface** to
supply owner-authenticated states for additional modules, paired JNI and
game-thread integration, verified callback semantics, source-specific
settings, and independent real-client acceptance. Add those providers
incrementally; never infer execution merely from a recovered descriptor.
