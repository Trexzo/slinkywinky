# R335 — Real Java 8 ↔ JNI source-event reconstruction

R335 advances from C++-only offline callbacks to **actual Java 8, native DLL/SO and the same ModuleExecutionBridge** exercised by previous R329–R334 CTests. The `SourceRuntimePreview` Java facade calls an owned `SourceRuntimePreview` JNI session registered with the source-exact R331 Backtrack, Lag Range, Timer Range and Inv Walk event/priority metadata.

Nine source callback operations are exposed over copied primitive arrays: Backtrack tick/packet; Lag Range tick/packet; Timer Range tick/snapshot-derived frame; Inv Walk tick/packet/queue-flush. Every returned eight-value receipt is checked for expected event and executed callback before leaving Java. This path includes source prediction, source packet policies and existing inventory state, not an independently reimplemented Java model.

The JNI owner gate binds each handle to one thread; world and player are identified by explicit opaque tokens; clock/generation are monotonic; transitions retire the four module states before replay can occur in another world. Unknown opcodes, wrong array dimensions, nonfinite inputs, invalid booleans/enums, stale ticks, wrong owner, closed session and player/world identity changes without generation increments fail closed. No JNI Java refs or packet objects are retained.

The new `r335_java8_native_four_source_callbacks` CTest compiles Java 8 source, loads the actual `slinkywinky_jni` native shared library, and checks all four source controllers, Timer Range frame, packet state, inventory replay, time and generation isolation, input validation and close.

**This remains a read-only diagnostic bridge.** It does not inject into Minecraft, intercept real game packets, run GL commands, mutate timer speed, or press movement keys. Java module toggles and original in-world event timing are not yet validated. PR #1 unchanged; PR #2 draft/unmerged. No private original binaries or decryption artifacts committed.
