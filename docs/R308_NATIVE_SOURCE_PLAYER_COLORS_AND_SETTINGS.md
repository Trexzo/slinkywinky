# R308 — source-faithful Player ESP style, color, and draw setting routing

## Verified source evidence

Read-only inspected `OpenSlinky-main(2).zip` archived Java 8
`PlayerEspModule.java` SHA-256
`8b99ecdb976b33b6195301d149df455dcfbbbdefc8b62a0415fe33eea0179e1b`,
`RenderAccess.java` SHA-256
`b1a4b1e7259058a23b4d192a9283a48fe4c3d479694e51fa63fc14d112f14d1b`,
`RenderMath.java` SHA-256
`1ad5e5720153375dda6a5235f84b67349e5fe67c30387542e4f00e84cc073f50`,
`RelationshipService.java` SHA-256
`046ae8f97b8bdc36eb47603e708e3f68b65ddcd245bd82d7296892a9d401e5bf`,
and recovered `MetadataCatalog.json` SHA-256
`c23fc4aae72e26d05caaf71b4b5b6902a8e15314e92b375568153cb460c17853`.

Original event subscriptions: 18 (collect), 6 (world 3D, priority -300),
7 (screen 2D, priority 0). Registration values are from the candidate
recovered archive, **not proof of original runtime scheduling**.

Source PlayerESP selection:
1. Exclude self, dead, and relationship flags masked by
   `(hide_friendlies ? 5 : 0) + 2`. Thus flag 2 is excluded even
   when hide_friendlies=false; flags 1 and 4 are additionally excluded
   when true.
2. Select **target color** only if override_target_color is enabled
   AND current entity matches the authoritative target.
3. Otherwise, if color_mode=0, select **name/team color** from the
   player's ScorePlayerTeam prefix (last valid `§0..§f` code, or
   white when unavailable). In original Forge, fontRenderer.getColorCode
   determines the final RGB and channels are divided by 255.
4. Otherwise (color_mode=1), choose **friend** RGB when flags&5 != 0,
   else **enemy** RGB.
5. Collect at most 100 rows before farthest-first source sort.
6. Draw a 2D box iff `show_2d_box || row.target`; add a
   filled overlay iff `filled`, with recovered alpha `0.25`;
   draw a health bar iff `show_health_bar`, regardless of box.
   Hurt effect is `lerp(source_rgb,(1,0,0),min(10,hurt-partial)*.1)`
   only when enabled and hurtTime>0.

The recovered metadata records **observed saved/current values**, but
does not establish the real factory defaults. Thus no automatic
factory-profile was introduced. The observed editable line_width
range is 1..4; examples and tests must supply explicit values.

## Implementation

- The existing R299 C++ PlayerEspController now supports **opt-in**
  `source_style`, per-player float RGB, explicit target/relationship
  flags, and a selected float RGB on each native *sorted* row.
  Non-style R299/R301-R307 behavior remains unchanged.
- `nativeStageScreenStyle` is a separate opt-in JNI transaction
  **after exact-owner R300 observe, before event0**. It validates a
  complete per-player encounter-order metadata vector, explicit
  8-option settings and color channels, plus 3 source palette colors.
  It refuses duplicate/late/foreign/invalid entries, and does not
  silently invent target, friend or name-color provenance.
- `nativeScreenStyleRows` is limited to the same owner thread,
  exact source world/player/generation/tick, the already completed
  source **event7**, and a previously staged style input. It reports
  selected RGB and target on the same farthest-first native rows
  as R306/R307 source `ScreenPlan`.
- Java 8 `SourceStyleSettings`, `SourcePlayerStyle` (UUID-bound to
  the raw original encounter order), `stageScreenStyle`,
  `screenStylePlan`, and immutable `SourceStylePlan` transport
  source settings; the owner/session lifetime and tick fences still
  apply. Native source rows, not a second Java roster sort, determine
  the actual selected RGB ordering.
- `sourceDrawBox` and `sourceDrawColor` expose original boolean
  and float interpolation arithmetic for independent regression
  testing. `ForgePlayerEspScreenRenderer.drawStyled` accepts an
  owner-matched native style plan and an equally sized screen projection,
  with source box/health/fill/hurt gates. It uses diagnostic GL11
  primitives, **not the original NanoVG canvas implementation**.
  The existing ForgePreview **does not call** this opt-in method
  yet because the required source relationship/target metadata is not
  available in the ordinary Forge sampler.

## R308 independent JNI fixture

A synthetic but fully specified five-player fixture passes real
Java8→JNI→C++ R299 code and checks:
- relationship 2 excluded while flags 1/4 retained in normal mode;
- source RGB color override takes priority over manual names;
- native sort returns x=6,x=4,x=2 with correct green/green/yellow;
- `show_2d_box=false` still draws target outline, not other boxes;
- `sourceDrawColor` correctly blends with red by hurtTime/partialTicks;
- next tick `hide_friendlies=true` excludes flags 1/2/4;
- source color_mode=0 chooses exact user-supplied name RGB,
  and overrideTarget=false disables source target override;
- source style reports showBox, healthBar, filled, hurtEffect,
  lineWidth exactly as explicitly supplied;
- duplicated style submissions, roster reorder, out-of-bounds line
  width, foreign Java threads, stale tick and event26 access
  are rejected.

## Remaining (not production-complete)

**Authoritative relationship flags and target identity are not yet
sampled from the original private game runtime.** Defaulting them to
zero in Forge would fabricate color and eligibility provenance.
R308 therefore keeps actual styled game rendering DISABLED until
R309 can add an authenticated relationship/target read-only port.

The recovered source is a *candidate* binary-lineage reconstruction;
official original-release authenticity, live Minecraft screenshots,
frame-time cost, exact canvas/font pixel parity, original production
event priorities and full 66-module client fidelity remain unproven.
Owner Windows desktop remains offline. PR #1 and #2 remain draft.
