# R284 — reject duplicate native roster receipts

R276's Java sampler may issue several receipts for the same client tick.
R281 previously accepted identical native tick observations and therefore
could accumulate the same (prevX-x)^2+(prevZ-z)^2 displacement repeatedly.

R284 requires a strictly increasing tick inside the same world/owner
generation. A new world or owner generation may start at tick zero.
Replayed or stale receipts are rejected before native event dispatch.

The C++ R281 regression and executable Java 8 JNI R282 regression now
test duplicate refusal. This is a bridge-specific safety gate, not a
claim that the original Slinky source subscribed only once per tick.
