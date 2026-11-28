# R366a — Independent original NetworkManager packet diagnostic CLI

New exact options `--source-network-readonly[=600]` do not silently
change existing `--source-tick-readonly` or
`--source-click-readonly` sessions.

The new session requires explicit launcher environment
`SLINKYWINKY_EXPERIMENTAL_HOOK=1`,
`SLINKYWINKY_SOURCE_NATIVE_READONLY=1`,
`SLINKYWINKY_SOURCE_CLICK_READONLY=1` and independently
`SLINKYWINKY_SOURCE_PACKET_READONLY=1`. Extended 600-tick mode also
requires `SLINKYWINKY_EXTENDED_DIAGNOSTIC=1`. Unknown switches
and synthetic JVMs are rejected before agent installation.

The selected agent mode is `minecraft-network-source-readonly`.
Future runtime wiring additionally pins the official `ek.class`
fingerprint; it must not hook or accept unverified class bytes.
The new network observer will collect primitive read-only packet
metadata and compute **hypothetical** native event3 verdicts
exclusively on the Minecraft client owner, never change live packet
delivery. The reference Slinky packet timing remains unverified.

This CLI admission stage does not by itself install a packet hook.
