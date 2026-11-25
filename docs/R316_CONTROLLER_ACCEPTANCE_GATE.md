# R316 — controller acceptance evidence

R315 baseline 9a0e71c7bee085dbcd647b25c22406a1a522d80b. The static evidence audit maps all 50 current controller headers to 54 distinct C++ test source files referenced by executable targets registered with CTest, across the root and reference_compat CMake projects. Grouped module fixtures have explicit associations. This is source/test registration evidence, not independent proof that all assertions execute or exhaustively cover behavior.

The root CMake test-enabled build now creates one separate translation unit per controller header. This exposes missing transitive includes and standalone compilation problems independently of the order of includes in combined test files. It does not link the runtime or change module behavior. The core workflow also executes the new evidence audit and six synthetic positive/negative tests.

R315 archive identities were independently rechecked for all 11 historical gap files; the original 66-entry target and 171 subscription count remain unchanged. No original module has been promoted to complete. Source coverage remains 50 partial offline controllers, six experimental models, ten missing. This batch strengthens the evidence gate, not gameplay implementation. Local execution of the new gate and standalone build has not yet been performed. CI must verify them. PR #1 unchanged.
