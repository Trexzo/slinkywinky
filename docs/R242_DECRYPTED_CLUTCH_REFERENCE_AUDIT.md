# R242 — direct Clutch candidate-table comparison to decrypted reference

Date: 2026-11-20. SlinkyWinky draft PR #2; PR #1 untouched.

## Primary evidence

The operator-owned Library file `decrypted files.zip` was retrieved
for **read-only** ZIP/PE-section inspection. No executable was run, injected,
linked or copied into GitHub. This is a candidate Slinky binary; its official
release authenticity is not independently established.

| Fact | Observed |
| --- | --- |
| ZIP entry | `decrypted files/slinky2.decrypted/rdata.bin` |
| Original `.rdata` length | 1,727,316 bytes |
| Original `.rdata` SHA-256 | `61eaa6bad74fd5cf0d374198506ec4f87fa042b5cf673e37ae01e87eb336706a` |
| Distance counts | `1,6,19,43,79,123,163,167` — exact 8/8 |
| Source candidate offsets | 167/167 packed little-endian uint16 words match |
| Combined counts + offsets | 350/350 exact bytes, one occurrence |
| Byte offset in raw extracted `.rdata` | `0x18280` (decimal 98,944) |
| Offset of first packed candidate | `0x18290` |
| Packed candidate SHA-256 (334 bytes) | `04a82e7f286d1163d7a515290190c67631005df9c984604ee582e8985784da48` |

The exact comparison used the **current first-party branch header**
`reference_compat/include/slinkywinky/clutch_candidate_table.hpp`,
not merely the upstream OpenSlinky transcription. This substantiates
the R235 table's exact data fidelity to this decrypted archive.

The archive also contains `src/gen/mod_clutch.cpp`, but it is a
reconstruction of setting registration and GUI callback functions,
**not** complete original Clutch gameplay logic. Other game-related
native routines, ownership, the original live RNG state, and event timing
still need independent implementation and verification.

## Reproducible verifier

`tools/verify_r242_clutch_reference.py` parses the branch's source
header and reads only the specific `.rdata.bin` ZIP member. It checks
the pinned full-section SHA-256, table length, all values, contiguous
byte equality, unique occurrence, and exact reference offset.

Run locally with the user-owned original archive:

```sh
python tools/verify_r242_clutch_reference.py --archive "/path/to/decrypted files.zip"
```

Five independent synthetic negative controls validate rejecting
missing/truncated/altered/duplicated tables, without shipping proprietary
reference bytes. CI runs **only those synthetic tests** because the
130 MB reference archive must never be committed or distributed in
GitHub Actions. Local reference-byte inspection and CI synthetic
self-tests are different evidence levels.

This does not establish original live game feature parity, commercial
authenticity, soundness of Java/JNI mapping, packet/hook fidelity or
completed Slinky client. Both PRs stay draft/unmerged.
