# R17 headless runtime diagnostics

The first R14 headless Forge run reached the actual Minecraft 1.8.9 client process but crashed before confirmed SlinkyWinky mod initialization.
Console-only crash details suggested an absent OpenGL context, which can be a downstream symptom, not necessarily the root exception.

This revision always uploads the full launcher console, Minecraft game logs, crash reports and JVM fatal-error logs (when present). It also prints the first 100 crash-report lines for immediate triage.

A failed headless run remains a failed test, not a successful launch. Core tests, JNI artifacts and Forge JAR compilation are independent successful gates.
