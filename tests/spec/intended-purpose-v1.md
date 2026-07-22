# TDFA intended-purpose annotations v1

Snapshot: `6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592`  
Source-aware author: Codex agent `/root/purpose_annotator`  
Authoring input: the frozen `src/**/*.{hpp,cpp}` snapshot and `tests/spec/api-dossier.md`

## Information boundary

This is a source-aware purpose map, not a behavioral contract and not a test oracle. It records only the responsibility and broad semantic outcome that can be inferred confidently from names, comments, and use; it deliberately excludes algorithms, numeric constants, lookup encodings, branch structure, iteration or move ordering, tie-breaking, and observed implementation quirks or failures.

`Firm` means the stated purpose is inferable from the frozen source, but expected values still require an approved contract or independent authority. `Owner policy` identifies behavior that a blind test writer must leave unasserted until the owner approves it; where a row contains both labels, only its core purpose is firm.

The author did not build or run TDFA and did not author tests, fixtures, or expected values during this stage.

## `src/Board.hpp`

| Callable/function group | Brief intended purpose and semantic outcome | Boundary |
|---|---|---|
| `Board::Board` | Construct an initialized board with no pieces in any placement view. | Firm. |
| `Board::ResetBoard` | Clear the square map and all aggregate occupancy views back to an empty, mutually consistent board. | Firm. |
| `Board::Pieces(PieceType)` | Return the occupied squares for the requested piece-type category. | Firm core; owner policy: invalid or sentinel categories. |
| `Board::Pieces(Colour)` | Return all occupied squares belonging to the requested colour. | Firm core; owner policy: invalid colour input. |
| `Board::Pieces(Colour, Colour)` | Return all occupied squares for the normal both-colour occupancy query. | Firm core; duplicate or invalid colour arguments are owner policy. |
| `Board::Pieces(PieceType, PieceTypes...)` | Return the union of the requested piece-type occupancies. | Firm core; owner policy: empty, duplicate, or invalid category arguments. |
| `Board::Pieces(Colour, PieceTypes...)` | Return the requested colour's occupancy restricted to the requested piece-type categories. | Firm core; owner policy: invalid categories. |
| `Board::PieceOn` | Report the piece identity occupying one board square, or the empty identity when no piece occupies it. | Firm core; owner policy: out-of-board input. |
| `Board::MakePiece` | Combine a valid colour and piece type into the corresponding piece identity. | Firm core; owner policy: invalid or non-piece categories. |
| `Board::pedantic_check` | Diagnose violations of primitive board-mutation preconditions in checking builds without defining release behavior. | Firm diagnostic role; owner policy: exact checks and failure mechanism. |
| `Board::AddPiece` | Place one piece on an empty square and update every occupancy view consistently. | Firm core; owner policy: invalid piece, invalid square, or occupied destination. |
| `Board::RemovePiece` | Remove the piece on a square and update every occupancy view consistently. | Firm core; owner policy: empty or invalid square. |
| `Board::PopPiece` | Remove and return the piece that occupied a square while preserving board-view consistency. | Firm core; owner policy: empty or invalid square. |
| `Board::MovePiece` | Relocate one existing piece between squares and update every placement view consistently. | Firm core; owner policy: empty source, occupied destination, or invalid squares. |

## `src/BoardUtils.hpp`

| Callable/function group | Brief intended purpose and semantic outcome | Boundary |
|---|---|---|
| `RemoveWhiteSpace` | Remove boundary padding from an input view before position-text parsing. | Firm core; owner policy: which whitespace characters are recognized. |
| `SplitFen` | Divide a FEN record into its six logical fields without interpreting their chess meaning. | Firm core; owner policy: missing, extra, empty, or multiply separated fields. |
| `IsDigit` | Classify a character as an ASCII decimal digit for narrow position-input parsing. | Firm; structural FEN digit validity belongs to the FEN parser contract. |

## `src/Debug.hpp`

All functions in this section are human diagnostics. Their exact wording, whitespace, glyphs, capitalization, orientation, destination stream, and enabled/disabled build behavior are owner policy unless separately promoted to an application contract.

| Callable/function group | Brief intended purpose and semantic outcome | Boundary |
|---|---|---|
| `Debug::PrintBB(BitBoard, bool)` | Render a bitboard in a human-readable board layout, optionally changing its viewing orientation. | Firm diagnostic role; formatting is owner policy. |
| `Debug::PrintBB(BitBoard, int, bool)` | Render a bitboard while visibly marking one selected board location. | Firm diagnostic role; invalid marker and formatting behavior are owner policy. |
| `Debug::PrintBoardState` | Report the principal non-placement state of a position for inspection. | Firm diagnostic role; included fields and formatting are owner policy. |
| `Debug::PrintInduvidualPieces` | Render the per-colour, per-piece occupancy sets supplied by legacy board storage. | Firm diagnostic role; input-shape preconditions and formatting are owner policy. |
| `Debug::PieceTypeToStr` | Produce a human-readable label for a piece type. | Firm diagnostic role; exact labels and invalid-category handling are owner policy. |
| `Debug::PrintEncodedMoveStr` | Show the decoded components of an encoded move for a developer. | Firm diagnostic role; displayed fields and formatting are owner policy. |
| `Debug::PrintEncodedMoveBin` | Show an encoded move in a binary diagnostic representation. | Firm diagnostic role; width and formatting are owner policy. |
| `Debug::Fun(BitBoard)` | Traverse the set squares of a bitboard as an undeclared developer placeholder. | Orphan definition with no callers or observable effect; intended behavior is unresolved owner policy. |
| `Debug::PrintUsThem` | Render two occupancy sets as friendly and opposing pieces while retaining square-location cues. | Firm diagnostic role; overlap handling, orientation, and formatting are owner policy. |
| `Debug::PrintUsThemBlank` | Render two occupancy sets as friendly and opposing pieces with neutral empty squares. | Firm diagnostic role; overlap handling, orientation, and formatting are owner policy. |
| `Debug::PrintEncodedMovesMoveInfo` | Visualize the destinations represented by a precomputed move-information record. | Firm diagnostic role; empty-record and formatting behavior are owner policy. |
| `Debug::PrintU8BB` | Render an eight-bit occupancy slice and optionally mark one location. | Firm diagnostic role; invalid marker and formatting behavior are owner policy. |
| `Debug::PrintBoardGraphically` | Render a position's piece placement as a human-readable chess board. | Firm diagnostic role; symbols, orientation, and formatting are owner policy. |

## `src/Evaluate.hpp`

| Callable/function group | Brief intended purpose and semantic outcome | Boundary |
|---|---|---|
| `Eval::CountMaterial<Colour>` | Compute the material contribution owned by the selected colour from the current position. | Firm core; owner policy: piece weights, score range, and malformed-position behavior. |
| `Eval::UpdateData` | Refresh evaluation context derived from the position, including its broad game-phase classification. | Firm core; owner policy: phase definition, global visibility, reentrancy, and thread safety. |
| `Eval::Mobility<Colour>` | Compute the selected colour's mobility contribution from its available or attacked squares. | Firm core; owner policy: exact mobility definition, weights, pins, and phase treatment. |
| `Eval::PawnProgress<Colour>` | Compute a contribution rewarding the selected colour's pawn advancement. | Firm core; owner policy: advancement scale, exceptional placements, and score range. |
| `Eval::Evaluate` | Produce a relative heuristic score in which larger values favour the side to move, using the current position without changing its logical chess state. | Firm perspective and non-mutation intent; owner policy: components, weights, symmetry guarantees, valid range, and observable evaluation cache or phase state. |

## `src/MagicConstants.hpp`

The lookup representation in this file is an implementation facility. Its exact tables and encodings are not semantic API promises even when a function is callable at compile time.

| Callable/function group | Brief intended purpose and semantic outcome | Boundary |
|---|---|---|
| `Magics::SqToBB<Sq>` | Produce the single-square bitboard corresponding to a compile-time board square. | Firm core; owner policy: invalid template arguments. |
| `Magics::FileOf` | Return the file coordinate of a valid square index. | Firm core; owner policy: invalid input. |
| `Magics::RankOf` | Return the rank coordinate of a valid square index. | Firm core; owner policy: invalid input. |
| `Magics::EncodeKing` | Form an internal key from a king move's origin and destination for recognizing special king transitions. | Firm internal role; exact representation is deliberately non-contractual. |
| `Magics::GetLS1B` | Isolate the least-significant set bit of a bitboard. | Firm; zero-input outcome requires an approved primitive contract before assertion. |
| `Magics::FindLS1B` | Return the square index of the least-significant set bit. | Firm for nonzero input; owner policy: zero input. |
| `Magics::FindMS1B` | Return the square index of the most-significant set bit. | Firm for nonzero input; owner policy: zero input. |
| `Magics::PopCnt` | Count the set bits in a bitboard. | Firm. |
| `Magics::pow` | Evaluate a real base raised to a nonnegative integer exponent for compile-time lookup construction. | Firm core; owner policy: overflow, precision, and unusually large exponents. |
| `Magics::PopLS1B` | Return a bitboard with its least-significant set bit removed. | Firm for nonzero input; owner policy: zero input. |
| `Magics::PopMS1B` | Return a bitboard with its most-significant set bit removed. | Firm for nonzero input; owner policy: zero input. |
| `Magics::PopNRetLS1B` | Remove the least-significant set bit from a bitboard by reference and return that bit's square index. | Firm for nonzero input; owner policy: zero input. |
| `Magics::ValidSq` | Report whether an integer denotes a square on the chess board. | Firm. |
| `Magics::SqToBB(Sq)` | Produce the single-square bitboard corresponding to a runtime board square. | Firm core; owner policy: invalid input and checked-versus-release handling. |
| `Magics::TypeOf` | Decode the piece-type component of a valid encoded piece identity. | Firm core; owner policy: empty or invalid piece identities. |
| `Magics::ColourOf` | Decode the colour component of a valid nonempty encoded piece identity. | Firm core; owner policy: empty or invalid piece identities. |
| `Magics::BBFileOf` | Produce a compact one-hot indicator for a square's file coordinate. | Firm internal role; exact compact representation is non-contractual. |
| `Magics::BBRankOf` | Produce a compact one-hot indicator for a square's rank coordinate. | Firm internal role; exact compact representation is non-contractual. |
| `Magics::CollapsedFilesIndex` | Compress board occupancy into the file-oriented signature consumed by sliding lookup. | Firm internal role; exact encoding is non-contractual. |
| `Magics::CollapsedRanksIndex(BitBoard)` | Compress board occupancy into the rank-oriented signature consumed by sliding lookup. | Firm internal role; exact encoding is non-contractual. |
| `Magics::CollapsedRanksIndex(BitBoard, U8)` | Compress the occupancy of a selected file into the axis signature consumed by sliding lookup. | Firm internal role; exact encoding and invalid-file behavior are owner policy. |
| `Magics::Shift<MD>` | Shift occupied squares one supported chess direction while preventing wrap across board edges. | Firm for supported directions; owner policy: unsupported direction values. |
| `Magics::KnightAttackingMask` | Construct geometry-only knight destination masks for every square. | Firm semantic geometry; construction method and table representation are non-contractual. |
| `Magics::KingAttackingMask` | Construct geometry-only king destination masks for every square. | Firm semantic geometry; construction method and table representation are non-contractual. |
| `Magics::PrecomputeMask` | Construct unobstructed rank, file, and diagonal ray masks for sliding pieces on every square. | Firm semantic geometry; table layout is non-contractual. |
| `Magics::compute_base_2_to_3` | Construct the internal mapping from one side's compact occupancy to a sliding-lookup index contribution. | Firm internal role; mapping values and encoding are non-contractual. |
| `Magics::GetBaseThreeUsThem` | Combine friendly and opposing compact occupancies into the lookup index for one sliding-piece axis. | Firm internal role; exact index values and malformed overlapping occupancy behavior are non-contractual or owner policy. |

## `src/Move.hpp`

| Callable/function group | Brief intended purpose and semantic outcome | Boundary |
|---|---|---|
| `Moves::EncodeMove` | Combine a move's origin, destination, and move category into the engine's move identity. | Firm valid-input round-trip intent; exact bit layout and invalid-input handling are non-contractual or owner policy. |
| `Moves::DecodeMove` | Recover the origin, destination, and move category represented by a valid encoded move. | Firm valid-input round-trip intent; null pointers and invalid encodings are owner policy. |
| `Moves::StartSq` | Recover the origin square represented by a valid encoded move. | Firm core; invalid encodings are owner policy. |
| `Moves::TargetSq` | Recover the destination square represented by a valid encoded move. | Firm core; invalid encodings are owner policy. |
| `Moves::IsPromotionMove` | Report whether an encoded move carries a promotion category. | Firm for valid encodings; null or invalid representation is owner policy. |
| `Moves::PTypeOfProm` | Recover the promoted piece type from a valid promotion move. | Firm for promotion input; non-promotion and invalid input are owner policy. |

## `src/MoveGen.hpp`

Unless approved otherwise, generator order is deliberately unspecified. Whether a generator appends to or replaces a nonempty destination list, and its capacity preconditions, remain owner policy.

| Callable/function group | Brief intended purpose and semantic outcome | Boundary |
|---|---|---|
| `MoveGen::GenerateMovesFromBB` | Append one encoded move for each destination square represented by a bitboard, sharing the supplied origin and move category. | Firm core; ordering, capacity, and invalid-origin behavior are owner policy. |
| `MoveGen::GetMovesForSliding<AttackDirection>` | Retrieve precomputed sliding destinations and attack information for one piece square, axis, and friendly/opposing occupancy. | Firm core; exact lookup representation, pointer lifetime, overlapping occupancy, and invalid template or square inputs are owner policy. |
| `MoveGen::WhitePawnMoves` | Generate White's pseudo-legal pawn moves, including captures and applicable pawn-special moves. | Firm chess role; append/replace, ordering, malformed positions, and template/turn consistency are owner policy. |
| `MoveGen::BlackPawnMoves` | Generate Black's pseudo-legal pawn moves, including captures and applicable pawn-special moves. | Firm chess role; append/replace, ordering, malformed positions, and template/turn consistency are owner policy. |
| `MoveGen::BishopMoves<Colour>` | Generate pseudo-legal bishop moves for every bishop of the selected colour with blockers and captures respected. | Firm chess role; list policy and malformed-position behavior are owner policy. |
| `MoveGen::RookMoves<Colour>` | Generate pseudo-legal rook moves for every rook of the selected colour with blockers and captures respected. | Firm chess role; list policy and malformed-position behavior are owner policy. |
| `MoveGen::QueenMoves<Colour>` | Generate pseudo-legal queen moves for every queen of the selected colour with blockers and captures respected. | Firm chess role; list policy and malformed-position behavior are owner policy. |
| `MoveGen::KnightMoves<Colour>` | Generate pseudo-legal knight moves for every knight of the selected colour with board edges and occupancy respected. | Firm chess role; list policy and malformed-position behavior are owner policy. |
| `MoveGen::KingMoves<Colour>` | Generate pseudo-legal ordinary king moves for the selected colour, leaving king-safety filtering to the legal-move layer. | Firm chess role; missing or multiple kings and list policy are owner policy. |
| `MoveGen::PawnAttacks<Colour>` | Produce the pawn-controlled squares for the selected colour under the engine's attack-map convention. | Firm pawn geometry; owner policy: friendly-occupied defended squares and malformed placements. |
| `MoveGen::KnightAttacks<Colour>` | Produce the knight-controlled squares for the selected colour under the engine's attack-map convention. | Firm knight geometry; owner policy: friendly-occupied defended squares and pinned pieces. |
| `MoveGen::BishopAttacks<Colour>` | Produce bishop-controlled squares for the selected colour with sliding blockers respected. | Firm bishop geometry; owner policy: friendly blocker inclusion and pins. |
| `MoveGen::RookAttacks<Colour>` | Produce rook-controlled squares for the selected colour with sliding blockers respected. | Firm rook geometry; owner policy: friendly blocker inclusion and pins. |
| `MoveGen::QueenAttacks<Colour>` | Produce queen-controlled squares for the selected colour with sliding blockers respected. | Firm queen geometry; owner policy: friendly blocker inclusion and pins. |
| `MoveGen::KingAttacks<Colour>` | Produce ordinary king-controlled squares for the selected colour under the engine's attack-map convention. | Firm king geometry; owner policy: friendly occupancy, opposing king constraints, and malformed positions. |
| `MoveGen::InCheck<Colour>` | Report whether the selected colour's king is attacked in the supplied position. | Firm chess outcome for a searchable position; missing or multiple kings and malformed positions are owner policy. |
| `MoveGen::GenerateAllAttacks<Colour>` | Combine the selected colour's piece attack maps into one controlled-square set. | Firm aggregation role; the component attack convention remains owner policy. |
| `MoveGen::Castling<Colour>` | Add castling moves available to the selected colour when the chess prerequisites for that castle are satisfied. | Firm chess role; actual-piece requirements in malformed positions, list policy, and template/turn consistency are owner policy. |
| `MoveGen::GeneratePseudoLegalMoves<Colour>` | Generate the selected colour's complete movement-rule move set before rejecting moves that leave its king in check. | Firm chess distinction; list policy, ordering, duplicates, and malformed positions are owner policy. |
| `MoveGen::GenerateLegalMoves<Colour>` | Generate exactly the selected colour's moves that are legal in the supplied position while leaving the position logically unchanged. | Firm chess outcome and restoration intent; list policy, ordering, duplicates, and template/turn mismatch are owner policy. |

## `src/MoveList.hpp`

| Callable/function group | Brief intended purpose and semantic outcome | Boundary |
|---|---|---|
| `MoveList::MoveList` | Construct an empty bounded move sequence. | Firm. |
| `MoveList::add` | Append one encoded move to the logical sequence. | Firm core; duplicate and capacity-overflow policies are owner choice. |
| `MoveList::operator[]` | Return the move at a populated sequence index. | Firm for an in-range logical index; out-of-range behavior is owner policy. |
| `MoveList::merge` | Append all moves represented by a move-information record. | Firm core; capacity, overlap, and duplicate policies are owner choice. |
| `MoveList::all` | Expose the fixed backing storage for low-level internal processing. | Firm internal role; whether external mutation may change logical contents is owner policy. |
| `MoveList::len` | Report the number of populated moves in the logical sequence. | Firm. |
| `MoveList::contains` | Report whether a move is present among the logical populated entries. | Firm logical-list intent; treatment of unused storage and invalid/null moves is owner policy until approved. |

## `src/Position.hpp`

| Callable/function group | Brief intended purpose and semantic outcome | Boundary |
|---|---|---|
| `StateInfo::StateInfo` | Construct a baseline reversible-state record with no active special rights, capture, or cached identity. | Firm broad role; exact sentinel values are representation, not contract. |
| `Position::Position()` | Construct an initialized empty position ready to be reset, imported, or populated. | Firm broad role; exact clocks and whether an empty position is searchable are owner policy. |
| `Position::Position(std::string_view)` | Construct the complete position represented by a FEN record. | Firm valid-FEN outcome; acceptance strictness, invalid-input handling, and failure atomicity are owner policy. |
| `Position::Reset` | Clear piece placement, reversible state, side, clocks, cached identity, and accumulated undo history to the position baseline. | Firm broad restoration role; exact baseline clock convention is owner policy. |
| `Position::ImportFen` | Replace all logical position state from a FEN record and establish a coherent position identity. | Firm valid-FEN outcome; validation level, invalid-input response, and failure atomicity are owner policy. |
| `Position::MakeMove` | Apply one valid encoded chess move to placement, side to move, special rights, clocks, capture state, history, and cached position identity. | Firm transition role; whether input must be encoded, pseudo-legal, or fully legal and behavior on violation are owner policy. |
| `Position::UnmakeMove` | Reverse the immediately preceding matching move and restore the complete logical position that existed before it, including cached identity and reversible state. | Firm paired-restoration intent; empty history, mismatch, out-of-order use, and copy-history semantics are owner policy. |
| `Position::EmptySqs` | Return the board squares not occupied by either colour. | Firm. |
| `Position::EnPasBB` | Return the current en-passant target as a single-square bitboard, or an empty bitboard when no target exists. | Firm broad outcome; malformed target handling is owner policy. |
| `Position::EnPasSq` | Return the current en-passant target square or the engine's no-target sentinel. | Firm broad outcome; exact sentinel is internal representation. |
| `Position::CastlingRights` | Return the currently retained castling-right state. | Firm broad outcome; numeric encoding is internal representation. |
| `Position::ColourToMove` | Return the side whose turn it is. | Firm. |
| `Position::ZKey` | Return the position's currently cached Zobrist identity. | Firm broad outcome; initialization timing and stale-key behavior require the hash lifecycle policy. |
| `Position::HalfMoves` | Return the current reversible halfmove clock represented by the position. | Firm; width/overflow and invalid-FEN policy are owner choice. |
| `Position::FullMoves` | Return the current FEN fullmove number represented by the position. | Firm standard role; baseline, overflow, and invalid-FEN policy are owner choice. |
| `Position::IsOk` | Report whether the position satisfies the engine's intended internal consistency level. | Firm diagnostic role; owner policy: structural coherence versus full chess legality or searchability. |
| `Position::HashCurrentPostion` | Re-establish and return a deterministic cached identity for search-relevant state, consistent for the same logical position and across make/unmake restoration. | Firm consistency role; owner policy: query-versus-refresh mutation, inclusion of clocks or history, and collision guarantees. |
| `Position::UpdateCastlingRights` | Centralize adjustment of castling rights affected by a position transition. | Firm internal role; invocation point and malformed-state behavior are implementation or owner policy. |

## `src/Search.hpp`

| Callable/function group | Brief intended purpose and semantic outcome | Boundary |
|---|---|---|
| `Search::GoSearch` | Search legal continuations to the requested horizon within the supplied score window and time control, return a score from the current side's perspective, and restore the caller's position on every exit. | Firm search and restoration role; depth-zero meaning, terminal and mate scoring, timeout representation, transposition-table influence, draw scope, and null collaborators are owner policy. |
| `Search::FindBestMove` | Select a best legal root move under the supplied search and timing controls while leaving the caller's position unchanged. | Firm selection and restoration role; tie-breaking, no-legal-move representation, completed-iteration guarantee, progress output, timeout behavior, and deterministic depth control are owner policy. |

## `src/Testing.hpp`

These are legacy developer tools, not application-facing behavior. Their output becomes a contract only if the owner explicitly retains and specifies the tool.

| Callable/function group | Brief intended purpose and semantic outcome | Boundary |
|---|---|---|
| `PerftHandler::PerftHandler` | Construct an empty legal-move-tree counting session. | Firm. |
| `PerftHandler::ResetData` | Clear accumulated node totals and optional root-path report data for a fresh run. | Firm. |
| `PerftHandler::RunPerft<output_perft_paths>` | Count the legal move tree to the requested depth, optionally retain root splits, reset prior results, and restore the supplied position. | Firm valid-input role; negative depth, malformed positions, ordering, and report formatting are owner policy. |
| `PerftHandler::RunBulkPerft<output_perft_paths>` | Count the same legal move tree using the bulk-counting entry point, optionally retain root splits, reset prior results, and restore the supplied position. | Firm equivalence and restoration role; zero/negative depth, malformed positions, ordering, and report formatting are owner policy. |
| `PerftHandler::GetNodes` | Return the node total accumulated by the latest counting run. | Firm; value before a run is the baseline-session policy unless separately approved. |
| `PerftHandler::PrintData` | Print the accumulated total and any retained root split data for human diagnosis. | Firm diagnostic role; ordering and exact format are owner policy. |
| `PerftHandler::Perft` | Recursively count legal continuations for the ordinary perft path while restoring the position at each recursion level. | Firm internal role; depth domain and reporting details are owner policy. |
| `PerftHandler::BulkPerft` | Recursively count legal continuations for the bulk perft path while restoring the position at each recursion level. | Firm internal role; depth domain and reporting details are owner policy. |
| `TestSearch` | Exercise search on representative developer positions and report progress for manual inspection. | Firm diagnostic role; retained status, runtime, positions, and pass/fail semantics are owner policy. |
| `TestPerft<output_perft_paths>` | Time one ordinary perft case, compare its count with supplied reference data, and report developer-facing throughput and comparison status. | Firm diagnostic role; return semantics, zero-duration handling, output, and failure propagation are owner policy. |
| `TestBulkPerft<output_perft_paths>` | Time one bulk perft case, compare its count with supplied reference data, and report developer-facing throughput and comparison status. | Firm diagnostic role; return semantics, zero-duration handling, output, and failure propagation are owner policy. |
| `RunBenchmark<output_perft_paths>` | Run the built-in ordinary-perft workload and summarize its throughput. | Firm diagnostic role; workload, runtime, output, and failure propagation are owner policy. |
| `RunBulkBenchmark<output_perft_paths>` | Run the built-in bulk-perft workload and summarize its throughput. | Firm diagnostic role; workload, runtime, output, and failure propagation are owner policy. |
| legacy `Split` | Divide a perft-suite text record around a supplied delimiter for the legacy suite reader. | Firm helper role; empty fields, trailing remainder, empty delimiter, and malformed records are owner policy. |
| `RunPerftSuite<output_perft_paths>` | Read the external perft-suite records, run selected reference counts, and report whether the suite resource could be processed. | Firm diagnostic role; file location, grammar, case selection, count-failure propagation, and malformed-line recovery are owner policy. |

## `src/Timer.hpp`

| Callable/function group | Brief intended purpose and semantic outcome | Boundary |
|---|---|---|
| `Timer<T>::Timer()` | Start an RAII scope timer whose elapsed duration will be reported for human diagnosis when the scope ends. | Firm diagnostic role; output destination and format are owner policy. |
| `Timer<T>::Timer(U64*)` | Start an RAII scope timer that will publish the elapsed duration in the requested duration unit to caller-provided storage. | Firm valid-pointer role; null pointer, conversion overflow, and clock guarantees are owner policy. |
| `Timer<T>::~Timer` | Finish the measurement and publish it through the mode selected at construction. | Firm broad outcome; exception, monotonicity, precision, and output-format guarantees are owner policy. |
| `TimeManager::GetTimeAllowance` | Derive the search budget for one turn from the configured remaining time and increment. | Firm internal role; allocation formula, reserves, rounding, overflow, and zero behavior are owner policy. |
| `TimeManager::SetOptions` | Store the remaining-time and increment inputs that will govern the next search deadline. | Firm broad role; units, invalid values, and whether it changes an active deadline are owner policy. |
| `TimeManager::StartTiming` | Establish a deadline for the next search from the current clock and configured allowance. | Firm broad role; clock choice, repeated-start behavior, and overflow are owner policy. |
| `TimeManager::OutOfTime` | Report whether the established search deadline has expired. | Firm after `StartTiming`; initial state and equality-at-deadline are owner policy, and timing tests must not infer exact polling behavior. |

## `src/TranspositionTable.hpp`

| Callable/function group | Brief intended purpose and semantic outcome | Boundary |
|---|---|---|
| `TransposTable::TransposTable` | Construct a valid table owner with no allocated cache capacity. | Firm broad role; whether zero capacity accepts operations is owner policy. |
| `TransposTable::~TransposTable` | Release storage owned by the table. | Firm; copy/move ownership and outstanding probe-pointer behavior are owner policy. |
| `TransposTable::Resize` | Replace table storage with capacity derived from a requested memory budget and invalidate prior entries. | Firm broad role; units, rounding, minimum, maximum, zero, allocation failure, and strong-guarantee behavior are owner policy. |
| `TransposTable::Store` | Associate a position key with its search score, best move, depth, and bound information in the bounded cache. | Firm valid-table role; collisions, replacement, zero-capacity, concurrency, and draw-context safety are owner policy. |
| `TransposTable::Probe` | Return matching cached search information for a position key when present and otherwise report a miss. | Firm key-match role; miss representation, pointer lifetime, zero-capacity, and concurrency are owner policy. |
| `TransposTable::Clear` | Invalidate all entries while retaining the table's current capacity. | Firm broad role; zero-capacity and outstanding-pointer behavior are owner policy. |
| `TransposTable::GetNumElems` | Report the table's current entry capacity. | Firm; conversion from the requested memory budget remains owner policy. |

## `src/Types.hpp`

| Callable/function group | Brief intended purpose and semantic outcome | Boundary |
|---|---|---|
| `move_info::move_info` | Construct an empty bounded group of precomputed moves with no attack squares. | Firm broad role; exact capacity and representation are internal. |
| `move_info::add_move` | Append one encoded move to the precomputed move group. | Firm core; capacity overflow and duplicate policy are owner choice. |
| `operator!(Colour)` | Return the opposite chess colour. | Firm for valid colours; invalid enum values are owner policy. |
| `FloatDiv` | Produce a floating-point quotient from two numeric operands. | Firm core; zero divisor, precision, narrowing, and unsupported numeric types are owner policy. |

## `src/Uci.hpp`

Application-observable behavior in this section is governed only by approved UCI/application contract cards. Internal handler decomposition does not authorize stronger protocol behavior.

| Callable/function group | Brief intended purpose and semantic outcome | Boundary |
|---|---|---|
| `Uci::Uci` | Construct an engine command session with initialized position, search, timing, and transposition-table state. | Firm broad role; exact defaults, startup allocation failure, and whether construction emits output are owner policy. |
| `Uci::Loop` | Read and dispatch engine commands until the session is directed or required to terminate, preserving a usable process between supported commands. | Firm application role; EOF, framing, whitespace, casing, malformed or unknown input, concurrency, and diagnostic streams require approved application policy. |
| `Uci::HandleUci` | Identify the engine, advertise supported options, and complete the UCI initialization handshake. | Firm protocol role; exact identity/version, option set, ordering beyond the protocol, repeat-handshake behavior, and diagnostics require application policy. |
| `Uci::HandleIsReady` | Confirm that previously received commands have been processed without changing the logical chess position. | Firm protocol role; behavior during active search and any internal synchronization are application policy. |
| `Uci::HandleGo` | Apply supported search controls, run or start search for the current position, and ultimately emit one protocol-valid best-move response. | Firm protocol role; supported tokens, defaults, asynchronous stopability, malformed arguments, timing semantics, and no-move representation require application policy. |
| `Uci::HandlePosition` | Establish a position from `startpos` or FEN and apply a supplied UCI move history in order. | Firm protocol role for valid commands; legality validation, malformed input, failure atomicity, and recovery are application policy. |
| `Uci::HandleStop` | Request that an active search finish promptly and produce its best completed result. | Firm protocol role; idle-stop behavior, threading model, polling, and interaction with `quit` or EOF require application policy and testability support. |
| `Uci::HandleNewGame` | Discard engine state that must not leak between games and prepare for a new game. | Firm protocol role; whether position, options, clocks, and table capacity are reset or merely cleared requires application policy. |
| `Uci::HandleSetOption` | Apply a supported engine option to subsequent engine operation. | Firm protocol role; supported names, casing, range, units, malformed values, active-search behavior, and allocation failure require application policy. |
| `Uci::HandleBench` | Run an optional engine-specific benchmark or perft diagnostic requested through the command session. | Firm diagnostic role if retained; supported syntax, stdout/stderr use, runtime, interruption, and pass/fail behavior are owner policy. |
| `Uci::HandlePrint` | Run an optional engine-specific position diagnostic requested through the command session. | Firm diagnostic role if retained; supported syntax, format, streams, and protocol-cleanliness are owner policy. |

## `src/Util.hpp`

| Callable/function group | Brief intended purpose and semantic outcome | Boundary |
|---|---|---|
| `UTIL::Square` | Convert a valid internal square to its lowercase UCI coordinate. | Firm valid-input outcome; invalid-square handling is owner policy. |
| `UTIL::PromotionChar` | Convert a supported promotion piece type to its lowercase UCI suffix. | Firm valid-promotion outcome; non-promotion and invalid-category handling are owner policy. |
| `UTIL::MoveToStr` | Convert a valid encoded move to UCI long-algebraic notation, including a promotion suffix when applicable. | Firm valid-move outcome; null/no-move and invalid encoding behavior require application or move policy. |
| `UTIL::UciToMove` | Convert valid UCI long-algebraic move text in the context of a position into the engine's encoded move identity. | Firm valid-input conversion role; syntax validation, casing, null moves, diagnostics, and whether pseudo-legality or full legality is checked are owner policy. |

## `src/ZobristConstants.hpp`

The generated key values and generator sequence are internal implementation data. Only consistency properties may become contracts; exact key values must not be copied into blind tests unless the owner explicitly makes the hash serialization stable.

| Callable/function group | Brief intended purpose and semantic outcome | Boundary |
|---|---|---|
| `Zobrist::_::ZobRand64` | Advance deterministic generator state and produce the next pseudo-random key used to build Zobrist tables. | Firm internal role; algorithm, seed, and exact sequence are non-contractual. |
| `Zobrist::_::InitPieces` | Construct deterministic Zobrist keys for piece identities on board squares. | Firm internal role; table values and layout are non-contractual. |
| `Zobrist::_::InitEnPassant` | Construct deterministic Zobrist keys for en-passant target state. | Firm internal role; table values and layout are non-contractual. |
| `Zobrist::_::InitCastling` | Construct deterministic Zobrist keys for castling-right state. | Firm internal role; table values and layout are non-contractual. |
| `Zobrist::_::InitTmp` | Assemble the complete deterministic Zobrist key set, including side-to-move state, for immutable engine use. | Firm internal role; generator details and exact keys are non-contractual. |

## Blind-writer use

A blind writer may use this document to understand why a callable exists, but must obtain every asserted outcome from an approved contract card, an independent standard/oracle, or a separately approved metamorphic relation. Any row marked `Owner policy` is a coverage blocker rather than permission to infer current behavior from the implementation.
