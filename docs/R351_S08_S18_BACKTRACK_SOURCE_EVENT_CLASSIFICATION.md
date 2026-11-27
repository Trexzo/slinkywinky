# R351 — Native Backtrack packet classification: S08 is not S18

## Original-source mismatch
Private `c++_possibly_OpenSlinky-main.zip`,
`src/logic/combat/backtrack.cpp`, shows
`backtrack_packet_hook_under` stopping only on
`S08PacketPlayerPosLook` (local player teleport), or optionally
on the local player's velocity packet. Its separate
`src/engine/modlib/reach_distance_checks.cpp` uses
`S18PacketEntityTeleport` as a target-position update.

R347/R348 previously mapped both S08 and S18 to native teleport-stop
kind 5. That was incorrect for S18.

## Correction
`SourcePacketReadOnlyQueue.sourceBacktrackKind` centrally maps
incoming packet facts to source event2 kinds; both
`SourcePacketVerdictBridge` and `SourceWorldReadOnlyHost`
use it. S08 maps to 5; S18 maps to 0 (normal possibly-held
entity movement), while `SourceRemotePositionHistory.packet`
still validates S18's fixed-point target position provenance.

## Regression
- Existing R347 Java8 JNI FIFO smoke now provides S18->S08->unknown
  following a fresh genuine native tick. Asserts S18 hold candidate,
  S08 stops, later unknown unchanged and strict 3-entry FIFO.
- Existing R348 live-shaped real-JNI telemetry smoke now measures
  6 packets/7 callbacks with exactly 2 Backtrack hold candidates,
  1 target-status feedback, 1 Lag Range hold and no post-S08 hold.
- Existing R347/R348 build and ForgeGradle workflows compile these
  tests; no new native packet owner or Forge runtime action has been
  installed.

**All packet proposals remain read-only and retrospective.** No traffic
is withheld, modified, retransmitted or consumed. This does not
establish original network-thread timing, packet-held state fidelity,
standalone injector parity, multiplayer safety or completed Slinky.
Keep PR #2 draft and PR #1 unchanged.
