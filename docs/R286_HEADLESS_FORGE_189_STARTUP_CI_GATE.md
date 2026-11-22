# R286 — one-time GitHub headless Forge 1.8.9 launch gate

Previous R278/R283/R285 checks build ForgeGradle 2.1 preview JARs
with the authentic Minecraft 1.8.9 dependency, but no code-driven
startup acceptance had been triggered on the current PR.

The pre-existing .github/workflows/forge-headless.yml launches
ForgeGradle runClient on a disposable GitHub Ubuntu runner with
software rendering and a dummy Xorg display. R286 adds a one-time
pull_request trigger restricted to editing THIS workflow. Normal
source changes do not repeatedly launch the expensive game process.

SUCCESS, if observed in CI, requires:
- Java 8 and Gradle 2.14.1 resolve and start actual Forge runClient;
- headless software GL initializes an Xorg 1280x720 display;
- game log includes the actual ForgePreview initialized-read-only
  marker, and the controlled 360-second timeout is reached.

It does NOT launch a singleplayer world, press F9, load the JNI
roster observer, verify an original Slinky event, demonstrate a
successful R283 decision receipt, or write to the user's machine.
Failure should be investigated from the preserved GitHub logs
without relaxing the real startup acceptance to a build-only PASS.

R286 is a narrower game **startup** acceptance gate on the road
to the still-unverified in-world R283 receipt.
