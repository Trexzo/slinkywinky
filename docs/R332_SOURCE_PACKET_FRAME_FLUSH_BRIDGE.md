# R332 — six source callback paths integrated with existing offline event replay

R332 connects **packet and lifecycle proposals** for the four previously absent modules to the SAME `ModuleExecutionBridge` and controllers introduced in R329–R331.

| Callback | Exact original table slot and priority | Offline controller behavior |
| --- | --- | --- |
| Backtrack incoming | Event 2 / 0 | Time-budget hold/feedback passthrough and source ownership check |
| Lag Range outbound | Event 3 / −100 | Relative movement packet-hold and release condition |
| Timer Range packet | Event 1 / 0 | Reads source freeze-before packet gate |
| Timer Range frame | Event 3 / 0 | Bounded supplied ray steps, timer/burst proposals |
| Inv Walk outgoing GUI | Event 23 / 0 | Open/click/close queue admission (count only) |
| Inv Walk inventory tick | Event 17 / 0 | Queue-flush confirmation, stale-input drain proposal |

Each additional binding requires the matching exact descriptor name, category, **full set of original event+priority pairs**, and an already bound tick callback. No new manager; all callbacks participate in module enable/disable and exception rollback. The shared metadata's event slot 3 is used by more than one candidate callback type; R332 conservatively REJECTS combined typed packet/frame requests without evidence that both represent the same original event. There is no host/JNI memory ownership or packet object transport in this change.

The default Inv Walk mode passes inventory packets until the module's silent setting has been explicitly configured by a tick. A GUI click is represented only as an enum + bounded count; the resulting open/send/replay proposals must NEVER be used as actual network actions without a client-thread packet owner and guaranteed flush or drop cleanup. Timer Range packet holding likewise depends on its separately provided frame simulation and is still an offline proposal.

CTest `r332_source_packet_frame_flush_bridge` checks incoming/outgoing delay semantics, entity ownership, freeze-before packet hold and disable, silent inventory queue/flush and re-enable, mismatched event refusal, and stop gating. R329 source policy tests and R330 tick integration tests remain.

This is **offline behavioural coverage only**. Still missing for the four models: original GL draw for Backtrack/Lag Range, Lag Range attack callback, Inv Walk/Timer Range toggle notification callbacks, exact production packet queue ownership, real JNI and Minecraft 1.8.9 execution. The original candidate reference is not independently authenticated as 1:1 commercial Slinky. Both PRs remain draft, no originals committed.
