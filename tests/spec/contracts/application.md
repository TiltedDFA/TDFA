# Application contracts — draft

Snapshot: `6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592`

These are the only application-facing contracts. C++ visibility does not make a
symbol application-facing. This batch is `TENTATIVE` until the supported UCI
feature set and deliberate extensions are owner-approved.

Authorities are the [UCI protocol](https://backscattering.de/chess/uci/), FEN,
and the [FIDE Laws of Chess](https://handbook.fide.com/chapter/E012023), with an
owner decision taking precedence only for an explicitly documented extension.

| Contract | Input/behavior | Brief intended purpose | Status |
|---|---|---|---|
| C-APP-001 | process start/EOF/`quit` | Start a command session and terminate cleanly when directed or when input closes. | TENTATIVE |
| C-APP-002 | `uci` | Identify TDFA, publish options, and complete the UCI handshake. | TENTATIVE |
| C-APP-003 | `isready` | Confirm command processing readiness without changing the chess position. | TENTATIVE |
| C-APP-004 | `setoption name Hash value N` | Configure the advertised transposition-table capacity within documented bounds. | UNRESOLVED |
| C-APP-005 | `ucinewgame` | Discard state that must not leak between games. | UNRESOLVED |
| C-APP-006 | `position startpos [moves ...]` | Establish the initial position and apply a legal UCI move history in order. | TENTATIVE |
| C-APP-007 | `position fen <fen> [moves ...]` | Establish the supplied FEN position and apply a legal UCI move history in order. | UNRESOLVED |
| C-APP-008 | `go ...` | Search under the supported limits and eventually emit one protocol-valid best move. | UNRESOLVED |
| C-APP-009 | `stop` | Request the active search to finish promptly and emit its best completed result. | UNRESOLVED |
| C-APP-010 | emitted moves | Emit legal long-algebraic UCI moves; promotions use `q/r/b/n` and no-move uses `0000`. | TENTATIVE |
| C-APP-011 | malformed/unknown input | Remain memory-safe and preserve state according to a documented recovery rule. | UNRESOLVED |
| C-APP-012 | stdout/stderr | Keep stdout protocol-clean; route any non-protocol diagnostics according to policy. | UNRESOLVED |
| C-APP-013 | `bench`/`print` | Provide optional engine-specific diagnostic commands, if retained as supported behavior. | UNRESOLVED |

## Required decisions

- D-UCI-001: exact supported `go` tokens and behavior for unsupported tokens.
- D-UCI-002: command/option casing policy.
- D-UCI-003: malformed-command, malformed-FEN, and illegal-history recovery.
- D-UCI-004: whether `position` commits atomically.
- D-UCI-005: `stop`, `quit`, and EOF behavior while searching.
- D-UCI-006: Hash minimum, maximum, units, and rounding.
- D-UCI-007: whether `bench` and `print` are supported extensions or non-contractual diagnostics.
- D-UCI-008: stdout/stderr diagnostic policy.
- D-RULE-001: draw adjudication promised by application search: repetition,
  50/75-move, insufficient material/dead position, and claimable draws.

