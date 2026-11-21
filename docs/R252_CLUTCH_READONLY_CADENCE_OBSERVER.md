# R252 — recovered Clutch click cadence, read-only Forge observer

Scope: PR #2 only. R251 is the fully passing build baseline. PR #1
remains independent.

The verified reconstructed Java source's event-0 Clutch callback advances
its float timer by -1 when positive, schedules the next slot using
20.0f / clicks-per-second, and only then checks game readiness, placement,
target coordinates and top-vs-side cursor face. A matching hit computes
the adjacent block. The independent C++ R247/R248 click gate has the same
fixture behavior; R252 ports its *value-only* timing into Java 8.

`ClutchCadenceObserver` is an owner-bound timer with monotonic
world, player, generation, tick, and exact target-bound R250 receipt.
It models the source even when there is no prior candidate or game focus.
It never calls mouse, blocks, rotation, packet or JNI action functions.

The new `-Dslinkywinky.r252ClutchCadenceReadOnlyProbe=true` setting
requires R251/R245/R237's independent TEMP-only offline opt-in.
For each observed Forge PlayerTickEvent.END, the integration advances
a 10 CPS **fixture** (not the live original Clutch setting), using the
previous native plan and one real cursor receipt when available.
Without a previous plan it still advances the timer, but cannot
produce a proposed press.

Markers:

- `R252_CLUTCH_CADENCE_INCONCLUSIVE_NO_PRESS` means zero
  would-press proposals in the 20-tick observation.
- `R252_CLUTCH_WOULD_PRESS_READONLY_NOT_PLACEMENT` means
  an actual native prior coordinate and cursor face agreed on a
  source-model timer-due tick. It is still **not a click**.

Tests include native C++ fixture parity at 10 and 12.5 CPS, no
planned target, no game readiness, wrong face, substituted target
receipts, stale owners, invalid CPS, duplicate ticks and foreign threads.

**Limits:** Forge PlayerTickEvent.END is *not* the original event-0
callback. The 10 CPS value is fixed for diagnostics. The original
native session does not yet own an event-0 input hook, and the
preview never executes a placement or validates a post-place state.
Only an isolated real Minecraft session can substantiate runtime
cursor/proposal receipt claims. No private recovered source,
binary or decrypted archive is committed.
