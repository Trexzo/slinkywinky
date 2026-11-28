# R370 — Strict real native event3 receipt ownership and category invariants

The original 1.8.9 passive NetworkManager diagnostic now validates
all four C++ Lag Range source packet decision fields, instead of
examining only the hypothetical packet-hold bit:

- `hold`: 0/1, cannot coexist with release or recorded movement
- `released_on_attack`: 0/1, only for source C02 kind 2
- `released_on_sprint`: 0/1, only for C0B ordinal 4 kind 3
- `position_recorded`: 0/1, only for source C03 movement kind 1
  with `moving=true`.

It also requires matching native world generation and client tick,
and rejects NaN, non-binary, mismatched and contradictory results.
Read-only counters separate observed original C02 / C0B actions
from **native C++ proposed releases**. A packet never gets canceled,
held, reordered, replayed or mutated based on these JNI receipts.

The R370 Java8 test exercises 14 positive/negative controls.
Existing R369 native C++ regression exercises real source event3
attack and sprint release transitions, with a non-stop C0B action
negative control.

An original Minecraft manual acceptance test and exact callback
time parity remain outstanding. Full Slinky reconstruction incomplete.
