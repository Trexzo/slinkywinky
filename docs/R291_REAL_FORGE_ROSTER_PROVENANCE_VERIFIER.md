# R291 — independent runtime provenance required for R289 acceptance

R289 initially offered --log, which tested text syntax but could be
mistaken for evidence of a live game. R291 removes that shortcut.

The real acceptance route is --root <fresh isolated TEMP gameDir>
--native-jni <first-party slinkywinky_jni.dll>. It requires:

- exact direct child of the canonical TEMP root with R278 prefix
- source SHA-pinned R289 Forge JAR in mods, not a different JAR
- staged path equal to the R207 independent launcher preflight
- matching vanilla SHA, required dummy identity and all R289 flags
- no mixed R208/R209/R212/R214/R224 modes
- actual R289-local-only/level.dat in the isolated save folder
- supplied native DLL whose SHA agrees with the preflight record
- genuine Forge mod-loader metadata, 32 ordered JNI receipts and
  owner/preview cleanup with no game writes by the roster module

R291 CTest selftest uses synthetic preflight/log strings only to
verify fail-closed behavior; it cannot establish real game acceptance.
Even a passing real in-world receipt remains read-only module evidence
rather than 1:1 original Slinky gameplay, GUI or injector parity.
