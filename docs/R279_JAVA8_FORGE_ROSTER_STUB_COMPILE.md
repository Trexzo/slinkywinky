# R279 — Java 8 compile test for R277 Forge roster Port

Compiles the unchanged ForgeRelationshipRosterPort.java against
minimal, disposable Java 8 type stubs with the signatures it uses
from Minecraft 1.8.9. The executable smoke tests owned local world,
ordered same-name UUIDs, movement/armor/invisibility field capture,
world-change token renewal and rejection of public multiplayer or
server-side world inputs.

This is only a stubbed API/compiler check: it is NOT the real
ForgeGradle 1.8.9 build, a real Minecraft run, or proof of runtime
source parity. R278 remains disabled by default and gated behind the
existing isolated TEMP offline Forge admission.
