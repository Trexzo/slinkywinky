# R255 — GUI-controlled offline policies for three more modules

Date 2026-11-20. Chat 1 PR #1 draft/unmerged; PR #2 unchanged.

R254 recovered and parity-tested No Hit Delay, Safe Walk and Fast Place
value-only policy decisions. They had no selected-module GUI transport.
R255 extends the existing first-party `GuiModuleControlHandoff` to
admit only those three new offline module identities IN ADDITION to
the original three PR2 R211 controls (No Jump Delay, No Slow, Sprint).

New `RecoveredSourcePolicyGuiAdapter` forwards the source-focussed,
copy-only GUI requests to the R254 `RecoveredSourcePolicyOwner`.
The adapter validates the complete previous published module state
and world generation before dispatch, only changes the specific
source-module requested bit, obtains a new independent authoritative
snapshot from that owner, and passes it to the existing ACK and
ArrayList publication infrastructure. Source/view mismatches,
lost ACKs and wrong ownership fail closed and retire the feed.

A new **always-on first-party** test tests actual No Hit Delay,
Safe Walk and Fast Place source GUI navigation across all three
different tabs, separate GUI/native owner threads, inactive
preview until ACK, exactly one native active row per selected
module, loss-of-world inactive display with requested bit still
set, a post-world-loss disable acknowledgement, and retirement.
Only source ACKs control activity. Local GUI previews remain
inert throughout.

The rest of the 67-label catalog remains preview-only; the
extra Chest Stealer descriptor is excluded from runtime
registration. No third-party source is modified or copied.

**Still NOT a Minecraft injection or live module implementation.**
These three additional GUI controls switch purely offline
decision policy models; they do not wire Java/game callbacks.
The three existing PR2 R211 modules retain the separate R246/R247
owner adapter and original offline tests, unaffected.

## CI

Exact-commit results added after checks finish.
