# R153 — source-grounded module expander hover

Candidate GUI HTML sets .exp:hover .pm background to #505050, its plus/minus strokes to white, and description text from #999999 to #BBBBBB. Compact mode changes the hovered module name to white. The candidate file is a forensic recreation, not authenticated signed source.

R153 adds a source-indexed, visual-only expander hover field to the immutable GL11 frame. Win32 pointer motion admits it only through the existing fully visible `MenuHitKind::Expand` hit test. Popups and modal surfaces clear the hover; invalid catalog indices are rejected. It cannot modify the module configuration, input or game runtime. Existing Boolean hover and dropdown fade remain independent.

The native preview's plus hitbox still covers only the 20px square, not the full source DOM text element; expansion height tween and exact running-client parity remain to be implemented and verified. Regression asserts index bounds, retargeting, popup ownership and pure visual state.
