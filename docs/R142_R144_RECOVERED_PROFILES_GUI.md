# R142–R144 — source-shaped Profiles GUI reconstruction

**Branch:** `reconstruction/r1-core-fidelity` · draft PR #1 · **not merged**.

## Primary reference and authenticity

The user's candidate `decrypted files.zip/re/clickgui.tpl.html` contains a reconstructed HTML transcription of profile cards and controls. The code supports these visible behaviors:

- Existing profile cards have a name, separate bind chip, four 20×20 actions with 12px spacing: **Load**, **Save**, **Open folder**, **Delete**.
- **Default** is protected against Save and Delete.
- The New Profile card has an inline **Profile name** text field and **CREATE NEW PROFILE** row. Submit trims whitespace, rejects empty or duplicate names, clears the field and shows `Profile name is taken.` on a duplicate, and creates a new card.
- Each action uses the same centered `Are you sure?` confirmation modal. In the HTML transcription the Load, Save and Open Folder callbacks are **empty**. Delete removes a non-default card from the HTML model after confirmation.
- Profile bind chips capture a key or mouse button in the separate GUI UI.

**Source limitation:** The archive's reconstructed HTML is not an authenticated native commercial Slinky source dump. Its profile list includes synthetic demo examples `legit` and `bedwars`, but those are **not proof of real user profiles**. This implementation therefore starts with the genuine presentation-only Default card and allows newly created **local session-only** cards.

## Implementation

**R142**
- `MenuState` owns a bounded local vector of at most six profile preview records; no profile files are opened, written or synthesized on disk.
- The standalone GL11 GUI uses the recovered two-row, semi-transparent Create Profile card. A source-indexed name input is editable with native window `WM_CHAR`; the preview accepts bounded printable ASCII without path separators and limits names to 28 bytes, matching the existing immutable profile-card display contract.
- Trims surrounding spaces, detects duplicate names, shows `Profile name is taken.`, and creates a source-shaped new card with an empty description and unset binding.
- Profile-name keyboard focus never passes keystrokes to module toggles, profile bind capture, global hooks, JNI or Minecraft.

**R143**
- Separate modal state for four source profile-card icon actions. Load, Save and Open Folder produce the recovered confirmation questions and dismiss on Continue without touching actual profile files, folders or runtime settings.
- Save and Delete are disabled on Default. Confirmed Delete removes only a previously created **local preview card**, not a file or original client profile. Cancel preserves the preview card.
- Confirmation modal owns mouse and keyboard admission, rejecting click-through to underlying profile cards, navigation or other inputs. The geometry is shared between GL11 and native hit-testing.

**R144**
- Each profile card has a shared renderer/hit-test bind chip with source-style `NONE`, captured code or `PRESS A KEY...` label.
- Binding data is in the local preview record. Key/mouse capture only handles input in the current standalone window, and ESC clears it. Focus loss cancels pending capture without rewriting the previous committed label.
- No global Windows keyboard hooks, Minecraft keybinds, profile loading, disk serialization or autoactivation. The candidate reference's demo bindings are not preloaded.
- The existing preview bind vocabulary displays only labels that fit the profile card's eight-character binding display contract. Invalid or unsupported codes are rejected; no alternative gameplay binding is inferred.

## Regression and acceptance scope

CTest `native_recovered_profiles_create_and_actions` covers:
- Three panel modes and shared hitboxes for the name field, create action, icon row and profile key chips
- Name trimming, duplicate rejection, bounded input and invalid characters
- Local profile creation up to six cards, preserving Default and showing no offscreen New Profile button when full
- Protected Default Save/Delete, confirmation ownership, modal questions, cancel versus confirmed local deletion, no file or game mutations
- F6 and mouse-profile binding, ESC clear, invalid key rejection and pending focus ownership.

**Important:** A passing CTest or GitHub Actions build does **not** prove the original Slinky GUI is visually pixel-exact. Future work includes hover/icon tooltips, accurate embedded-font metrics, dynamic source scroll, full hover and tab transitions, genuine screenshot comparisons, and verified native behavior for each of 67 gameplay modules. The original injector/attach architecture remains distinct from this standalone preview. Verify GitHub Actions for the **exact latest head** before claiming R142–R144 passed.
