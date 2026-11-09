# R184 — painted-frame fence for inert retained ClickGUI pointer transactions

**Branch:** `reconstruction/r1-core-fidelity` (draft PR #1). The independent OpenSlinky reference track, PR #2, is unchanged.

## Source issue and fix

R180 protected *passive hover* against inspecting unpainted GUI navigation, but `WglRetainedMenuPreview::pointer()` could still build a fresh `MenuRenderSession::next(...)` snapshot for a synthetic Down/Move/Up transaction. This allowed an explicitly attested **local preview** input to hit a configuration panel that had not yet been painted by GL11.

R184 makes the pointer transaction obey the same last-painted source of truth as passive source hover. Non-Cancel phases require `last_drawn_frame_` from a successful WGL `draw`; they use that snapshot, never an on-demand future layout. The existing pinned render-thread/context/DC/viewport, monotonic timestamp, foreground and separately attested exclusive-dispatch gates remain. Cancellation still works for an active owner.

`apply(MenuNavAction)` already cancels a held gesture and invalidates the painted frame. A completed preview click also invalidates it so the next gesture must wait for a real draw. In the slider dispatch branch, the **old drawn frame must remain alive** until source geometry and the local profile value have been evaluated. A narrow scope guard resets the optional when dispatch returns; resetting it before reading the geometry would cause a dangling reference.

No original Minecraft input is consumed, no mouse events are synthesized and no module/gameplay actions are enabled.

## Windows tests

- The exact disposable native WGL retained-pointer test now asserts that a second click immediately after a committed click is rejected until repaint; a local NextRow/ExpandFocused navigation similarly rejects a synthetic slider Down before repaint.
- A subsequent slider Down/Up is accepted only against the `MenuSourceHoverTarget` independently certified from the newly painted original Auto Block/Main/Range frame. Profile snapshot round-trip remains local-only.
- R167–R169 disposable Java 8/JNI tests now explicitly reject pre-repaint clicks, issue real WGL redraws after navigation/commits, and retain their existing slider range, Boolean paging and dropdown modal negative controls. No production JNI action bypasses the paint fence.
- The Java 8 R182 negative receipt fixture previously mislabeled `source_hits=4` as invalid when `numeric_hits=4` (equal is valid). Its negative now uses `source_hits=5`, correctly testing the strict inequality.
- An incremental Windows build after the R183 header layout change showed `0xc0000409` failures in the two older native WGL executables. **Both passed in a newly configured, fully built Release directory**, without a production change or a larger stack. This supports stale/inconsistent incremental objects as the cause; it does not prove that no other Windows builds could reproduce a problem.

R183's original-window OS focus tri-state is read-only. Passing disposable WGL/JNI tests **does not** establish an actual foreground-background-foreground interaction in an original Minecraft 1.8.9 game or grant safe LWJGL event ownership. Leave all live click delivery disabled; keep PR #1 draft and unmerged.
