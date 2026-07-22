# Contract card schema

Every behavioral expectation must trace to an approved contract card. Purpose
text is deliberately short and non-prescriptive; it is not itself a complete
behavioral contract.

```yaml
id: C-<SUBSYSTEM>-NNN
snapshot: <64-character snapshot id>
version: 1
status: UNRESOLVED | TENTATIVE | APPROVED | RETIRED
kind: application | internal | mathematical
signatures:
  - exact declaration or application command
purpose: one or two implementation-neutral sentences
authority:
  type: UCI | FIDE | FEN | mathematical | owner-decision
  reference: URL, theorem/model identifier, or decision id
preconditions:
  - valid inputs and required state
result:
  - observable return/output/postcondition
mutates:
  - permitted state changes, or none
preserves:
  - state that must remain unchanged
failure:
  - invalid-input and resource-failure behavior
ordering: stable rule | explicitly unspecified | not applicable
determinism: deterministic inputs/state, allowed nondeterminism, or not applicable
oracle:
  type: standard | exhaustive-model | reference-model | corpus | metamorphic
  id: independently versioned oracle identifier
open_decisions:
  - D-<SUBSYSTEM>-NNN
review:
  author: agent/person id
  approvers: []
  approved_at: null
```

Rules:

- `APPROVED` requires no open decisions and at least two independent approvers.
- A card never cites a TDFA function body or observed TDFA result as its oracle.
- Unspecified ordering must be compared as a set or multiset, not a sequence.
- Invalid inputs are tested only when the card defines behavior for them.
- A changed contract increments `version` and invalidates dependent approvals.
- A card may cover a coherent family of templates/overloads when their contract
  is identical; traceability still lists every exercised symbol.

