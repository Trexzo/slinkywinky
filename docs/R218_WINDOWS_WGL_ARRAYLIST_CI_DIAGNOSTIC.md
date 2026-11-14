# R218 — Windows WGL ArrayList CI pixel evidence

## Scope

PR #2's integration CI continues to check out the GitHub-generated merge
against its evolving PR #1 base. The R217 head (1b854264) completed its pinned
OpenSlinky reference audit, but the Windows workflows failed in the
GUI-owned `isolated_wgl_menu_state_restore` test. The standalone loader job
also reports the `r209_retained_jni_arraylist_owner_and_pixel` assertion.
The Linux offline jobs passed.

This is **not proof of a native No Slow regression**. The failing tests and
their strict F3 5D 12 accent expectation come from PR #1's later R207–R209
renderer, which is absent from the PR #2 standalone head. The integration
branch does not edit its GUI-owned test or renderer.

## Read-only diagnostic

- Add `tools/probe_wgl_arraylist_ppm.py` to parse the already-supported
  disposable WGL fixture's P6 framebuffer export. It reports RGB values at
  GL bottom-left-origin coordinates `(666,612)`, `(666,590)`,
  `(662,612)` and `(4,628)`.
- Enable `SLINKYWINKY_R207_ARRAYLIST_PPM=wgl-arraylist-ci.ppm`
  for the normal CTest step in the existing Windows/Linux core workflow.
  CTest writes the relative filename inside its build directory; the
  reporter reads `build/wgl-arraylist-ci.ppm`.
- On failing Windows jobs, report sampled RGB via a diagnostic step.
  **The original CTest failure is preserved**. No tolerance adjustment,
  test skipping, retries that mask failures, production code change,
  screenshot of a real game, or user-machine capture is added.
- Include 3 small synthetic parser/coordinate rejection tests.

## Local source-exact evidence

Read-only PR #1 base worktree: `1550cd7e0f94903b3077523cb2ffb79fa0ca09fe`.
An isolated MSVC Debug build of the WGL test target completed successfully.

- `ctest -C Debug -R ^isolated_wgl_menu_state_restore$`: **1/1 PASS**.
- Disposable `676x630` P6 capture returned:
  - GL `(666,612)`: **F35D12**
  - GL `(666,590)`: **F35D12**
  - GL `(662,612)`: **0B0B0E**
  - GL `(4,628)`: **000000**
- `python -m unittest discover -s tests -p test_probe_wgl_arraylist_ppm.py -v`:
  **3/3 PASS**.

Thus the strict accent expectation works on the connected Windows machine
but was failing on GitHub's Windows runner. The runner's actual pixel values
and the cause (pixel format, rasterization, draw state, or another factor)
**remain unverified pending a new Actions run**. Do not loosen pixel checks
based on a guess.

PR #1's source and workflow remain untouched. PR #2 remains draft and
unmerged, and this diagnostic is not native gameplay or original commercial
Slinky equivalence.
