# R150 — control motion and text alignment

Draft PR #1; Windows standalone preview only. R150 adds an isolated 100ms Boolean hover tint and 120ms dropdown fade, plus vertically centered label placement for 22px widgets, 20px buttons, and popup option rows. Both durations and tint are explicit approximations, not claims of original Slinky binary fidelity. The decrypted candidate HTML is not authenticated signed source.

Boolean hover is selected by existing source-indexed hit testing; stale widgets, popups and off-screen controls cannot establish hover ownership. While a popup is fading in, click and wheel input are withheld. Paint data and the 16ms window-local timer do not modify MenuState, Minecraft, Java attach hooks, or game inputs. A source-bounded deterministic regression was added.

Pending: verify current Windows/Linux CTest and workflows, inspect actual original-client screenshots, and individually implement/validate 67 gameplay modules. ClickGUI is not complete.
