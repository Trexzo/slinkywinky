# R320 — deterministic presentation colors and numeric text

Base 66a89ce9ba48010f5d30d45172858a3f3af61e3d. The hover tint formerly rounded channel values beyond 255 then shifted and ORed them into packed RGB, causing carry into other channels and values beyond 24 bits. Clamp each channel before packing, retaining the previous 0–16 increment. The existing center-text helper now declines overflow of the computed position.

Numeric tooltip formats now use bounded C++17 floating-point to_chars with fixed precision, instead of locale-sensitive snprintf. The existing restricted format grammar, 0–3 decimals, literal percent and output limits remain. Nonprintable/non-ASCII format literals are rejected; current reference format literals are ASCII. Render snapshot primary captions explicitly use the classic locale.

Tests extend the existing control-motion fixture with 3,840 channel/opacity combinations and bright-color edge cases, numeric format fixture with rounding ties and malformed literals, and render-session fixture with a synthetic comma-decimal locale. These are independent presentation reliability corrections, not original commercial behavior or gameplay changes. No module coverage promotion. No local full build or test execution was performed; CI pending. Both PRs remain draft.
