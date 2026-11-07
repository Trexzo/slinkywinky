# R137 — Source-identified inline text fields (GUI preview only)

**Repository:** Trexzo/slinkywinky · draft PR #1 on `reconstruction/r1-core-fidelity`, unmerged.

## Recovered reference and provenance

The user's candidate `decrypted files.zip` contains `re/clickgui.tpl.html`, whose reconstructed `it.k==='t'` widget emits an inline text field with a source placeholder. This is not authenticated official source; its exact font, caret, composition, selection and clipboard semantics remain to be verified.

The current 385-entry GUI fixture identifies **six text fields**:
- Block ESP / Block types — Search block
- Item ESP / Item types — Search item
- Auto Tool / Allowed blocks — Search item
- Friends / Add a friend — Enter friend username
- Weapons / Extra — Search item
- Auto Soup / Items — Search item

R137 gives all six *editable preview values*. Entering a Friends username **does not add a friend**, nor does entering text select inventory items or send a gameplay action.

## Implementation

- `MenuState::focus_recovered_text_preview(catalog,widget)` admits only a source-matched visible expanded five-control page, correct tab/module and non-modal context. The model owns one focused widget ID, not an arbitrary pointer or raw UI target.
- `MenuState::append_recovered_text_preview` validates exactly one well-formed printable UTF-8 Unicode scalar per insertion and limits the text to **128 bytes**. It rejects ASCII control codes, C1 controls, truncated/overlong UTF-8, surrogate code points and invalid non-Unicode values.
- `MenuState::erase_recovered_text_preview` removes one complete UTF-8 code point without splitting multibyte text. It cannot edit after blur, page change or modal activation.
- The Windows standalone WGL preview handles real `WM_CHAR` events, including valid UTF-16 surrogate pairs, and converts them to UTF-8 before model admission. While text is focused, `VK_BACK` deletes one character, and Enter/Escape leave the field. Navigation/activation shortcuts cannot pass through the focused field.
- The GL11 renderer draws an ephemeral copy of entered text, keeping the original source placeholder and an accent caret when the field is focused.
- The renderer-neutral `MenuRenderFrame` snapshots source-indexed text values and focused identity, so a held mouse click becomes stale when editing changes the visual state. Losing focus, changing tab/page, opening a popup/modal or clicking inert background dismisses text editing safely.
- This path does not access original Minecraft keyboard input, JNI, packets, live module settings, a saved profile or network services.

## Regression

`native_recovered_text_preview_safety` is registered in CTest. The new source-based test checks six field identities and source placeholders; UTF-8 scalar validity; Auto Tool's inline field hitbox in all three GL preview modes; wrong-tab/wrong-module refusal; typing ASCII, non-ASCII BMP text and emoji; rejecting invalid characters; complete-codepoint backspace; stale gesture invalidation; loss of focus from settings pagination and modal; a 128-byte bound; and unchanged module/profile state.

## Limitations and next gates

- **Not full original text editing parity:** cursor positioning, selection, paste/clipboard, IME composition, horizontally scrolling long input and original font metrics are not finished.
- **Not real gameplay:** Friends addition, search results and item filtering remain disconnected until separately reconstructed and verified.
- **Build status:** committed to draft PR, but full Windows Release/CTest, source screenshot comparison, interactive keyboard/IME and native Minecraft 1.8.9 verification have not been observed for this head. The connected Windows desktop was offline.

Next: source-backed action buttons (two Friends commands) should stay inert until their precise confirmation/permission boundary is proven, then finish profile/bind interactions and measure native GUI visual fidelity before claiming reconstruction completion.
