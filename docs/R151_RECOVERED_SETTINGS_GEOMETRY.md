# R151 — recovered ClickGUI settings geometry

Reference: the user's candidate decrypted GUI file (decrypted files/re/clickgui.tpl.html). The HTML is a forensic reconstruction, not independently authenticated original signed Slinky source.

The fixture CSS has .in padding 8px; .row height 20px; .half-r left:50% right:0; switches 40x20 with knob at x+6 or x+26; inactive hover background #505050; text inputs height 20px and 15px font; color swatches 40x20 rounded to radius 10. The native preview previously had 54/46 right controls, 34x18 switches and several 22px controls.

R151 adds a renderer-neutral source-aligned control column; updates the GL renderer, source-indexed hit testing, popup, text, numeric and color geometry together; and adds a deterministic CSS regression. This is GUI-only and adds no gameplay actions, native font assets or game hooks. Actual original-client screenshot parity and implementation of all 67 modules remain unfinished.
