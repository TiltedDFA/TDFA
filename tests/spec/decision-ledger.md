# Contract decision ledger

Snapshot: `6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592`

Record owner answers here before changing a contract from `UNRESOLVED` or
`TENTATIVE` to `APPROVED`. Do not derive answers from implementation bodies.

| Decision IDs | Subsystem | State | Decision/evidence |
|---|---|---|---|
| D-TYP-001..002, D-MOV-001..003, D-UTIL-001 | Types/moves/utilities | TENTATIVE | Recommended defaults sent to owner; awaiting correction/confirmation. |
| D-POS-001..007 | Board/Position | TENTATIVE | Recommended defaults sent to owner; awaiting correction/confirmation. |
| D-GEN-001..005, D-RULE-001 | Move generation/rules | UNRESOLVED | See subsystem batch. |
| D-EVAL-001, D-SRCH-001..004, D-TIME-001..002 | Evaluation/search/time | UNRESOLVED | See subsystem batch. |
| D-TT-001..006 | Transposition table | UNRESOLVED | See subsystem batch. |
| D-UCI-001..009 | Application/UCI | UNRESOLVED | See application and internal batches. |
| D-DBG-001, D-LEG-001..002 | Diagnostics/legacy tests | UNRESOLVED | See subsystem batch. |

Use this record format for each resolved decision:

```yaml
id: D-<SUBSYSTEM>-NNN
status: PROPOSED | DECIDED | SUPERSEDED
question: one independently understandable semantic question
options_considered: []
decision: exact selected behavior
rationale: why this behavior is intended
decided_by: owner id
decided_at: YYYY-MM-DD
affected_contracts: []
contract_version_after: 0
supersedes: null
```

