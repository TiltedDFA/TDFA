#!/usr/bin/env python3
"""Generate deterministic, offline chess oracle fixtures.

The generated TSV files are intentionally dependency-free at test runtime.  This
script is the only place where python-chess is needed; its exact version is pinned
in ``tools/requirements-oracle.txt``.
"""

from __future__ import annotations

import argparse
import hashlib
import sys
from pathlib import Path
from typing import Any, Iterable, Sequence


FORMAT_VERSION = "tdfa-chess-oracle-v1"
PINNED_CHESS_VERSION = "1.11.2"
DEFAULT_SEED = 0x54444641  # ASCII "TDFA"
DEFAULT_RANDOM_COUNT = 128
DEFAULT_MAX_GAME_PLIES = 160
DEFAULT_WALK_STEP_MAX = 8
MAX_PERFT_DEPTH = 3
COMMITTED_RANDOM_COUNT = 8
COMMITTED_MAX_GAME_PLIES = 96

REPOSITORY_ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT_DIR = REPOSITORY_ROOT / "tests" / "data"

CASE_COLUMNS = (
    "case_id",
    "source",
    "root_fen",
    "history_uci",
    "fen",
    "turn",
    "in_check",
    "is_checkmate",
    "is_stalemate",
    "is_insufficient_material",
    "is_seventyfive_moves",
    "is_threefold_repetition",
    "is_fivefold_repetition",
    "can_claim_fifty_moves",
    "can_claim_threefold_repetition",
    "game_over",
    "result",
    "termination",
    "winner",
    "legal_uci",
)

PERFT_COLUMNS = ("case_id", "fen", "depth", "nodes")


# The first six positions are the standard Chess Programming Wiki perft suite.
# The remaining cases isolate rules which are easy to miss in move generators.
CURATED_CASES = (
    (
        "start",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
    ),
    (
        "kiwipete",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
    ),
    (
        "perft_endgame",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
    ),
    (
        "perft_promotions",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
    ),
    (
        "perft_castling",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    ),
    (
        "perft_tactics",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    ),
    (
        "castling_minimal",
        "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1",
    ),
    (
        "en_passant_legal",
        "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2",
    ),
    (
        "en_passant_pinned",
        "k3r3/8/8/3pP3/8/8/8/4K3 w - d6 0 2",
    ),
    (
        "promotion_choices",
        "4k3/P7/8/8/8/8/7p/4K3 w - - 0 1",
    ),
    (
        "checkmate",
        "7k/6Q1/6K1/8/8/8/8/8 b - - 0 1",
    ),
    (
        "stalemate",
        "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1",
    ),
    (
        "insufficient_material",
        "7k/8/8/8/8/8/8/K7 w - - 0 1",
    ),
    (
        "seventyfive_move_rule",
        "7k/8/8/8/8/8/R7/K7 w - - 150 76",
    ),
    (
        "fifty_move_prospective",
        "7k/8/8/8/8/8/R7/K7 w - - 99 51",
    ),
    (
        "fifty_move_clock_100",
        "7k/8/8/8/8/8/R7/K7 w - - 100 51",
    ),
)

# These cases must retain their move stacks.  A FEN alone cannot reproduce
# repetition state, so every row stores both its root FEN and complete UCI replay.
_KNIGHT_CYCLE = ("g1f3", "g8f6", "f3g1", "f6g8")
HISTORY_CASES = (
    (
        "threefold_prospective",
        CURATED_CASES[0][1],
        _KNIGHT_CYCLE + ("g1f3", "g8f6", "f3g1"),
    ),
    (
        "threefold_current",
        CURATED_CASES[0][1],
        _KNIGHT_CYCLE * 2,
    ),
    (
        "fivefold_repetition",
        CURATED_CASES[0][1],
        _KNIGHT_CYCLE * 4,
    ),
)

HISTORY_EXPECTATIONS = {
    # is_threefold, is_fivefold, can_claim_threefold, game_over_without_claim
    "threefold_prospective": (False, False, True, False),
    "threefold_current": (True, False, True, False),
    "fivefold_repetition": (True, True, True, True),
}

CLOCK_EXPECTATIONS = {
    # halfmove_clock, can_claim_fifty_moves, is_seventyfive_moves
    "fifty_move_prospective": (99, True, False),
    "fifty_move_clock_100": (100, True, False),
    "seventyfive_move_rule": (150, True, True),
}

# Counts are embedded as independent, well-known sentinels.  The generator still
# computes every selected value with python-chess and refuses to emit a fixture if
# a sentinel differs.
PERFT_POSITIONS = (
    (
        "start",
        CURATED_CASES[0][1],
        (20, 400, 8902),
    ),
    (
        "kiwipete",
        CURATED_CASES[1][1],
        (48, 2039, 97862),
    ),
    (
        "perft_endgame",
        CURATED_CASES[2][1],
        (14, 191, 2812),
    ),
    (
        "perft_promotions",
        CURATED_CASES[3][1],
        (6, 264, 9467),
    ),
    (
        "perft_castling",
        CURATED_CASES[4][1],
        (44, 1486, 62379),
    ),
    (
        "perft_tactics",
        CURATED_CASES[5][1],
        (46, 2079, 89890),
    ),
)


class OracleError(RuntimeError):
    """A deterministic fixture could not be produced."""


class SplitMix64:
    """Small version-independent PRNG used only to select legal walk moves."""

    _MASK = (1 << 64) - 1

    def __init__(self, seed: int) -> None:
        self._state = seed & self._MASK

    def next_u64(self) -> int:
        self._state = (self._state + 0x9E3779B97F4A7C15) & self._MASK
        value = self._state
        value = ((value ^ (value >> 30)) * 0xBF58476D1CE4E5B9) & self._MASK
        value = ((value ^ (value >> 27)) * 0x94D049BB133111EB) & self._MASK
        return (value ^ (value >> 31)) & self._MASK

    def randbelow(self, upper_bound: int) -> int:
        if upper_bound <= 0:
            raise ValueError("upper_bound must be positive")
        limit = (1 << 64) - ((1 << 64) % upper_bound)
        while True:
            value = self.next_u64()
            if value < limit:
                return value % upper_bound


def load_chess() -> Any:
    try:
        import chess  # type: ignore[import-not-found]
    except ModuleNotFoundError as exc:
        raise OracleError(
            "python-chess is not installed; run "
            "`python -m pip install -r tools/requirements-oracle.txt`"
        ) from exc

    actual_version = getattr(chess, "__version__", "unknown")
    if actual_version != PINNED_CHESS_VERSION:
        raise OracleError(
            f"python-chess {PINNED_CHESS_VERSION} is required, found "
            f"{actual_version}; install tools/requirements-oracle.txt"
        )
    return chess


def bool_field(value: bool) -> str:
    return "1" if value else "0"


def canonical_fen(board: Any) -> str:
    # en_passant="fen" retains the FEN-standard target square even when no legal
    # en-passant capture exists.  That makes such edge cases observable.
    return board.fen(en_passant="fen")


def outcome_fields(board: Any) -> tuple[str, str, str]:
    outcome = board.outcome(claim_draw=False)
    if outcome is None:
        return "*", "-", "-"
    if outcome.winner is None:
        winner = "-"
    else:
        winner = "w" if outcome.winner else "b"
    return outcome.result(), outcome.termination.name.lower(), winner


def make_case_row(
    board: Any,
    case_id: str,
    source: str,
    root_fen: str,
    history: Sequence[str],
) -> tuple[str, ...]:
    result, termination, winner = outcome_fields(board)
    legal_uci = sorted(move.uci() for move in board.legal_moves)
    return (
        case_id,
        source,
        root_fen,
        " ".join(history) if history else "-",
        canonical_fen(board),
        "w" if board.turn else "b",
        bool_field(board.is_check()),
        bool_field(board.is_checkmate()),
        bool_field(board.is_stalemate()),
        bool_field(board.is_insufficient_material()),
        bool_field(board.is_seventyfive_moves()),
        bool_field(board.is_repetition(3)),
        bool_field(board.is_fivefold_repetition()),
        bool_field(board.can_claim_fifty_moves()),
        bool_field(board.can_claim_threefold_repetition()),
        bool_field(board.is_game_over(claim_draw=False)),
        result,
        termination,
        winner,
        " ".join(legal_uci) if legal_uci else "-",
    )


def validated_board(chess: Any, fen: str, case_id: str) -> Any:
    try:
        board = chess.Board(fen)
    except ValueError as exc:
        raise OracleError(f"invalid curated FEN for {case_id}: {exc}") from exc
    if not board.is_valid():
        raise OracleError(
            f"invalid curated board for {case_id}: status={board.status()}"
        )
    return board


def curated_rows(chess: Any) -> list[tuple[str, ...]]:
    rows: list[tuple[str, ...]] = []
    for case_id, fen in CURATED_CASES:
        board = validated_board(chess, fen, case_id)
        if case_id in CLOCK_EXPECTATIONS:
            expected = CLOCK_EXPECTATIONS[case_id]
            actual = (
                board.halfmove_clock,
                board.can_claim_fifty_moves(),
                board.is_seventyfive_moves(),
            )
            if actual != expected:
                raise OracleError(
                    f"clock sentinel mismatch for {case_id}: got {actual}, "
                    f"expected {expected}"
                )
        normalized_fen = canonical_fen(board)
        rows.append(
            make_case_row(
                board,
                case_id=case_id,
                source="curated",
                root_fen=normalized_fen,
                history=(),
            )
        )
    return rows


def history_rows(chess: Any) -> list[tuple[str, ...]]:
    rows: list[tuple[str, ...]] = []
    for case_id, root, history in HISTORY_CASES:
        board = validated_board(chess, root, case_id)
        normalized_root = canonical_fen(board)
        for ply, uci in enumerate(history, start=1):
            try:
                move = board.parse_uci(uci)
            except ValueError as exc:
                raise OracleError(
                    f"illegal history move for {case_id} at ply {ply}: {uci}"
                ) from exc
            board.push(move)
        expected = HISTORY_EXPECTATIONS[case_id]
        actual = (
            board.is_repetition(3),
            board.is_fivefold_repetition(),
            board.can_claim_threefold_repetition(),
            board.is_game_over(claim_draw=False),
        )
        if actual != expected:
            raise OracleError(
                f"repetition sentinel mismatch for {case_id}: got {actual}, "
                f"expected {expected}"
            )
        rows.append(
            make_case_row(
                board,
                case_id=case_id,
                source="history",
                root_fen=normalized_root,
                history=history,
            )
        )
    return rows


def random_walk_rows(
    chess: Any,
    count: int,
    seed: int,
    max_game_plies: int,
    walk_step_max: int,
    already_seen: Iterable[str],
) -> list[tuple[str, ...]]:
    if count == 0:
        return []

    rng = SplitMix64(seed)
    board = chess.Board()
    root_fen = canonical_fen(board)
    history: list[str] = []
    seen = set(already_seen)
    rows: list[tuple[str, ...]] = []
    attempts = 0
    attempt_limit = max(1000, count * 50)

    while len(rows) < count:
        attempts += 1
        if attempts > attempt_limit:
            raise OracleError(
                f"could not produce {count} distinct random positions after "
                f"{attempt_limit} walk samples"
            )

        if board.is_game_over(claim_draw=False) or len(history) >= max_game_plies:
            board = chess.Board()
            history = []

        remaining = max_game_plies - len(history)
        step_count = 1 + rng.randbelow(min(walk_step_max, remaining))
        for _ in range(step_count):
            moves = sorted(board.legal_moves, key=lambda move: move.uci())
            if not moves:
                break
            move = moves[rng.randbelow(len(moves))]
            history.append(move.uci())
            board.push(move)
            if board.is_game_over(claim_draw=False):
                break

        fen = canonical_fen(board)
        if fen in seen:
            continue
        seen.add(fen)
        rows.append(
            make_case_row(
                board,
                case_id=f"random_{len(rows):06d}",
                source="random_walk",
                root_fen=root_fen,
                history=history,
            )
        )

    return rows


def perft(board: Any, depth: int) -> int:
    if depth == 0:
        return 1
    if depth == 1:
        return board.legal_moves.count()

    nodes = 0
    for move in list(board.legal_moves):
        board.push(move)
        nodes += perft(board, depth - 1)
        board.pop()
    return nodes


def make_perft_rows(chess: Any, max_depth: int) -> list[tuple[str, ...]]:
    rows: list[tuple[str, ...]] = []
    for case_id, fen, sentinels in PERFT_POSITIONS:
        board = validated_board(chess, fen, case_id)
        normalized_fen = canonical_fen(board)
        for depth in range(1, max_depth + 1):
            nodes = perft(board, depth)
            expected = sentinels[depth - 1]
            if nodes != expected:
                raise OracleError(
                    f"perft sentinel mismatch for {case_id} depth {depth}: "
                    f"python-chess returned {nodes}, expected {expected}"
                )
            rows.append((case_id, normalized_fen, str(depth), str(nodes)))
    return rows


def validate_tsv_fields(columns: Sequence[str], rows: Sequence[Sequence[str]]) -> None:
    for row_index, row in enumerate(rows, start=1):
        if len(row) != len(columns):
            raise OracleError(
                f"row {row_index} has {len(row)} fields; expected {len(columns)}"
            )
        for value in row:
            if "\t" in value or "\n" in value or "\r" in value:
                raise OracleError(f"row {row_index} contains a non-TSV-safe field")


def render_document(
    columns: Sequence[str],
    rows: Sequence[Sequence[str]],
    metadata: Sequence[tuple[str, str]],
) -> tuple[bytes, str]:
    validate_tsv_fields(columns, rows)
    body_lines = ["\t".join(columns)]
    body_lines.extend("\t".join(row) for row in rows)
    body = ("\n".join(body_lines) + "\n").encode("utf-8")
    digest = hashlib.sha256(body).hexdigest()

    header_lines = [
        f"# format={FORMAT_VERSION}",
        "# encoding=utf-8",
        "# delimiter=TAB",
        "# empty=-",
        "# checksum_scope=column-header-and-data-rows-with-lf",
    ]
    header_lines.extend(f"# {key}={value}" for key, value in metadata)
    header_lines.append(f"# data_sha256={digest}")
    header = ("\n".join(header_lines) + "\n").encode("utf-8")
    return header + body, digest


def parse_nonnegative(value: str) -> int:
    parsed = int(value, 10)
    if parsed < 0:
        raise argparse.ArgumentTypeError("must be non-negative")
    return parsed


def parse_positive(value: str) -> int:
    parsed = int(value, 10)
    if parsed <= 0:
        raise argparse.ArgumentTypeError("must be positive")
    return parsed


def parse_seed(value: str) -> int:
    try:
        return int(value, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(
            "must be an integer (decimal or 0x-prefixed hexadecimal)"
        ) from exc


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--profile",
        choices=("custom", "committed"),
        default="custom",
        help=(
            "generation profile; 'committed' fixes every corpus parameter and "
            "rejects overrides (default: custom)"
        ),
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=DEFAULT_OUTPUT_DIR,
        help=f"fixture directory (default: {DEFAULT_OUTPUT_DIR})",
    )
    parser.add_argument(
        "--count",
        type=parse_nonnegative,
        default=None,
        help="number of distinct deterministic random-walk cases (supports 10000)",
    )
    parser.add_argument(
        "--seed",
        type=parse_seed,
        default=None,
        help="random-walk seed; decimal or 0x-prefixed (default: 0x54444641)",
    )
    parser.add_argument(
        "--max-game-plies",
        type=parse_positive,
        default=None,
        help="reset each legal walk after this many plies",
    )
    parser.add_argument(
        "--walk-step-max",
        type=parse_positive,
        default=None,
        help="maximum plies advanced between sampled positions",
    )
    parser.add_argument(
        "--perft-depth",
        type=int,
        choices=range(0, MAX_PERFT_DEPTH + 1),
        default=None,
        metavar="{0,1,2,3}",
        help="maximum selected perft depth (default: 3)",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="verify that existing fixtures exactly match instead of writing",
    )
    return parser


def resolve_profile(
    parser: argparse.ArgumentParser, args: argparse.Namespace
) -> argparse.Namespace:
    parameter_names = (
        "count",
        "seed",
        "max_game_plies",
        "walk_step_max",
        "perft_depth",
    )
    if args.profile == "committed":
        overridden = [name for name in parameter_names if getattr(args, name) is not None]
        if overridden:
            formatted = ", ".join("--" + name.replace("_", "-") for name in overridden)
            parser.error(f"--profile committed does not accept overrides: {formatted}")
        args.count = COMMITTED_RANDOM_COUNT
        args.seed = DEFAULT_SEED
        args.max_game_plies = COMMITTED_MAX_GAME_PLIES
        args.walk_step_max = DEFAULT_WALK_STEP_MAX
        args.perft_depth = MAX_PERFT_DEPTH
        args.generation_args = "--profile committed"
        return args

    args.count = DEFAULT_RANDOM_COUNT if args.count is None else args.count
    args.seed = DEFAULT_SEED if args.seed is None else args.seed
    args.max_game_plies = (
        DEFAULT_MAX_GAME_PLIES
        if args.max_game_plies is None
        else args.max_game_plies
    )
    args.walk_step_max = (
        DEFAULT_WALK_STEP_MAX if args.walk_step_max is None else args.walk_step_max
    )
    args.perft_depth = (
        MAX_PERFT_DEPTH if args.perft_depth is None else args.perft_depth
    )
    args.generation_args = (
        f"--profile custom --count {args.count} --seed "
        f"0x{args.seed & SplitMix64._MASK:016x} "
        f"--max-game-plies {args.max_game_plies} "
        f"--walk-step-max {args.walk_step_max} "
        f"--perft-depth {args.perft_depth}"
    )
    return args


def produce_documents(chess: Any, args: argparse.Namespace) -> dict[str, bytes]:
    fixed_rows = curated_rows(chess) + history_rows(chess)
    random_rows = random_walk_rows(
        chess,
        count=args.count,
        seed=args.seed,
        max_game_plies=args.max_game_plies,
        walk_step_max=args.walk_step_max,
        already_seen=(row[CASE_COLUMNS.index("fen")] for row in fixed_rows),
    )
    case_rows = fixed_rows + random_rows

    common_metadata = (
        ("generator", "tools/generate_oracle_cases.py"),
        ("python_chess", PINNED_CHESS_VERSION),
        ("profile", args.profile),
        ("generation_args", args.generation_args),
        ("seed", f"0x{args.seed & SplitMix64._MASK:016x}"),
        ("prng", "splitmix64-v1"),
    )
    cases_document, _ = render_document(
        CASE_COLUMNS,
        case_rows,
        common_metadata
        + (
            ("curated_case_count", str(len(CURATED_CASES))),
            ("history_case_count", str(len(HISTORY_CASES))),
            ("random_case_count", str(len(random_rows))),
            ("case_count", str(len(case_rows))),
            ("max_game_plies", str(args.max_game_plies)),
            ("walk_step_max", str(args.walk_step_max)),
        ),
    )

    perft_rows = make_perft_rows(chess, args.perft_depth)
    perft_document, _ = render_document(
        PERFT_COLUMNS,
        perft_rows,
        common_metadata
        + (
            ("position_count", str(len(PERFT_POSITIONS))),
            ("max_depth", str(args.perft_depth)),
            ("row_count", str(len(perft_rows))),
        ),
    )
    return {
        "oracle_cases.tsv": cases_document,
        "oracle_perft.tsv": perft_document,
    }


def check_documents(output_dir: Path, documents: dict[str, bytes]) -> bool:
    matched = True
    for filename, expected in documents.items():
        path = output_dir / filename
        try:
            actual = path.read_bytes()
        except FileNotFoundError:
            print(f"missing fixture: {path}", file=sys.stderr)
            matched = False
            continue
        if actual != expected:
            print(f"stale fixture: {path}", file=sys.stderr)
            matched = False
    return matched


def write_documents(output_dir: Path, documents: dict[str, bytes]) -> None:
    output_dir.mkdir(parents=True, exist_ok=True)
    for filename, contents in documents.items():
        path = output_dir / filename
        path.write_bytes(contents)
        print(f"wrote {path} ({len(contents)} bytes)")


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    args = resolve_profile(parser, parser.parse_args(argv))
    try:
        chess = load_chess()
        documents = produce_documents(chess, args)
    except OracleError as exc:
        print(f"oracle generation failed: {exc}", file=sys.stderr)
        return 2

    if args.check:
        if check_documents(args.output_dir, documents):
            print("oracle fixtures are deterministic and up to date")
            return 0
        return 1

    write_documents(args.output_dir, documents)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
