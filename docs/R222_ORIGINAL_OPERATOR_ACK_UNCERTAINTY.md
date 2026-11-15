# R222 — Original-client operator ACK timeout isolation

Date: 2026-11-15. Chat 1; draft PR #1.

## Source-proven failure window

The original-client native menu uses PID/birth-scoped local Win32 command events,
a sender mutex, and two shared accepted/refused acknowledgement events.
The mutex stops simultaneous senders, but it does not correlate ACKs after
a sender exits on timeout. A late reply to sender A can appear during sender
B's wait, producing a false success/refusal for B.

## Fail-closed first-party repair

- Add a sticky manual-reset uncertain-session event at slot 12. Keep
  all existing command, ACK and mutex slot names unchanged.
- The sender must observe the event unset before dispatch. Any unknown
  acknowledgement outcome sets it before releasing the producer mutex.
- The source-pinned original-client JNI render owner refuses further
  operator commands if the latch is set, including after dequeue.
- The Java operator owner already treats a negative poll as poisoned
  and enters its existing cleanup/verified-bytecode-restoration flow.
- No vanilla keyboard or mouse hooks, gameplay module actions, packet
  changes, Forge dependencies, or renderer changes are introduced.

## Validation scope

The first-party WGL/JNI fixture retains all normal accepted/refused
navigation and STOP checks and now asserts that a poisoned session
does not acknowledge or dequeue another posted command.
Windows 11, Java 8, MSVC x64 Release, CMake configured with
SLINKYWINKY_BUILD_JNI=ON, SLINKYWINKY_BUILD_ATTACH=ON and
BUILD_TESTING=ON, fresh clone at exact R222 commit a7ec40c:
the full native/Java/Attach/WGL CMake build completed successfully
and CTest reported **107/107 tests passed in 37.36 seconds**.
This includes isolated_wgl_menu_state_restore with the new poison
negative control and native_agent_attach (29.87 seconds).
No original Minecraft process was launched. GitHub Actions CI and
physical input acceptance remain independently unverified.
No original Minecraft process or physical hardware input is exercised
by this source-level patch.

This does not revoke an action already committed before a sender timed
out, and local same-user Win32 events are not a cryptographic security
boundary. It prevents accidental reuse of a late ACK as another action.
Both PRs remain draft and unmerged.