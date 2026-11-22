# R288 — LWJGL2 XRandR active-mode crash on dummy Xorg

The first real GitHub headless Minecraft 1.8.9 startup CI run
(45025062590, R287) failed BEFORE ForgePreview initialization.

The headless Xorg and OpenGL preflight passed, but Minecraft 1.8.9
LWJGL 2.9.4 raised `java.lang.ArrayIndexOutOfBoundsException: 0`
in `org.lwjgl.opengl.XRandR.findPrimary` during display init.
The underlying LWJGL2 parser matches xrandr modeline names against
`^(\\d+)x(\\d+)$`. The project's dummy Xorg file instead registered
`1280x720_60.00`, so no parseable active mode was available.

R288 renames the Xorg ModeLine and requested mode to `1280x720`
and changes the headless step to require a connected **primary**
output whose selected current mode is exactly `1280x720`.
The actual xrandr mode line must carry the active-mode `*`
marker; a mere screen header with 1280x720 is insufficient.

No binary patch, game source edit or weakening of the startup PASS:
the original workflow still demands the game's exact read-only mod
initialization marker and a controlled timeout. The next CI run
will determine whether this repairs the XRandR crash or surfaces
the next independent runtime blocker. The result is unverified
until that run completes.

External reference: original LWJGL2 XRandR.java populate() and
findPrimary() in github.com/LWJGL/lwjgl.
