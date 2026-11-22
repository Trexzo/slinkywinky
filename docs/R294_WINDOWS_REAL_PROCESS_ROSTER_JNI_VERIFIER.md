# R294 — process-identity-gated Windows in-world JNI receipt

R291 verifies the exact staged Forge JAR, first-party native DLL
SHA256, isolated TEMP gameDir, test-only world level.dat,
dummy R278Offline profile and 32 real native Anti Bot receipt
markers. It does not, by itself, prove the test JVM was running
at the point when those files were inspected.

R294 adds tools/verify_r294_roster_windows.ps1 with required
InstanceRoot, Java8Home, NativeJniDir, PythonExe and TestProcessId.
The verifier refuses a non-TEMP or nested instance and requires
the precise already running Java8 executable, exact owned JVM PID,
LaunchWrapper main, dummy identity, read-only R289/R278/R283 flags,
and no Javaagent/action trial. It runs the R291 Python verifier.
After verification it rechecks the original JVM creation timestamp
and full command line to prevent accepting a recycled PID.

This script is read-only; it does not start, stop or modify Minecraft.
R294 CI parses its PowerShell 5.1 source on Windows and checks required
contract assertions, not the existence of an active real-world game.
Actual R294 success still requires a genuine local test JVM and log.
