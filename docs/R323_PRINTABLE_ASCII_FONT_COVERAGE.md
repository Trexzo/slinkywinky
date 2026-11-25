# R323 — printable ASCII fallback font and GDI input guards

Base a6f406d7059eefc7a07c1a8f1cf03579ade9f368. The first-party fallback bitmap font now includes 19 missing printable punctuation glyphs, completing the 95 printable ASCII characters (26 lowercase letters intentionally reuse uppercase geometry). Nonprintable and non-ASCII bytes retain the question-mark fallback. No commercial font binary, external font asset or original typeface equivalence is claimed.

The existing bitmap regression now checks every printable ASCII byte for nonempty, non-fallback glyphs (except space and question mark), all remaining byte values for fallback, all lowercase mappings and directional punctuation pairs. Windows system-font rasterization now validates unsigned GDI dimensions and coverage before narrowing/row indexing; draw rejects nonfinite coordinates and overflowing nominal bounds. No rendering changes to valid GDI samples intended.

This is first-party UI presentation coverage, not full Unicode, authentic font parity or an additional module. Local execution not performed; CI must verify. No gameplay, input hook or native loader behavior changes. PR remains draft.
