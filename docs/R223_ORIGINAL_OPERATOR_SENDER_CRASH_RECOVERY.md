# R223 — Original-client operator sender-crash recovery

Date: 2026-11-15. First-party original Minecraft 1.8.9 native menu
control plane; Chat 1 reconstruction branch, draft PR #1.

## Failure mode confirmed from source

R222 added a sticky session-uncertain event to guard timeouts. A further
problem remained: only the short-lived sender created/opened the Win32
producer mutex. If the sender exited while owning it and its only handle
closed, Windows could destroy the named object. The next sender then
created a *new* mutex and saw a clean WAIT_OBJECT_0 instead of the
abandoned ownership, allowing an old response to be misattributed.

## R223 first-party fix

- On original-game nativeOpenOperatorMenu, retain a non-owned slot-11
  mutex handle for the whole session; continue to fail on duplicate
  event/mutex names and tear it down with the other session handles.
- Sender requires the live receiver's slot-12 uncertainty event before
  opening a mutex. It then uses menu_operator_claim_mutex, which
  distinguishes acquired, busy, abandoned, and uncertain outcomes.
- WAIT_ABANDONED immediately sets the sticky uncertainty event while
  owning the mutex, then releases and refuses the command.
- The actual source-pinned JNI render owner already refuses polling
  when the uncertainty latch is set. Its Java callback follows normal
  restoration/teardown; no original keyboard/mouse hooks are enabled.
- Normal accepted/refused commands, stop, the GUI, renderer, and
  gameplay remain unchanged.

## Regression

The production JNI/WGL fixture now opens the actual receiver-owned
named mutex, verifies normal claim/release, then starts a separate
Windows thread that exits while holding it. The next claim must return
Abandoned, latch uncertainty and prevent another posted menu action
from being consumed or acknowledged. That verifies the Win32 kernel
abandonment path, not merely a synthetic SetEvent.

## Verified Windows acceptance

The exact R223 source commit `230197c` was fast-forwarded into the
existing isolated Windows build checkout, configured with MSVC x64 Release,
`SLINKYWINKY_BUILD_JNI=ON`, `SLINKYWINKY_BUILD_ATTACH=ON`,
`BUILD_TESTING=ON`, and Temurin Java 8. The native build succeeded,
and **107/107 CTests passed**, exit **0**, total CTest time **34.39 s**.

That includes the production JNI/WGL `isolated_wgl_menu_state_restore`
fixture with an actual abandoned Win32 mutex and the Java 8
`native_agent_attach` test (34.38 s). The fix was not exercised against
a live original Minecraft process; no actual physical input, game world,
or production injector session was launched. GitHub Actions CI for the
new head remains independently unverified.

Limits: Win32 named objects are not an adversarial authentication
mechanism. A command applied before process death cannot be rolled
back. A crashed sender with no pending command may conservatively
abort the original GUI preview session. Both PRs stay draft.