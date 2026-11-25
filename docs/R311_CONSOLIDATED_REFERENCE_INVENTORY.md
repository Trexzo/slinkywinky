# R311 — consolidated private reference reconciliation

Base R310 80947f35db9a68eeca0808fc4c6dd8f937268e24: all 13 GitHub checks passed. Two exact user Library archives were read without extracting, executing or uploading any reference binary. Their SHA-256 pins are in tools/audit_r311_reference_inventory.py. The script ran successfully locally on both archives.

## Reconciled scope

The decrypted candidate archive contains 67 unique descriptors. A scan of the actual raw read-only data finds exactly one maximal contiguous 66-pointer run covering 66 unique descriptors and excluding Chest Stealer. A second shorter run has six pointers. This independently corroborates the R193/R196 choice to exclude Chest Stealer from the 66-module target. It is static pointer-table evidence, not live proof of original commercial registration.

All 66 pointer-table descriptors have matching category, default-disabled state and GUI pointer metadata. Direct decoding of 27 callback slots in the candidate data yields 171 non-null callbacks, all pointing within the candidate code section, priorities -10000 to 10000, and histogram:
24,5,6,10,3,25,7,4,4,3,3,2,1,5,10,6,3,9,20,1,1,3,2,1,1,5,7.
This independently agrees with R196 linked metadata totals (event 0: 24; event 5: 25). This does not validate handler behavior. 60 of the 66 registered descriptors have a GUI callback; Chams and No Hurt Cam have zero ordinary event-slot subscriptions.

| Inventory | Scope |
|---|---:|
| Pointer-table target | 66 |
| All descriptors | 67 |
| C++ candidate manager count | 67 |
| C++ candidate logic source files | 66 |
| Legacy extracted name/description entries | 64 |
| Recovered Java class-name snapshot | 68 |

Active category counts: combat 17, move 10, visual 15, block 7, utility 17. Matrix correction: Ghost Hand is Block, not Combat. C++ candidate has no separate No Hurt Cam logic file. The legacy name list lacks Auto Soup, Bridge Assist, Teams and Timer; includes Remove All Friends (not a descriptor). The recovered generated-primary index lacks eight named module primary files; auxiliary files are not proof that these modules are missing from the original. Java-only Speed is outside the native descriptor roster.

## Remaining work accounting

The C++ archive's own per-module completeness report lists 424 settings and 200 extracted logic functions across 67 descriptors; restricting to the 66 table members yields 419 reported settings and 199 reported functions. These are counts from the reference report, not independently reconstructed setting semantics or function bodies.

Current first-party header/matrix audit: 36 offline controllers, 6 experimental decision models, 24 missing original-table implementations plus descriptor-only Chest Stealer. No claim of 36 playable modules or that offline models cover all 171 callbacks. Future acceptance must separately track settings semantics, event dispatch, local offline test evidence, runtime integration, lifecycle, rendering and authenticated reference parity.

## Reproduction

Run python tools/audit_r311_reference_inventory.py with the two archive paths. Output is an address-free, 66-module JSON inventory containing category, GUI-callback presence, number of subscriptions, source-file availability and reference-report setting/function counts. Reference files are not committed. The tool checks the complete archive SHA-256 values before ZIP access.

## Limits

Static candidate archive identity is established, commercial provenance is not. This batch is a comprehensive reference-scope audit, not a replacement for the remaining reconstruction or full runtime testing. No new gameplay execution or native loading is introduced. PRs remain separate and draft.
