# R292 — Windows PowerShell 5.1 R289 launcher parse CI

R290 added a SHA-pinned, isolated Windows test-world mode to the
existing R207 independent Minecraft 1.8.9 loader. R291 then
hardened the provenance verifier against fabricated log-only proof.

R292 extends the existing Windows JNI workflow with an actual
Windows PowerShell 5.1 language Parser.ParseFile check. The job
refuses syntax errors and missing required R289 hash/flags/dummy
identity and native SHA evidence fields before compiling JNI.

The check performs no launch, file extraction or data mutation:
parsing the source script does not execute it. It cannot prove
that the source-controlled Windows launcher actually starts a
real TEMP singleplayer world or receives a valid JNI event.
That still requires isolated in-world acceptance on a Windows PC.
