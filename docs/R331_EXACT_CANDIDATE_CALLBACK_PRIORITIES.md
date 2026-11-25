# R331 — SHA-pinned original binary callback priority reconciliation

Original local reference: `decrypted files.zip` (SHA-256 `326efa346905370ed1815aba2247d35666008346dd6858262c38070d0f42aea4`), containing original-decrypted `.data` image at `slinky2.decrypted/data.bin` (SHA-256 `8aee98d473f5092686ea3ed080828bbb02500e4bbd8d89130a3ce7751176dea5`).

Read-only parser evidence: descriptor offsets from `re/modules_real.json`; module record +0x48 + (event * 16), each <Qii callback record; emitted call targets were checked for the four R329 controllers. Recovered original candidate callbacks:

| Module | Event/priority pairs |
| --- | --- |
| Backtrack | 2/0, 6/0, 18/0 |
| Lag Range | 3/-100, 6/0, 15/0, 18/0 |
| Timer Range | 0/-100, 1/0, 3/0, 5/0 |
| Inv Walk | 5/0, 17/0, 23/0, 25/0 |

R331 enforces these complete event/priority sets before the four existing `ModuleExecutionBridge` source tick bindings are admitted. The first R330 implementation only checked event membership. Fixture improvements add wrong-priority negative controls and use identical equal-priority event18 registration. Equal-priority source qsort behavior remains unverified; the first-party event replay uses its documented stable descriptor tie order.

The Backtrack packet classifier now checks the **exact target ID** for S19 status feedback, and the **exact local player ID** for a velocity release, as in candidate source `backtrack_packet_hook_under`. Previously, the classifier incorrectly bypassed all S19 status packets and relied on a pre-labelled 'self_velocity' kind. New negative fixtures reject unrelated entity status/velocity.

No private binary, decrypted bytes, original symbols, or proprietary source are committed. Candidate DLL commercial authenticity and full callback logic remain unverified; priorities and descriptor IDs are *metadata-level* parity evidence only. No live packet, input or timer hook is activated. PR #1 untouched and both PRs remain draft/unmerged.
