# R290 — SHA-pinned R289 local Windows real-world probe

GitHub ForgeGradle 2.1 / JDK8 compiled the exact R289 source commit
20a3266ffe176502b6b26b26d05f58a06906b0b1.
Artifact ZIP and the internal forge-preview.sha256 manifest agree:

Preview JAR SHA-256: 37ed0d9ec4319ae7ffeefce19787394d6de1d75214c27f063b85c461ea62a807

The source-controlled R207 isolated launcher now accepts the
single R289RosterWorldTest switch. It refuses all conflicting
acceptance modes and mismatched JAR hashes. A fresh, direct
child of TEMP named slinkywinky-r278-independent-forge-<unique>
and dummy username R278Offline are required. The original Minecraft
libraries/assets and normal saves are never modified.

R289 enables the R278 read-only roster, the R283 Java8/JNI/native
AntiBot decision preview, and test-only creation of one isolated
singleplayer world. It installs NO Javaagent or gameplay hooks.
The preflight records the staged JAR, exact native DLL SHA and flags.

Launcher success is only staging/starting the expected test process.
An independently verified local world save and 32 real JNI receipts
with correct shutdown are still necessary for runtime acceptance.
