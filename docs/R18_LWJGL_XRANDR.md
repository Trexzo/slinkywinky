# R18 legacy LWJGL 2.9.4 XRandR display fix

Full R14 crash report was recovered. Minecraft 1.8.9 crashed before any SlinkyWinky initialization:

ExceptionInInitializerError caused by ArrayIndexOutOfBoundsException: 0
at org.lwjgl.opengl.LinuxDisplay.getAvailableDisplayModes(LinuxDisplay.java:951)

Xvfb did not offer the connected, enabled mode expected by legacy LWJGL/XRandR. This is a graphics-environment failure, not an observed combat-module exception.

CI now attempts a dummy Xorg device with an explicit 1280x720 mode. It captures xrandr and Mesa GLX diagnostics before launching the game. Failure remains visible; no fake pass is allowed.

Still pending: successful headless startup, native Windows Minecraft test, manually operated F9 and overlay acceptance.