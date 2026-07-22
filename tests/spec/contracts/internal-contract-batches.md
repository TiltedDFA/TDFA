# Internal contract batches — draft

Snapshot: `6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592`

All entries are internal APIs. Purpose lines are intentionally brief and do not
authorize unstated behavior. The Types, Move, and Position defaults below are
`TENTATIVE`; all other ambiguous semantics remain `UNRESOLVED`.

## Types, moves, bitboards, and small utilities

| Symbols | Brief intended purpose |
|---|---|
| `operator!(Colour)` | Select the opposite chess colour. |
| `FloatDiv` | Produce a floating-point quotient from numeric operands. |
| `move_info::{move_info,add_move}` | Hold a bounded group of encoded moves and its attack mask. |
| `Moves::{EncodeMove,DecodeMove,StartSq,TargetSq}` | Convert between a move's encoded and component forms. |
| `Moves::{IsPromotionMove,PTypeOfProm}` | Identify and decode promotion information. |
| `MoveList::{MoveList,add,operator[],merge,all,len,contains}` | Accumulate and inspect a bounded move sequence. |
| `Magics::{SqToBB,FileOf,RankOf,BBFileOf,BBRankOf,ValidSq}` | Convert and classify board-square coordinates. |
| `Magics::{GetLS1B,FindLS1B,FindMS1B,PopLS1B,PopMS1B,PopNRetLS1B,PopCnt}` | Query and remove selected bits from a bitboard. |
| `Magics::{Shift,CollapsedFilesIndex,CollapsedRanksIndex}` | Transform bitboards into shifted or compact occupancy forms. |
| `Magics::{TypeOf,ColourOf}` | Decode an encoded piece. |
| `Magics::{KnightAttackingMask,KingAttackingMask,PrecomputeMask}` | Construct geometry-only attack masks. |
| `Magics::{compute_base_2_to_3,GetBaseThreeUsThem}` | Map two disjoint occupancy sets to a sliding lookup index. |
| `RemoveWhiteSpace`, `SplitFen`, `IsDigit` | Perform narrow lexical operations used by position input. |
| `UTIL::{Square,PromotionChar,MoveToStr,UciToMove}` | Convert internal squares and moves to or from UCI notation. |

Open decisions:

- D-TYP-001: fixed-width guarantees for aliases and whether enum numeric values
  are contractual or merely representation.
- D-TYP-002: zero-input behavior of bit scans/removals and invalid square/piece
  behavior in checked and unchecked builds.
- D-MOV-001: invalid/null move representation and its conversion behavior.
- D-MOV-002: `MoveList` append-versus-replace behavior, ordering guarantee,
  duplicate policy, and capacity overflow precondition.
- D-MOV-003: invalid notation behavior and whether `UciToMove` validates
  syntax, pseudo-legality, or full legality.
- D-UTIL-001: exact whitespace accepted by lexical helpers and required behavior
  when FEN has fewer/more than six fields.

Tentative defaults awaiting owner confirmation: fixed-width aliases; enum values
are internal encoding where explicitly serialized; zero input has a documented
safe sentinel or explicit precondition; move lists append with unspecified order;
ordinary comparison considers only populated entries; null move renders `0000`.

## Board and Position

| Symbols | Brief intended purpose |
|---|---|
| `Board::{Board,ResetBoard}` | Create or clear the piece placement. |
| `Board::Pieces` overloads | Query occupancy by colour and/or piece type. |
| `Board::{PieceOn,MakePiece}` | Query or construct a piece identity. |
| `Board::{AddPiece,RemovePiece,PopPiece,MovePiece}` | Apply a primitive piece-placement transition. |
| `Board::pedantic_check` | Diagnose violations of primitive board preconditions in checked builds. |
| `StateInfo::StateInfo` | Create a default reversible-position state record. |
| `Position::{Position,Reset,ImportFen}` | Create or replace a complete chess position. |
| `Position::{MakeMove,UnmakeMove}` | Apply and reverse one encoded chess move. |
| `Position::{EmptySqs,EnPasBB,EnPasSq,CastlingRights,ColourToMove,ZKey,HalfMoves,FullMoves}` | Observe current position state. |
| `Position::IsOk` | Report whether the position satisfies its documented consistency level. |
| `Position::HashCurrentPostion` | Produce or refresh the current position's Zobrist identity. |
| `Position::UpdateCastlingRights` | Update rights affected by the current transition. |

Open decisions:

- D-POS-001: whether FEN acceptance is lexical, structural, or fully legal; exact
  invalid-input response; and whether failed import is atomic.
- D-POS-002: whether construction/import computes the hash automatically and
  whether `HashCurrentPostion` is a pure query or state-mutating refresh.
- D-POS-003: initial/fullmove convention and precise halfmove/fullmove updates.
- D-POS-004: `MakeMove` input precondition: encoded, pseudo-legal, or legal move.
- D-POS-005: exact make/unmake restoration set, history discipline, and behavior
  when unmake order/move mismatches.
- D-POS-006: whether `IsOk` promises structural coherence or full searchability.
- D-POS-007: copy/move behavior and ownership of accumulated state history.

Tentative defaults awaiting owner confirmation: FEN failure is rejected
atomically; import establishes a usable hash; make/unmake exactly restores all
logical state; FEN fullmove follows the standard one-based field and increments
after Black; `IsOk` is structural, with legal/searchable validation handled by
test oracles rather than inferred.

## Move generation and rules

| Symbols | Brief intended purpose |
|---|---|
| `GenerateMovesFromBB` | Encode destinations from a bitboard into a move list. |
| `GetMovesForSliding` | Obtain sliding movement information for an occupancy configuration. |
| `WhitePawnMoves`, `BlackPawnMoves` | Generate pawn moves for the named colour. |
| `BishopMoves`, `RookMoves`, `QueenMoves`, `KnightMoves`, `KingMoves` | Generate piece moves for the named colour/type. |
| `PawnAttacks`, `KnightAttacks`, `BishopAttacks`, `RookAttacks`, `QueenAttacks`, `KingAttacks` | Produce attacked-square sets for the named colour/type. |
| `InCheck` | Determine whether the named side's king is attacked. |
| `GenerateAllAttacks` | Combine attacked-square sets for one side. |
| `Castling` | Add available castling moves for one side. |
| `GeneratePseudoLegalMoves` | Generate moves satisfying movement rules before king-safety filtering. |
| `GenerateLegalMoves` | Generate the complete legal move set for the side to move. |

Open decisions:

- D-GEN-001: every generator's append/replace behavior, capacity precondition,
  duplicate guarantee, and whether ordering is explicitly unspecified.
- D-GEN-002: accepted position validity and relationship between template colour
  and `Position::ColourToMove()`.
- D-GEN-003: whether attack maps include friendly-occupied defended squares,
  pinned-piece attacks, and squares the king cannot legally enter.
- D-GEN-004: whether castling additionally requires the king/rook on their
  conventional squares when rights are present.
- D-GEN-005: guarantees for missing/multiple kings and other malformed states.
- D-RULE-001: draw/repetition/dead-position scope and where those outcomes belong.

Recommended defaults: append with no contractual order; no duplicates; adequate
capacity is a precondition; searchable legal position is a precondition; attack
maps follow chess attack semantics including defended friendly occupancy;
castling requires actual king/rook placement as well as rights.

## Evaluation, search, and time

| Symbols | Brief intended purpose |
|---|---|
| `Eval::{CountMaterial,Mobility,PawnProgress,UpdateData,Evaluate}` | Compute documented components and a position score. |
| `Search::GoSearch` | Search a position to the requested depth within its controls and return a score. |
| `Search::FindBestMove` | Select a best legal root move under the supplied controls. |
| `Timer::{Timer,~Timer}` | Measure a scope and publish or print its duration. |
| `TimeManager::{SetOptions,StartTiming,OutOfTime}` | Configure and observe a search deadline. |

Open decisions:

- D-EVAL-001: score perspective, exact component contract, symmetry expectations,
  valid score range, and whether `UpdateData`/global phase state is observable.
- D-SRCH-001: depth-zero and quiescence semantics; terminal and mate-distance
  scores; best-move tie behavior.
- D-SRCH-002: TT-null/disabled semantics and which TT behavior may affect results.
- D-SRCH-003: timeout result convention, polling granularity, completed-iteration
  guarantee, and exact root restoration on all exits.
- D-SRCH-004: whether deterministic node/depth control is required without a
  search refactor; otherwise record polling-point tests as blocked by testability.
- D-TIME-001: initial state, allowance policy, zero/overflow behavior, clock type,
  and whether equality at the deadline counts as timed out.
- D-TIME-002: whether `Timer` output is a contract or diagnostic-only behavior.

No default is frozen. Search cancellation must not be tested with brittle sleep
timings; inaccessible deterministic polling behavior is recorded as blocked.

## Transposition table

| Symbols | Brief intended purpose |
|---|---|
| `TransposTable::{TransposTable,~TransposTable}` | Own the table's storage lifetime. |
| `Resize` | Select table capacity from a requested memory budget. |
| `Store` | Associate a position key with search information. |
| `Probe` | Retrieve matching search information when present. |
| `Clear` | Invalidate all stored entries. |
| `GetNumElems` | Report the current entry capacity. |

Open decisions:

- D-TT-001: size units, rounding, minimum, maximum/failure, and zero-size behavior.
- D-TT-002: replacement policy and whether it is contractual or unspecified.
- D-TT-003: collision guarantee and precise miss representation.
- D-TT-004: pointer lifetime after store/clear/resize/destruction.
- D-TT-005: copy/move policy and thread-safety of every operation.
- D-TT-006: whether draw-sensitive clock/history context must prevent unsafe reuse
  even though it is not normally part of the Zobrist board key.

Recommended defaults: zero size safely disables storage; failed allocation leaves
a valid object; key mismatch is always a miss; pointers expire on any modifying
operation; ownership is noncopyable; no thread-safety unless explicitly added.

## UCI internals, debug, and legacy testing helpers

| Symbols | Brief intended purpose |
|---|---|
| `Uci::{Uci,Loop}` | Own and run an engine command session. |
| `Uci::Handle*` | Process the corresponding command or diagnostic operation. |
| `Debug::Print*`, `PieceTypeToStr` | Format internal values for human diagnostics. |
| `PerftHandler::{RunPerft,RunBulkPerft,PrintData,GetNodes}` | Count and optionally report legal move trees. |
| `TestSearch`, `TestPerft`, `TestBulkPerft`, `RunBenchmark`, `RunBulkBenchmark`, `RunPerftSuite` | Run legacy developer diagnostics or benchmarks. |
| legacy `Split` | Divide a perft-suite record into fields. |

Open decisions:

- D-UCI-009: internal handler preconditions and whether a handler may partially
  mutate state before detecting invalid input.
- D-DBG-001: whether diagnostic text has stable formatting; recommendation is no.
- D-LEG-001: whether legacy helpers are retained as tested tooling or superseded
  by assertion-based tests and excluded from semantic contracts.
- D-LEG-002: if retained, whether failure must propagate as a failing process/test
  and whether split includes the final field.

Application-observable UCI behavior is governed by `application.md`; these
internal decompositions must not create stronger application requirements.

