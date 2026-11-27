# R348 — Real in-game native packet-verdict telemetry, no packet changes

R347/R347.1 transfers actual read-only native per-packet source decisions from a bounded primitive queue to a Netty reader, but is still an isolated JNI test path. This milestone adds **actual runtime evidence** to the existing opt-in Forge 1.8.9 singleplayer R339 Netty observer, without causing a second native packet callback or enabling any real network hold/release.

## Live Forge behavior

- `SourceWorldReadOnlyHost.observePackets` already invokes original-table Backtrack event2, Lag Range event3 and Timer Range event1 through the real native module execution bridge for each **previously forwarded** Minecraft packet captured by the passive Netty tap. Now, its `PacketReceipt` reports the genuine native verdict counts: `backtrackHold`, `backtrackFeedback`, `lagRangeHold`, `timerHold`, in addition to the earlier callback/observation counts.
- For incoming S18 entity teleports, the original Backtrack source treats them as target movement (R351 corrected the prior S08/S18 conflation), not as a player teleport stop, while unknown-ID S19 status is never misreported as target feedback. All JNI verdict fields must belong to the supported enum/bool domains; invalid native results retire the source session rather than becoming false telemetry.
- New explicit flag `-Dslinkywinky.r348NativePacketVerdictTelemetryReadOnly=true` requires the existing R337 native world owner and R339 passive packet observer flags, with manual F9 opt-in and local singleplayer only. The first nonempty batch prints a single `R348_REAL_NATIVE_PACKET_VERDICT_TELEMETRY` receipt with proposal counts, native world generation and the explicit `packetsAlreadyForwarded=1 gameWrites=0` statement. No ongoing per-packet log spam.
- Neither R344/R345 real packet-owning test handlers is deployed or called; all original packet and promise references in the actual game still follow their original channel pipeline exactly once. Native outputs are read-only counters, not actions.

## Offline verification

New real Java 8/JNI CTest `r348_java8_live_shaped_native_packet_telemetry`: a deliberately explicit synthetic source reach eligibility fixture triggers one actual native Backtrack e2 hold, one matching-target pass-feedback, and one Lag Range e3 closer-movement hold. Checks the exact callback totals, validates Timer Range event1 proposal domain, verifies empty batches never invent decisions, and ensures direct world replacement revokes the old primitive Netty queue. ForgeGradle builds the same production host + runtime logging and asserts proper opt-in and `packetsAlreadyForwarded` language in the packaged JAR workflow.

## Outstanding

This is an *actual in-game diagnostic integration*, but it still does not implement original packet holding or live verdict-to-Netty owner admission. Real Forge source reach admission remains intentionally unknown (0), so Backtrack active/hold will typically remain disabled in live mode; the offline test's explicit reach=true is not evidence that native Slinky would activate in a real client. Future work requires accurate source reach and packet hook timing, real event-loop bounded admission and no-world-leak rollback, and real process acceptance. Full original Slinky modules/ClickGUI remain incomplete, PR #2 draft and unmerged, PR #1 unchanged.