# R242 follow-up — Linux Release Block In cross-compiler regression

The exact R241 pinned source-audit workflow failed 1 of 153 tests in
Ubuntu Release: `r231_block_in_source_target_and_look_at_geometry`
at its legacy `overridden.proposed_yaw_delta==0.0f` assertion.
The other 152 tests passed; the same suite passed on Ubuntu Debug
and Windows Debug/Release.

The R231 test already documented that libm/optimizer rounding may
produce sub-ULP residuals for a mathematically zero angle and elsewhere
uses `close(...,0)` with a `0.00004` tolerance. The overridden
legacy-input rejection subtest alone used exact float equality.

**Correction:** Retain strict `admitted`, `proposed_click`, and
both optional rotation values; compare the two proposed deltas with
the test's existing geometry tolerance. Production Block In
calculations and Clutch code are unchanged.

This does not classify the failing test as a proven false positive
until the final-commit Ubuntu Release CTest and other workflows pass.
