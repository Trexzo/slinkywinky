# R157 — first-party Sprint decision foundation

Catalog entry 25 (Sprint) now maps to a typed native profile field, and its four recovered GUI Booleans are connected by exact module/group/label/source-kind identity: In inventory, Using item, Backwards and Sideways. Settings serialize and parse through the existing strict profile format and retain old version 1 compatibility.

An original C++ pure function evaluates Sprint intent from a validated world observation: reject absent world, insufficient hunger, sneaking, collision, blindness, invalid/nonfinite movement and disallowed GUI/item-use conditions. Backward and lateral movement are allowed only when their corresponding config flags are set. It never writes an input key, client motion or packet. This is a proposed first-party behavior model, not proven original Slinky logic. Backward/sideways sprint especially would require a separately audited live-client movement implementation.

This does NOT complete Sprint in Minecraft, change the public native C ABI, or expand permission to synthesize game input. The loader/JNI agent and the 59 other currently unsupported catalog module toggles remain unchanged. Native model testing is not game-world acceptance.
