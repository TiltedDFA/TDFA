#!/usr/bin/env python3
"""Run curated, snapshot-pinned TDFA mutants in an isolated temporary copy."""

from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time
import uuid
from dataclasses import dataclass, asdict


ROOT = Path(__file__).resolve().parents[1]
SNAPSHOT = "6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592"


@dataclass(frozen=True)
class Mutant:
    id: str
    path: str
    old: str
    new: str
    test_regex: str
    rationale: str


MUTANTS = (
    Mutant(
        "M-MOVE-TARGET-SHIFT", "src/Move.hpp",
        "move |= (target_index & START_SQ_MASK) << END_SQ_SHIFT;",
        "move |= (target_index & START_SQ_MASK) << (END_SQ_SHIFT - 1);",
        "T-MOV-001", "target field shifted into the wrong bits"),
    Mutant(
        "M-MOVE-DECODE-TYPE", "src/Move.hpp",
        "*move_type     = MoveType(move >> PIECE_TYPE_SHIFT);",
        "*move_type     = MoveType(move >> (PIECE_TYPE_SHIFT + 1));",
        "T-MOV-001", "decoded semantic move kind loses its low bit"),
    Mutant(
        "M-LEX-DROP-NINE", "src/BoardUtils.hpp",
        "constexpr bool IsDigit(const char i) {return i <= '9' && i >= '0';}",
        "constexpr bool IsDigit(const char i) {return i < '9' && i >= '0';}",
        "T-LEX-001", "ASCII nine is excluded from the FEN digit alphabet"),
    Mutant(
        "M-LEX-FIELD-BOUNDARY", "src/BoardUtils.hpp",
        "start = i + 1;",
        "start = i;",
        "T-LEX-002", "each FEN view retains the preceding separator"),
    Mutant(
        "M-BOARD-ADD-OMIT-ALL", "src/Board.hpp",
        "by_type_[pt_All]        |= SqToBB(s);",
        "by_type_[pt_All]        |= BitBoard{0};",
        "T-BRD-003|T-POS-002", "adding a piece omits aggregate occupancy"),
    Mutant(
        "M-BOARD-REMOVE-COLOUR", "src/Board.hpp",
        "by_colour_[ColourOf(p)] ^= SqToBB(s);",
        "by_colour_[ColourOf(p)] ^= BitBoard{0};",
        "T-BRD-005", "removal leaves a stale colour-occupancy bit"),
    Mutant(
        "M-BOARD-MOVE-TARGET", "src/Board.hpp",
        "board_[to]     = p;",
        "board_[to]     = p_None;",
        "T-BRD-005", "move clears the source without installing the target piece"),
    Mutant(
        "M-LIST-LENGTH", "src/MoveList.hpp",
        "constexpr void add(const Move m) noexcept  {data_[idx_++] = m;}",
        "constexpr void add(const Move m) noexcept  {data_[idx_] = m;}",
        "T-GEO-004", "MoveList writes a move without growing its logical prefix"),
    Mutant(
        "M-GEN-DROP-REMAINDER", "src/MoveGen.hpp",
        "b = Magics::PopLS1B(b);",
        "b = 0;",
        "T-GEO-004", "destination-bitboard emission stops after its first bit"),
    Mutant(
        "M-WHITE-PAWN-REVERSE", "src/MoveGen.cpp",
        "ml->add(Moves::EncodeMove(index - 8, index, mt_Quiet));",
        "ml->add(Moves::EncodeMove(index + 8, index, mt_Quiet));",
        "T-GEN-002", "ordinary White pawn source is reflected behind its target"),
)


def run(command: list[str], *, cwd: Path, env: dict[str, str], timeout: int = 240) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        command, cwd=cwd, env=env, text=True, encoding="utf-8", errors="replace",
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout, check=False)


def output_tail(output: str, lines: int = 80) -> str:
    return "\n".join(output.splitlines()[-lines:])


def ignored(_directory: str, names: list[str]) -> set[str]:
    result: set[str] = set()
    for name in names:
        if name in {".git", ".idea", ".agents", ".codex", "build", "__pycache__"}:
            result.add(name)
        elif name.startswith("cmake-build-") or name.endswith(".pyc"):
            result.add(name)
    return result


def find_catch_source() -> Path:
    candidates = (
        ROOT / "build/test-debug/_deps/catch2-src",
        ROOT / "build/test-release/_deps/catch2-src",
        ROOT / "build/coverage-gcc/_deps/catch2-src",
    )
    for candidate in candidates:
        if (candidate / "CMakeLists.txt").is_file():
            return candidate.resolve()
    raise RuntimeError("Catch2 source cache is absent; configure the test-debug preset once before mutation")


def mutate_once(path: Path, mutant: Mutant) -> str:
    original = path.read_text(encoding="utf-8")
    count = original.count(mutant.old)
    if count != 1:
        raise RuntimeError(f"{mutant.id}: expected one exact mutation site, found {count}")
    path.write_text(original.replace(mutant.old, mutant.new), encoding="utf-8")
    return original


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--mutant", action="append", default=[], help="run only this mutant id (repeatable)")
    parser.add_argument("--jobs", type=int, default=max(1, os.cpu_count() or 1))
    parser.add_argument("--keep-temp", action="store_true")
    parser.add_argument("--report", type=Path, default=ROOT / "build/mutation/report.json")
    args = parser.parse_args()

    selected = [m for m in MUTANTS if not args.mutant or m.id in args.mutant]
    unknown = set(args.mutant) - {m.id for m in MUTANTS}
    if unknown:
        parser.error("unknown mutant(s): " + ", ".join(sorted(unknown)))
    if not selected:
        parser.error("no mutants selected")

    manifest = run([sys.executable, "tools/generate_api_dossier.py", "--check"],
                   cwd=ROOT, env=os.environ.copy(), timeout=30)
    if manifest.returncode != 0:
        print(manifest.stdout, file=sys.stderr)
        raise RuntimeError("source snapshot/dossier check failed; mutation is pinned to the frozen snapshot")
    manifest_text = (ROOT / "tests/spec/source-snapshot.sha256").read_text(encoding="utf-8")
    dossier_text = (ROOT / "tests/spec/api-dossier.md").read_text(encoding="utf-8")
    manifest_match = re.search(r"^# Snapshot-SHA256: ([0-9a-f]{64})$", manifest_text, re.MULTILINE)
    dossier_match = re.search(r"^Snapshot: `([0-9a-f]{64})`", dossier_text, re.MULTILINE)
    if not manifest_match or not dossier_match:
        raise RuntimeError("frozen snapshot identifier is missing from the manifest or dossier")
    manifest_snapshot = manifest_match.group(1)
    dossier_snapshot = dossier_match.group(1)
    if manifest_snapshot != SNAPSHOT or dossier_snapshot != SNAPSHOT:
        raise RuntimeError(
            "mutation snapshot mismatch: "
            f"runner={SNAPSHOT} manifest={manifest_snapshot} dossier={dossier_snapshot}")

    catch_source = find_catch_source()
    temp_base = ROOT / "build/mutation/tmp"
    temp_base.mkdir(parents=True, exist_ok=True)
    temp_parent = temp_base / f"tdfa-mutation-{uuid.uuid4().hex}"
    temp_parent.mkdir(mode=0o777)
    temp_root = temp_parent / "TDFA"
    shutil.copytree(ROOT, temp_root, ignore=ignored)
    build_dir = temp_root / "mutation-build"
    env = os.environ.copy()
    runtime = Path("C:/msys64/ucrt64/bin")
    env["PATH"] = str(runtime) + os.pathsep + env.get("PATH", "")

    configure = run([
        "cmake", "-S", str(temp_root), "-B", str(build_dir), "-G", "MinGW Makefiles",
        "-DBUILD_TESTING=ON", "-DCMAKE_BUILD_TYPE=Debug", "-DTDFA_TEST_TOOLCHAIN=ON",
        "-DCMAKE_CXX_COMPILER=C:/msys64/ucrt64/bin/g++.exe",
        "-DCMAKE_MAKE_PROGRAM=C:/msys64/ucrt64/bin/mingw32-make.exe",
        f"-DFETCHCONTENT_SOURCE_DIR_CATCH2={catch_source.as_posix()}",
    ], cwd=temp_root, env=env)
    if configure.returncode != 0:
        print(output_tail(configure.stdout), file=sys.stderr)
        raise RuntimeError("isolated mutation configure failed")

    initial_build = run([
        "cmake", "--build", str(build_dir), "--target", "tdfa_tests", "--parallel", str(args.jobs)
    ], cwd=temp_root, env=env, timeout=600)
    if initial_build.returncode != 0:
        print(output_tail(initial_build.stdout), file=sys.stderr)
        raise RuntimeError("isolated baseline build failed")

    unique_filters = sorted({m.test_regex for m in selected})
    for test_filter in unique_filters:
        baseline = run([
            "ctest", "--test-dir", str(build_dir), "--output-on-failure",
            "--no-tests=error", "-R", test_filter,
        ], cwd=temp_root, env=env, timeout=180)
        if baseline.returncode != 0:
            print(output_tail(baseline.stdout), file=sys.stderr)
            raise RuntimeError(f"baseline filter fails before mutation: {test_filter}")

    results: list[dict[str, object]] = []
    start_time = time.monotonic()
    for ordinal, mutant in enumerate(selected, 1):
        print(f"[{ordinal}/{len(selected)}] {mutant.id}", flush=True)
        source = temp_root / mutant.path
        original = mutate_once(source, mutant)
        build = run([
            "cmake", "--build", str(build_dir), "--target", "tdfa_tests", "--parallel", str(args.jobs)
        ], cwd=temp_root, env=env, timeout=600)
        record: dict[str, object] = asdict(mutant)
        record["mutated_file_sha256"] = hashlib.sha256(source.read_bytes()).hexdigest()
        record["build_exit"] = build.returncode
        record["build_tail"] = output_tail(build.stdout)
        if build.returncode == 0:
            test = run([
                "ctest", "--test-dir", str(build_dir), "--output-on-failure",
                "--no-tests=error", "-R", mutant.test_regex,
            ], cwd=temp_root, env=env, timeout=180)
            record["test_exit"] = test.returncode
            record["test_tail"] = output_tail(test.stdout)
            record["status"] = "KILLED" if test.returncode != 0 else "SURVIVED"
        else:
            record["test_exit"] = None
            record["test_tail"] = ""
            record["status"] = "COMPILE_REJECTED"
        results.append(record)
        source.write_text(original, encoding="utf-8")

    elapsed = time.monotonic() - start_time
    counts = {status: sum(r["status"] == status for r in results)
              for status in ("KILLED", "SURVIVED", "COMPILE_REJECTED")}
    report = {
        "schema_version": 1,
        "snapshot": SNAPSHOT,
        "source_root": str(ROOT),
        "isolated": True,
        "private_access": False,
        "elapsed_seconds": round(elapsed, 3),
        "summary": counts,
        "mutants": results,
    }
    args.report = args.report.resolve()
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(counts, sort_keys=True))
    print(f"report: {args.report}")

    if args.keep_temp:
        kept = ROOT / "build/mutation/last-worktree"
        if kept.exists():
            raise RuntimeError(f"refusing to overwrite existing kept worktree: {kept}")
        shutil.copytree(temp_root, kept, ignore=ignored)
        print(f"kept isolated worktree: {kept}")
    resolved_temp = temp_parent.resolve()
    if resolved_temp.parent != temp_base.resolve() or not resolved_temp.name.startswith("tdfa-mutation-"):
        raise RuntimeError(f"refusing to clean unvalidated mutation path: {resolved_temp}")
    shutil.rmtree(resolved_temp)
    return 0 if counts["SURVIVED"] == 0 and counts["COMPILE_REJECTED"] == 0 else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (RuntimeError, subprocess.TimeoutExpired) as error:
        print(f"mutation runner error: {error}", file=sys.stderr)
        raise SystemExit(2)
