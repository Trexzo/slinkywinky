# R309 — offline clipped geometry/style provenance regression

Base: R308 `1c71ace16538a213584badab6349c30340b8f3be`. The R308 report describes native source selection, settings and style ordering; original relationship and target provenance in the running client is still unresolved.

## Defect
The source projection intentionally discards invisible or invalid projected bounds. The R308 diagnostic styled drawing method expected the compacted list to have the same length as the unprojected style list. Rejecting the whole frame on clipping avoided a wrong color but prevented rendering a valid remaining subset. Simply dropping that length check would pair the wrong entries.

## Change
Retain the original source row index privately in source-projected geometry. Pair geometry and style in the projection adapter, only after matching frame/world/player/generation/tick and original roster size. Return immutable paired rows. The dormant styled drawing adapter consumes those pairs rather than indexing the unprojected roster by compacted index.

The existing synthetic Java/JNI fixture now checks one off-screen leading row, two visible rows with different styles, and rejects a style plan from a later tick. The established R308 workflow includes the modified source and fixture paths.

## Limits
This is an offline diagnostic correction, not recovery of original live relationship/target providers. The ordinary Forge preview still does not invoke styled drawing. No new hook, input, game launch, original pixel parity or production integration is introduced. CI is not yet verified at commit creation. Both draft PRs remain separate. Original archive access was not independently re-established for this change; the R308 source report and code are the evidence for the defect.
