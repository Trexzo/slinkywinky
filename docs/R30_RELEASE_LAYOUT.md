# R30: clean standalone loader release layout

The Windows x64 native attachment artifact now contains only:

- `slinkywinky_loader.exe`
- `slinkywinky_agent.dll`
- `attach-classes/dev/slinkywinky/attach/AttachMain.class`
- `SHA256SUMS.txt`

Test-only `AttachTarget.class` is built into the separate `attach-test-classes` directory for local/CI regression testing, but is excluded from the GitHub distributable artifact. The native agent has no Forge runtime dependency.

The `--tick-probe` option remains explicitly **experimental and unsupported on the tested live Java 8 JVM** (JVMTI AddCapabilities returned 98). Standard `attach` still succeeds. Neither mode installs a functional combat client or GUI.