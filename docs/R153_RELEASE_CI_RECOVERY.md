# R153 — restore Windows Release regression coverage

R152's source-derived ClickGUI vertical layout changed the expected Auto Clicker expanded heights to 223px and 222px and the numeric track hitbox height to 20px. The old fixed expected values remained in three legacy tests, causing two independent GitHub workflows to fail even though native rendering and JNI roundtrips built.

The compact overlay's clipped module row now extends one pixel past the safe bottom edge. The gesture test must assert this row remains noninteractive rather than attempting a click on a clipped card.

The old Windows Release checks aborted without useful diagnostics on an uncaught test exception. The two affected tests now emit the failing expression and exit with code 1. All source/input restrictions are unchanged; this is test-only cleanup and does not activate or reconstruct gameplay modules.

Evidence: Windows Release local CTest repro on the connected desktop, repaired 3/3 passing. Full CI on the exact new SHA must be reviewed separately.
