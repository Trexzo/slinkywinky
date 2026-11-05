# R50 — overlapping native attachment admission is fail-closed

The instrumentation agent previously called `ObservedState.admit(target)` before attempting to claim exclusive ownership with `hookInstalled.compareAndSet(false, true)`. A second attach could therefore reset world/player observation state while the first hook was still running, even though its own request was subsequently rejected.

R50 claims hook ownership **before JNI loading or any observation-state mutation** and releases that claim on a preflight failure before a transformer is installed. If a retransformation begins but fails, the agent locks out further attachment rather than guessing whether the JVM is still carrying modified method bytecode.

A deterministic disposable-Java negative control simulates the JVM Instrumentation interface, marks a hook as active and places sentinel values into observation state. It invokes a duplicate attach and asserts that it fails immediately, without loading a DLL, resetting world/session state or changing ownership. A second test supplies no JNI path and asserts preflight rejection releases the unused claim. It runs with the normal offline Java 8 attach test suite; no Minecraft runtime is needed.

The successfully verified R48 vanilla hook path is otherwise unchanged. The fresh R50 agent requires its own real-game acceptance before any release claim.
