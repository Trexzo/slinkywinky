# R365b — Strict original 1.8.9 C03PacketPlayer source fields

The recovered private C++ JNI table for 1.8.9 identifies
`C03PacketPlayer` as `ip`, `moving` as `g:Z`,
and movement coordinates as `a:D`, `b:D`, `c:D`.
This stage requires all five exact original 1.8.9 facts in
the SHA-1 and size-pinned Mojang JAR audit, in addition to
`NetworkManager=ek`, `Packet=ff` and `ek.a(Lff;)V`.

A future packet observer can now classify the original movement
subtype by strict classloader identity and only copy fields when
the packet has a position. No JNI reference or packet object
is retained, and this static source check never hooks a live JVM.
Original class bytes remain ephemeral and never enter Git.
