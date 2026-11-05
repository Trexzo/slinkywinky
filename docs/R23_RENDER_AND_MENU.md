# R23: main-menu lifecycle and in-game render callback acceptance

Main-menu render frames used to call GameHost.transition every frame, generating duplicate native disables and synthetic release notifications. This revision calls transition only when leaving an active world/player, while preserving release on actual disconnect.

GameHost.tick(null) now fails closed before throwing. HostSmoke verifies no redundant menu releases and reliable null-frame release after an active session.

Experimental ForgePreview has an optional `-Dslinkywinky.renderProbe=true` flag. When enabled, it logs `RENDER_EVENT_PROBE_PASS` after 30 real Forge RenderTickEvent END callbacks, without requiring F9 or issuing any Minecraft actions. It is off by default.

Use alongside the separately opt-in `-Dslinkywinky.startupProbe=true` to verify both JNI ABI and render callback registration on a Windows Minecraft 1.8.9 development client.

These checks do not establish F9 manual interaction, production action dispatch, network codecs, original client equivalence, or gameplay fidelity.