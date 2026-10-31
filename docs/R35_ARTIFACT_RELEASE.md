# R35: safe distribution of experimental injector

The standalone loader package contains only `slinkywinky_loader.exe`, `slinkywinky_agent.dll`, `slinkywinky_instrument.jar`, its Java 8 attachment class, and `SHA256SUMS.txt`. The manifest now uses paths relative to the ZIP root, so it can be verified after extraction. CI recomputes and verifies every entry.

The Forge-independent `--hook-probe` is **unverified in a real Minecraft instance after the R33 fix**, so it requires both an explicit CLI option and `SLINKYWINKY_EXPERIMENTAL_HOOK=1`. A negative regression ensures it is rejected without this additional gate. The passive `--instrument-probe` and proven native attach functionality remain available.

No combat module, packet mutation, input modification, or stealth mechanism is enabled by default. The PR remains a draft.