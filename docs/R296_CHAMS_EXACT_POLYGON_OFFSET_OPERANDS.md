# R296 — exact Chams polygon offset immutable operand recovery

Source bundle: `Slinky-Verified-SourceBuild.zip`.
Java resolver `dev.slinky.client.render.RenderConstants` reads
little-endian spans from `assets/slinkysemantic/RenderConstants.json`.

SHA256 of that source JSON:
`3e6a7dba6433f6416ef73ae3193e04a09dd2e9275b0b4890f6432f743f71301e`.

Recovered original ChamsModule.java calls:

- `graphics.enable(32823)` (GL_POLYGON_OFFSET_FILL)
- `graphics.offset(RenderConstants.f(1392420), RenderConstants.f(1392424))`
- `graphics.disable(32823)` in render-finally after selecting entity

The JSON span beginning at byte address `1390288` includes:

| Address | Raw little-endian bytes | Float32 |
|---|---|---|
| 1392420 (`0x153f24`) | `00 00 80 3f` | `1.0` |
| 1392424 (`0x153f28`) | `00 1b 37 ca` | `-3000000.0` |

R296 exposes these as `kSourcePolygonFactor` and
`kSourcePolygonUnits` and adds `begin_source` / `render_source`
paths. Tests check exact IEEE754 32-bit bit patterns, call ordering
and the inherited source relationship mask. No GL state is mutated;
the model uses a fake Graphics port. Full live GL rendering, default
state preservation and renderer lifecycle remain unverified.
