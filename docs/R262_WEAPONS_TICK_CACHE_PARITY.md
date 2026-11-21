# R262 — Weapons per-player-tick cache source parity

Recovered source: the user-supplied verified reconstruction archive's
WeaponsModule.java, SHA256
d3b8e218386d9a3d1c4a8aa4aeee569f599d451e5f5ebfcb23a804f0685ec115.

The nested Java Cache starts at {key=0,groups=0,allowed=false} and
updates only when player.ticksExisted changes. Therefore a query on
initial tick 0 returns its initial false value even with an allowed
weapon. A same-tick change in slot, enchantments or settings does NOT
refresh the cached classification. The classification refreshes at
the next player tick.

The R262 C++ WeaponsTickCache uses the already source-derived
WeaponsController::allowed_current and preserves this exact timing.
It additionally binds world and player session identities and refuses
regressed ticks. This identity fence is an explicit safety divergence
from the recovered Java class, which has only the tick counter.

No JNI or client hook; no Minecraft player pointer, item transaction
or hotbar mutation. The CTest regression checks tick0, first tick,
same-tick item/settings change, next-tick classification, new owner,
tick regression and malformed inventory/settings evidence.

R263 should replace R261's direct classification with this source
cache, but preserve the original Auto Weapon control-flow order:
isWeapon() is queried only when target and input conditions pass.
