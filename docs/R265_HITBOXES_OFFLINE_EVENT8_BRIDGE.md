# R265 — Hitboxes alongside Reach and Ghost Hand on event8

The reconstructed HitboxesModule.java registers category COMBAT,
event8, sets event.enabled=true and event.expand=number("expand").
The decrypted native registration has a slider but no authenticated
callback priority. The new offline fixture uses provided event
priorities solely for testing (Hitboxes=50, Reach=40,
Ghost Hand=0); **this is not claimed as the original order**.

R265 integrates the R264 C++ HitboxesController with the existing
ModuleExecutionBridge. The typed event8 frame can now carry any
one-, two- or three-module combination of Hitboxes, Reach and
Ghost Hand, without permitting unrelated typed event payloads.
Only an enabled/registered Hitboxes callback may propose the
event expansion; a caller cannot forge the enabled state.
Disabled modules produce no event proposal. Invalid/nonfinite
settings return a refused source proposal rather than a
nonfinite collision expansion.

CTest covers triple delivery, pair subsets, priority fixture order,
untrusted flags, wrong event, invalid slider, disabled callback,
malformed registration and stopped executor.

No real hitbox expansion, ray modification, native event8 hook,
player targeting or network output has been introduced.
