# R272 — Piercing original event8 ordering model (offline)

Recovered from user-supplied reconstructed PiercingModule.java.
The module is combat category, event8 queryOverride=true.
Its rank() compares two raycast candidates with relationship flags
and a cached Weapons.isWeapon result:

- A disabled module, insufficient inside-bounding-box, or missing
  required weapon => source ranking 2 (ignore)
- A non-player target => ranking 1 if allowed, else 2
- Team/friend flags 1|4 and Anti Bot flag 2 are independently tested
  when their settings are enabled; selected matches => ranking 1
- A remote player after a non-player source => ranking 0
- Remote-player/remote-player comparisons require enemies enabled;
  sort_by 1 compares health, sort_by 0 compares hurtTime; ties use
  two stages of Java 32-bit mix with constant 73244475 and signed
  int ordering on ticksExisted+lastAttackerTime.

Float NaN health is treated as 20.0 in the recovered Java source.
The C++ code avoids undefined signed overflow, and rejects infinite
health and malformed sort modes as a deliberate offline safety gate.

The new regression covers query override separation, block eligibility,
weapon/inside gates, non-player prioritization, friend/team/bot
relationships, health/hurt ranking, NaN, Java signed hashing and wrap.

Not reconstructed: actual Minecraft ray traversal, first/second entity
ownership, original event priority, rendered collision boxes or
end-to-end hit selection. No gameplay or packet mutation is made.
