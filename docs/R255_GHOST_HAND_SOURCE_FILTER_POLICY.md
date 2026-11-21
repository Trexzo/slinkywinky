# R255 — independently reconstructed Ghost Hand predicate

Source: user-owned Slinky-Verified-SourceBuild archive
\`dev/slinky/module/impl/GhostHandModule.java\`, cross-checked with
original decrypted native \`mod_ghost_hand.cpp\` bool registrations.
Original native callback priority remains unverified.

R255 introduces a pure, stateless C++ Ghost Hand policy with
separate event8 query-override and target/item interaction gates.
The reconstructed Java event8 callback sets \`queryOverride=true\`
unconditionally for an enabled module; this is intentionally NOT
treated as proof that every target can be interacted through.

Source-derived item groups:
- Swords 267, 268, 272, 276, 283
- Tools 256-258, 269-271, 273-275, 277-279, 284-286, 359
- Buckets 325-327; flint and steel 259; cobweb 30
- No held item => fists; all other nonnegative IDs => other

Remote-player relationships select bots for flag bit2,
friendlies for bit1 or bit4, else enemies; non-player is a
separate setting. Bot wins when both bot/friendly bits are set.

The regression tests cover all item gates, friend/bot priority,
mouse-press condition, ready/disabled gates, fist versus unknown
item, and the independent event8 override. C++ source is entirely
first-party and does not include or compile decrypted sources.

This is NOT a working Minecraft Ghost Hand: no line-of-sight ray,
entity relationships, JNI runtime hook, interactions, packets or
anti-cheat evasion has been integrated.
