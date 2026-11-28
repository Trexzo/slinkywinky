# R369b — Original combat-packet types in the real passive e3 callback

`Vanilla189OutboundPacketSampler` now supports strictly verified
original classloader-compatible `ff`, `ip`, `in`, `is`,
and `is$a` classes, with the C0B enum value stored in exact
`is.b:Lis$a;`. Existing 2-class C03-only constructor remains
available for older standalone regressions.

Parity follows recovered original source `lag_range.cpp`:
- C03 movement `ip`: category 1, position only when `ip.g` true
- C02 use entity `in`: source attack-release category 2;
  not proof the attack landed
- C0B action `is`: only enum ordinal 4 is stop-sprint category 3,
  all other actions remain category 0 (UNKNOWN)
- Everything else: category 0

No game packet or enum object is retained after sampling.
Original Netty/NetworkManager calls complete through vanilla code.
Only primitives enter the 256-row original sender queue.
On the admitted client tick, categories 1/2/3 replay through the
existing read-only C++ Lag Range event3 telemetry, and hypothetical
hold/release results are NEVER applied to live packets.

Strict original-JAR and synthetic Java 8 JNI regression gates are
required. Neither a read-only event3 proposal nor source attack
classification proves the original's attack callback timing,
a packet was held, or a full 1:1 original-client reconstruction.
