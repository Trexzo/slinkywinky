# R293 — recover R290 PowerShell launcher from source baseline

The new R292 Windows PowerShell 5.1 ParseFile CI gate rejected R290.
The error was a malformed R289 TEMP prefix: a literal regex ending
in a dollar-sign plus quote had been interpreted as replacement
syntax during source editing. It corrupted the remaining branch.

R293 REBUILDS the launcher from the last verified pre-R290 source
at R289 commit 20a3266, then reapplies all R289 additions using
strict literal replacements. The R278 prefix and all older launcher
modes are now present exactly once with their correct quote/regex.

No executable is launched by this commit. The existing R292
actual Windows PowerShell 5.1 syntax test must PASS on the new
commit before using this test launcher against any real PC.
