# R46 — offline native session registry contention and capacity test

No Minecraft launch or attachment is performed. The disposable Java 8 JNI regression now creates four simultaneous native observation contexts and verifies the fifth is rejected by the bounded registry. It then performs 2,000 wrong-thread observation calls concurrently across four Java threads while the rightful owner performs 500 valid calls.

Expected results: **all foreign calls fail**, all owner calls succeed, no thread hangs, every owner context closes correctly, and every released token remains invalid. These tests run as part of the existing Windows CTest native-agent suite and do not modify any game process or file.

The instrumentation hook remains developer-gated and read-only. No claim is made about real Minecraft runtime acceptance of R44–R46.
