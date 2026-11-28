# R368 — Exact original 1.8.9 active NetworkManager identity

Privately recovered original JNI tables (second 1.8.9 variant) map:
- `Minecraft.getNetHandler()` to `ave.u()Lbcy;`
- `NetHandlerPlayClient` to `bcy`
- `NetHandlerPlayClient.netManager` to `bcy.c:Lek;`
- `NetworkManager` to the separately SHA-pinned `ek` class.

R368 extends the official 1.8.9 SHA-1 verified Mojang client
read-only classfile audit with these exact method/field owners
and descriptors. Synthetic wrong-variant negative controls reject
untrusted mappings.

This mapping permits comparing the transient NetworkManager
argument of `ek.a(Lff;)V` with the network manager belonging
to the **actual current original Minecraft NetHandler**.
The intended runtime implementation uses only a short-lived,
client-thread value and a weak identity reference across threads;
it must not strongly retain a Channel/NetworkManager or allow
stale world connections to poison JNI e3 observations.

The static mapping audit is not original game runtime acceptance
and has no packet side effects or proprietary byte export.
