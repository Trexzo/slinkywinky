# R154 — recovered expander text and icon geometry

The candidate decrypted ClickGUI template implements an .exp flex row with text first, 8px gap, and then a 20px plus/minus icon. Its click handler recreates the card immediately; the candidate does not define a card-height transition. Therefore R154 does NOT introduce an invented expansion animation.

The native standalone preview now draws its expander icon after the description or compact module title, and a single renderer-neutral geometry owns both rendering and input hit testing. Full text + icon area, not merely the icon, admits expansion. Compact key chips reserve the expander icon space. The source-derived input is strictly scoped to recovered-setting cards; popup, modal and clipped-card protections remain in place.

The text-width measurement is an explicit approximation using an independent portable advance model, not original font bytes or a pixel-perfect reconstruction. No gameplay actions, runtime injection changes, or proprietary font files are included. Tests check source layout constraints and whole-expander pointer ownership.
