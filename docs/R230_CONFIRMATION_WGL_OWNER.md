# R230 — Unload and profile-action modal ownership, native WGL dispatch

Date: 2026-11-15. Chat 1, PR #1, reconstruction/r1-core-fidelity.

## Verified source gap

The first-party renderer and hit tester already defined and painted
Unload confirmation and Profile action/confirmation controls, but the
retained WGL pointer dispatcher lacked their action cases. The
MenuState select() and scroll() paths also treated non-dropdown
confirmations as non-exclusive, permitting direct navigation to
silently dismiss them. A profile action could be replaced by Unload
without its own explicit Close/Continue button.

## Native implementation

- Added a read-only MenuState::modal_input_owned() predicate,
  combining any dropdown, Unload confirmation and Profile action.
  No MenuState object-layout or compiled Java ABI changes.
- Model-level select, both scroll methods, expand/toggle, setting
  pages, Aim/Target mode, accent and scale now refuse while a
  confirmation is pending. The existing dialog, tab, source module
  identity, pointer geometry, and profile remain unchanged.
- The Unload opener refuses to replace an existing profile action.
  Its only permitted exits are its explicit close/continue events.
- MenuNavigation direct tab/focus/apply and synthetic keyboard router
  now quarantine input for both confirmations, reusing the existing
  ABI-preserving modal predicate. Escape, Tab, Enter and arrows do
  not reach background GUI navigation or request game close.
- Retained WGL apply, scroll and focused-runtime-toggle admission
  refuse modal-background operations before revoking the actually
  painted pointer hit rectangle.
- Retained WGL pointer release now dispatches the original typed hits
  for UnloadTab, ModalClose, ModalContinue, ProfileAction,
  ProfileModalClose and ProfileModalContinue to the already-existing
  in-memory MenuState preview methods.
- Profile bridge toggle, primary parameter adjustment and settings
  adoption cannot mutate native settings through these modals.

## Offline regression coverage

- Tests for Unload confirm modal on all three panel modes, including
  direct model/profile background mutations, tab switch refusal,
  explicit-close stale pointer rejection, and restored navigation.
- Tests for Profile action modal on all three panel modes, including
  pending-action preservation, rival Unload refusal, background
  scroll/tab/profile changes, and explicit in-memory confirmation.
- Synthetic keyboard router tests verify exact menu-owned key pairs
  for both confirmations without a game close or gameplay event.
- A native GL11/WGL fixture opens Unload via the actual navigation
  tab, closes it via the painted X, visits Profiles, opens the
  Default Load confirmation via its original icon, closes via X,
  verifies that denied navigation retains the painted geometry,
  checks menu-only key delivery, normal navigation restoration,
  and unchanged serialized profile/GL state.

None of these events launches Minecraft, installs a keyboard hook,
unloads a process, opens profile folders, or writes real profile
files. Production runtime behavior is not claimed as validated.

## CI status

Pending exact-commit GitHub CI validation.