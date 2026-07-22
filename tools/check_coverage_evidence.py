#!/usr/bin/env python3
"""Strictly validate TDFA's instrumented-test and line-coverage evidence."""

from __future__ import annotations

import argparse
from decimal import Decimal, InvalidOperation
import json
from pathlib import Path
import re
import sys
from typing import NoReturn


INTEGER_TEXT = re.compile(r"(?:0|[1-9][0-9]*)\r?\n?\Z")


class EvidenceError(ValueError):
    """Evidence is missing, malformed, internally impossible, or failing."""


def reject(message: str) -> NoReturn:
    raise EvidenceError(message)


def parse_test_result(text: str) -> int:
    if INTEGER_TEXT.fullmatch(text) is None:
        reject("instrumented test result must be one nonnegative decimal integer")
    return int(text.strip())


def require_json_integer(document: dict[str, object], name: str) -> int:
    if name not in document:
        reject(f"coverage summary is missing {name}")
    value = document[name]
    if isinstance(value, bool) or not isinstance(value, int):
        reject(f"coverage summary {name} must be an integer")
    if value < 0:
        reject(f"coverage summary {name} must be nonnegative")
    return value


def parse_line_counts(text: str) -> tuple[int, int]:
    try:
        document = json.loads(text)
    except (json.JSONDecodeError, UnicodeDecodeError) as error:
        reject(f"coverage summary is not valid JSON: {error}")
    if not isinstance(document, dict):
        reject("coverage summary root must be an object")
    covered = require_json_integer(document, "line_covered")
    total = require_json_integer(document, "line_total")
    if total == 0:
        reject("coverage summary line_total must be greater than zero")
    if covered > total:
        reject("coverage summary line_covered cannot exceed line_total")
    return covered, total


def parse_threshold(text: str) -> Decimal:
    try:
        threshold = Decimal(text)
    except InvalidOperation:
        reject(f"invalid line threshold: {text}")
    if not threshold.is_finite() or threshold < 0 or threshold > 100:
        reject("line threshold must be finite and between 0 and 100")
    return threshold


def assert_test_result(path: Path) -> None:
    if not path.is_file():
        reject(f"instrumented test result is missing: {path}")
    try:
        result = parse_test_result(path.read_text(encoding="ascii"))
    except (OSError, UnicodeError) as error:
        reject(f"cannot read instrumented test result {path}: {error}")
    if result != 0:
        reject(f"instrumented Fast+Deep+Audit matrix exited with code {result}")
    print("instrumented Fast+Deep+Audit matrix passed")


def assert_line_threshold(path: Path, threshold: Decimal) -> None:
    if not path.is_file():
        reject(f"coverage summary is missing: {path}")
    try:
        covered, total = parse_line_counts(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError) as error:
        reject(f"cannot read coverage summary {path}: {error}")
    exact = Decimal(covered) * Decimal(100) / Decimal(total)
    if Decimal(covered) * Decimal(100) < Decimal(total) * threshold:
        reject(
            f"line coverage {covered}/{total} = {exact:.4f}% is below {threshold}%"
        )
    print(f"line coverage gate passed: {covered}/{total} = {exact:.4f}% >= {threshold}%")


def self_test() -> None:
    assert parse_test_result("0\n") == 0
    assert parse_test_result("8\r\n") == 8
    for malformed in ("", "-1\n", "1.0\n", " 0\n", "0\nextra"):
        try:
            parse_test_result(malformed)
        except EvidenceError:
            pass
        else:
            raise AssertionError(f"accepted malformed test result: {malformed!r}")

    assert parse_line_counts('{"line_covered": 95, "line_total": 100}') == (95, 100)
    invalid_documents = (
        "{}",
        '{"line_covered": true, "line_total": 1}',
        '{"line_covered": 1.0, "line_total": 1}',
        '{"line_covered": 0, "line_total": 0}',
        '{"line_covered": 2, "line_total": 1}',
        "not-json",
    )
    for malformed in invalid_documents:
        try:
            parse_line_counts(malformed)
        except EvidenceError:
            pass
        else:
            raise AssertionError(f"accepted malformed coverage summary: {malformed!r}")
    assert Decimal(95) * 100 >= Decimal(100) * parse_threshold("95")
    assert Decimal(949) * 100 < Decimal(1000) * parse_threshold("95")
    print("coverage evidence validator self-test passed")


def main() -> int:
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command", required=True)

    test_parser = subparsers.add_parser("tests")
    test_parser.add_argument("path", type=Path)

    line_parser = subparsers.add_parser("lines")
    line_parser.add_argument("path", type=Path)
    line_parser.add_argument("--minimum", default="95")

    subparsers.add_parser("self-test")
    arguments = parser.parse_args()

    try:
        if arguments.command == "tests":
            assert_test_result(arguments.path)
        elif arguments.command == "lines":
            assert_line_threshold(arguments.path, parse_threshold(arguments.minimum))
        else:
            self_test()
    except EvidenceError as error:
        print(f"coverage evidence error: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
