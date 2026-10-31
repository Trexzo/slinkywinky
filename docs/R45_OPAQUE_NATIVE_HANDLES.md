# R45 — opaque native observation-session handles

**Offline-only:** no Minecraft process is launched, attached to or modified by this milestone.

The R43 observation JNI interface previously returned a raw `SWContext*` encoded as a Java `long`. An accidental stale/forged handle could then be dereferenced after closure, causing a native crash rather than a controlled rejection.

R45 replaces that raw observation handle with a monotonically increasing, nonzero integer token backed by a **bounded four-context registry**. All observation and close operations acquire a short native mutex during lookup and decision evaluation to prevent concurrent close/use-after-free; the existing native owner-thread restriction remains enforced. Invalid, stale and forged tokens return failure without dereferencing them. A failed wrong-thread close retains the context for its creator thread to release. Tokens are never recycled during the process lifetime.

The new disposable Java 8 JNI regression checks wrong-thread observation, wrong-thread close, recovery on the rightful owner, double close, stale-handle use, forged handles and re-opened contexts, in addition to all earlier malformed-frame and field-mapping regressions. This does not enable a gameplay action, inspect Minecraft, or establish original Slinky fidelity.

The existing read-only callback's `nativeOpenObservation`, `nativeObserveFrame` and `nativeCloseObservation` Java signature stays unchanged.
