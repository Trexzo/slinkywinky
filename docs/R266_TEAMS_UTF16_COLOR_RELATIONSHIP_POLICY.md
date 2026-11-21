# R266 — recovered Teams color and relationship decision (offline)

Source: independently reconstructed user-supplied
Slinky-Verified-SourceBuild.zip/TeamsModule.java, SHA-256
5f7c4fd36327e8de251d36e92a350f32f88626205e35ab30c61ac1761962ec09.
Decrypted native registration src/gen/mod_teams.cpp contains a bool
and items setting; it does not prove live game callback priority.

The value-only C++ model follows the Java char/UTF-16 formatting parser:
it recognizes the section sign U+00A7, lowercase 0-9/a-f/r codes,
uses first player-name prefix color or plain display-name suffix,
and prefers scoreboard team prefix (fallback 'r') over other
player-info display overrides. It uses the source-derived 0-15
color-index arithmetic for name-selected team colors.

Source event0 matches a remote player's formatting color either to
the current owner (when match_self_color is selected) or to one of
the 16 configured colors. A resulting flag-1 relationship request is
a value-only proposal; disable/unload return a clear-mask of 1.

Tests verify exact UTF-16 handling, malformed/uppercase formatting,
prefix/suffix fallback, scoreboard precedence, color index arithmetic,
explicit name colors, match-self behavior and lifecycle clear bits.

Nothing in this milestone modifies RelationshipService, Minecraft
entities, chat, scoreboards or packets. Future event-0 integration
must bind identity/generation and test actual host ordering.
