# R152 — candidate ClickGUI flex-row vertical geometry

Reference: supplied decrypted candidate (decrypted files/re/clickgui.tpl.html); NOT authenticated signed commercial source.

The candidate CSS defines .in padding 8px, flex-direction column, 12px gap, .row height20, .sec height19 and font-size15. An expanded card therefore places the first setting control 35px below its collapsed header body (regular 68px, compact 36px); subsequent controls in one group have 32px pitch and every group transition adds 31px. Each title is 27px above the first control in its section.

R152 replaces the 27px source-widget pitch and 20px group offsets with a single deterministic layout model that is reused by the renderer, hit test, dropdown popups, sliders, swatches and text inputs. The five-widget page footer is a first-party fixture extension (not part of candidate HTML) and receives its own 32px spacing. Removes an out-of-source per-card text banner that overlapped recovered widgets. Slider paint and pointer-track geometry use the same right-2px inset. The independent source-widget inventory and experimental gameplay state remain unchanged.

Checks: model math for compact/regular cards, group boundaries, source-hit ownership and CTest; CI status on exact PR head must be checked independently. Still no pixel-perfect proof from signed client, and no completed gameplay modules.
