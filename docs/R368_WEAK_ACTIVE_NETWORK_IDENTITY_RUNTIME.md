# R368c — Live original NetworkManager identity-fenced packet observation

The original 1.8.9 JNI mappings `ave.u()Lbcy;` and
`bcy.c:Lek;` now permit packet metadata to be associated
with the **currently active game's** NetworkManager rather than
all instances of the original `ek` class.

The SHA-gated original packet diagnostic's client owner obtains
a transient original `ek` manager from `ObservedState`.
The original field/method reader `Vanilla189ActiveNetworkBinding`
stores only reflected Class/Method/Field metadata. A published
`WeakReference<Object>` to this manager is used by the foreign
`ek.a(Lff;)V` callback to compare exact object identity before
offering primitive packet samples. No strong manager/NetHandler/Channel
reference is retained by the packet observer.
A disconnected, changed or stale manager leads to immediate
primitive queue retirement/reset on the next admitted game tick.
Old-manager sends are ignored, never used to compute new-world e3.

This reduces a material gap in R366: packets from any other
original NetworkManager in the same classloader no longer count
as this world's native source input. As in R366, all native
event3 decisions remain hypothetical, delayed to the actual
client thread; original packet sends are never canceled,
retained, reordered or replayed.

R368's separate Mojang SHA-pinned original-JAR audit proves the
read-only method/field mappings and Java8 regression tests verify
weak exact manager identity changes and retirement. Full in-game
acceptance, true source callback phase and real packet hold
fidelity remain unverified.
