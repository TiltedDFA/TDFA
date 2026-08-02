#!/usr/bin/env python3
"""Render TDFA's CTest/JUnit evidence as a compact terminal dashboard.

The renderer is deliberately dependency-free.  JUnit and the immutable raw
logs remain the evidence of record; this file is the human-facing presentation
layer over that evidence.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import platform
import re
import shutil
import subprocess
import sys
import textwrap
import unicodedata
import xml.etree.ElementTree as ET
from collections import OrderedDict
from dataclasses import asdict, dataclass, field
from datetime import datetime
from pathlib import Path
from typing import Any, Iterable, Sequence


ANSI_RE = re.compile(r"\x1b\[[0-9;]*m")
TEST_ID_RE = re.compile(r"\b(T-[A-Z0-9]+(?:-[A-Z0-9]+)*(?:\.[A-Za-z0-9-]+)?)\b")
SOURCE_RE = re.compile(
    r"(?m)^((?:[A-Za-z]:)?[/\\][^\r\n]+?\.(?:cpp|cxx|cc|hpp|hxx|h)):(\d+): FAILED:"
)


@dataclass
class TestCase:
    lane: str
    name: str
    status: str
    duration: float
    output: str = ""
    labels: tuple[str, ...] = ()
    source_junit: str = ""
    failure_message: str = ""
    xml_status: str = ""

    @property
    def test_id(self) -> str:
        match = TEST_ID_RE.search(self.name)
        return match.group(1) if match else self.name.split(" ", 1)[0]

    @property
    def short_name(self) -> str:
        return self.name.split("::", 1)[-1]


@dataclass
class FixtureIssue:
    path: str
    cr_count: int
    lf_count: int
    raw_sha256: str
    lf_sha256: str


@dataclass
class Issue:
    key: str
    code: str
    severity: str
    title: str
    cases: list[TestCase] = field(default_factory=list)


@dataclass
class ParsedEvidence:
    cases: list[TestCase] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)
    timestamps: list[str] = field(default_factory=list)


class Theme:
    STYLES = {
        "bold": "1",
        "dim": "2",
        "red": "91",
        "green": "92",
        "yellow": "93",
        "blue": "94",
        "magenta": "95",
        "cyan": "96",
        "white": "97",
    }

    def __init__(self, colour: bool, unicode: bool):
        self.colour = colour
        self.unicode = unicode
        if unicode:
            self.chars = {
                "tl": "╭",
                "tr": "╮",
                "bl": "╰",
                "br": "╯",
                "v": "│",
                "h": "─",
                "pass": "✓",
                "fail": "✕",
                "blocked": "◆",
                "infra": "▲",
                "skip": "○",
                "running": "→",
                "bar": "█",
                "bar_empty": "░",
            }
        else:
            self.chars = {
                "tl": "+",
                "tr": "+",
                "bl": "+",
                "br": "+",
                "v": "|",
                "h": "-",
                "pass": "+",
                "fail": "x",
                "blocked": "!",
                "infra": "^",
                "skip": "o",
                "running": ">",
                "bar": "#",
                "bar_empty": ".",
            }

    def paint(self, text: str, *styles: str) -> str:
        if not self.colour or not text:
            return text
        codes = [self.STYLES[style] for style in styles if style in self.STYLES]
        return f"\x1b[{';'.join(codes)}m{text}\x1b[0m" if codes else text


def _local_name(tag: str) -> str:
    return tag.rsplit("}", 1)[-1]


def _children(element: ET.Element, name: str) -> Iterable[ET.Element]:
    return (child for child in element if _local_name(child.tag) == name)


def _first_child(element: ET.Element, name: str) -> ET.Element | None:
    return next(_children(element, name), None)


def _float(value: str | None) -> float:
    try:
        parsed = float(value or "0")
    except ValueError:
        return 0.0
    return parsed if math.isfinite(parsed) and parsed >= 0 else 0.0


def parse_junit_spec(specification: str) -> tuple[str, Path]:
    if "=" in specification:
        lane, raw_path = specification.split("=", 1)
        if lane.strip() and raw_path.strip():
            return lane.strip(), Path(raw_path)
    path = Path(specification)
    return path.stem.replace("_", "-"), path


def parse_junit(specification: str) -> ParsedEvidence:
    lane_override, path = parse_junit_spec(specification)
    evidence = ParsedEvidence()
    if not path.is_file():
        evidence.warnings.append(f"JUnit evidence is missing: {path}")
        return evidence

    try:
        document = ET.parse(path)
    except (ET.ParseError, OSError) as error:
        evidence.warnings.append(f"Cannot read JUnit evidence {path}: {error}")
        return evidence

    root = document.getroot()
    suites = (
        [root]
        if _local_name(root.tag) == "testsuite"
        else [
            element
            for element in root.iter()
            if _local_name(element.tag) == "testsuite"
        ]
    )
    for suite in suites:
        timestamp = suite.get("timestamp")
        if timestamp:
            evidence.timestamps.append(timestamp)
        if suite.get("tests") == "0":
            evidence.warnings.append(f"JUnit evidence contains zero tests: {path}")

    testcases = [
        element for element in root.iter() if _local_name(element.tag) == "testcase"
    ]
    if not testcases and not any(
        "zero tests" in warning for warning in evidence.warnings
    ):
        evidence.warnings.append(f"JUnit evidence contains zero tests: {path}")

    for testcase in testcases:
        name = testcase.get("name", "<unnamed test>")
        duration = _float(testcase.get("time"))
        failure = _first_child(testcase, "failure")
        error = _first_child(testcase, "error")
        skipped = _first_child(testcase, "skipped")
        xml_status = (testcase.get("status") or "").lower()
        failure_message = ""
        if failure is not None:
            failure_message = failure.get("message") or ""
        elif error is not None:
            failure_message = error.get("message") or ""
        skipped_message = skipped.get("message") if skipped is not None else ""
        skipped_message = skipped_message or ""
        if failure is not None or error is not None or xml_status in {"fail", "failed"}:
            status = "fail"
        elif "fixture dependency failed" in skipped_message.lower():
            status = "blocked"
        elif "unable to find executable" in skipped_message.lower():
            status = "infra"
        elif xml_status == "disabled":
            status = "disabled"
        elif skipped is not None or xml_status in {"notrun", "skip", "skipped"}:
            status = "skip"
        else:
            status = "pass"

        labels: list[str] = []
        properties = _first_child(testcase, "properties")
        if properties is not None:
            for prop in _children(properties, "property"):
                if prop.get("name") == "cmake_labels":
                    labels.extend(
                        value for value in (prop.get("value") or "").split(";") if value
                    )

        chunks: list[str] = []
        for child_name in ("system-out", "system-err"):
            child = _first_child(testcase, child_name)
            if child is not None and child.text:
                chunks.append(child.text)
        if failure is not None and failure.text:
            chunks.append(failure.text)
        if error is not None and error.text:
            chunks.append(error.text)
        if failure_message and failure_message.lower() != "failed":
            chunks.append(f"CTest failure: {failure_message}")
        if skipped_message:
            chunks.append(f"CTest skipped: {skipped_message}")

        evidence.cases.append(
            TestCase(
                lane=lane_override,
                name=name,
                status=status,
                duration=duration,
                output="\n".join(chunks).strip(),
                labels=tuple(labels),
                source_junit=str(path),
                failure_message=failure_message or skipped_message,
                xml_status=xml_status,
            )
        )
    return evidence


def load_summary(path: Path | None) -> tuple[dict[str, Any] | None, list[str]]:
    if path is None:
        return None, []
    if not path.is_file():
        return None, [f"Verification summary is missing: {path}"]
    try:
        value = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        return None, [f"Cannot read verification summary {path}: {error}"]
    if not isinstance(value, dict):
        return None, [f"Verification summary root is not an object: {path}"]
    return value, []


def discover_fixture_issues(project_root: Path) -> list[FixtureIssue]:
    fixture_root = project_root / "tests" / "data"
    issues: list[FixtureIssue] = []
    if not fixture_root.is_dir():
        return issues
    for path in sorted(fixture_root.glob("*.tsv")):
        try:
            payload = path.read_bytes()
        except OSError:
            continue
        cr_count = payload.count(b"\r")
        if cr_count == 0:
            continue
        normalized = payload.replace(b"\r\n", b"\n").replace(b"\r", b"\n")
        issues.append(
            FixtureIssue(
                path=path.relative_to(project_root).as_posix(),
                cr_count=cr_count,
                lf_count=payload.count(b"\n"),
                raw_sha256=hashlib.sha256(payload).hexdigest(),
                lf_sha256=hashlib.sha256(normalized).hexdigest(),
            )
        )
    return issues


def _generic_title(case: TestCase) -> str:
    title = case.short_name
    if title.startswith(case.test_id):
        title = title[len(case.test_id) :].strip(" .:-")
    return title or case.test_id


def _fixture_eol_is_evidenced(
    output: str, fixture_issues: Sequence[FixtureIssue]
) -> bool:
    if "TSV contains CR" in output:
        return True

    normalized_output = output.replace("\\", "/")
    for fixture in fixture_issues:
        if fixture.path in normalized_output:
            return True
        if (
            fixture.raw_sha256 in normalized_output
            and fixture.lf_sha256 in normalized_output
        ):
            return True
    return False


def classify_failure(case: TestCase, fixture_issues: Sequence[FixtureIssue]) -> Issue:
    output = case.output
    ctest_outcome = case.failure_message.strip().upper().replace(" ", "_")
    fixture_patterns = (
        "TSV contains CR",
        "fixture scoped bytes do not match data_sha256",
        "sha256_hex(whole_file) == file_hash",
    )
    if any(pattern in output for pattern in fixture_patterns):
        eol_evidenced = _fixture_eol_is_evidenced(output, fixture_issues)
        code = "FIXTURE_EOL" if eol_evidenced else "FIXTURE_INTEGRITY"
        return Issue(
            key=code,
            code=code,
            severity="blocked" if eol_evidenced else "fail",
            title=(
                "Canonical fixture line endings"
                if eol_evidenced
                else "Canonical fixture integrity"
            ),
        )

    if "PieceType=" in output and re.search(r"expected=\S+\s+actual=\S+", output):
        return Issue(
            key="PROMOTION_MAPPING",
            code="VALUE_MISMATCH",
            severity="fail",
            title="PromotionChar mapping",
        )

    if "nonzero bit query/removal disagrees" in output or (
        "expected_first=" in output and "FindMS1B=" in output
    ):
        return Issue(
            key="BIT_SCAN_MODEL",
            code="VALUE_MISMATCH",
            severity="fail",
            title="Bit-scan ordered-set model",
        )

    if "response deadline expired" in output or (
        "outcome=timeout" in output and case.test_id.startswith("T-APP")
    ):
        return Issue(
            key=f"UCI_TIMEOUT:{case.test_id.split('.', 1)[0]}",
            code="UCI_TIMEOUT",
            severity="fail",
            title=f"{case.test_id.split('.', 1)[0]} protocol variants",
        )

    lowered = f"{case.failure_message}\n{output}".lower()
    if (
        "addresssanitizer" in lowered
        or "undefinedbehaviorsanitizer" in lowered
        or "runtime error:" in lowered
    ):
        return Issue(
            key=f"SANITIZER:{case.test_id}",
            code="SANITIZER",
            severity="fail",
            title=_generic_title(case),
        )
    if ctest_outcome in {"BAD_COMMAND", "FAILED_TO_START", "NOT_RUN"} or any(
        marker in lowered
        for marker in ("bad_command", "failed to start", "unable to find executable")
    ):
        return Issue(
            key=f"CTEST_INFRA:{case.test_id}",
            code="CTEST_INFRA",
            severity="infra",
            title="CTest could not launch the test process",
        )
    if ctest_outcome in {
        "SEGFAULT",
        "ILLEGAL",
        "INTERRUPT",
        "NUMERICAL",
        "OTHER_FAULT",
        "CHILD_ABORTED",
    } or any(
        marker in lowered
        for marker in (
            "segfault",
            "segmentation fault",
            "illegal instruction",
            "child aborted",
            "subprocess aborted",
            "access violation",
            "numerical exception",
            "other_fault",
        )
    ):
        return Issue(
            key=f"PROCESS_CRASH:{case.test_id}",
            code="PROCESS_CRASH",
            severity="fail",
            title="Test process crashed",
        )
    if "timeout" in lowered or "deadline expired" in lowered:
        return Issue(
            key=f"TIMEOUT:{case.test_id}",
            code="TIMEOUT",
            severity="fail",
            title=_generic_title(case),
        )
    if "unexpected exception" in lowered or "exception with message" in lowered:
        return Issue(
            key=f"EXCEPTION:{case.test_id}",
            code="EXCEPTION",
            severity="fail",
            title=_generic_title(case),
        )
    return Issue(
        key=f"ASSERTION:{case.test_id}",
        code="ASSERTION",
        severity="fail",
        title=_generic_title(case),
    )


def group_issues(
    cases: Sequence[TestCase], fixture_issues: Sequence[FixtureIssue]
) -> list[Issue]:
    grouped: OrderedDict[str, Issue] = OrderedDict()
    for case in cases:
        if case.status == "blocked":
            classified = Issue(
                key="CTEST_FIXTURE_DEPENDENCY",
                code="FIXTURE_DEPENDENCY",
                severity="blocked",
                title="CTest fixture dependency",
            )
        elif case.status == "infra":
            classified = Issue(
                key=f"INFRA:{case.test_id}",
                code="INFRA",
                severity="infra",
                title=_generic_title(case),
            )
        elif case.status == "fail":
            classified = classify_failure(case, fixture_issues)
        else:
            continue
        issue = grouped.get(classified.key)
        if issue is None:
            issue = classified
            grouped[classified.key] = issue
        issue.cases.append(case)

    severity_order = {"fail": 0, "blocked": 1, "infra": 2}
    return sorted(
        grouped.values(),
        key=lambda issue: (
            severity_order.get(issue.severity, 9),
            -len(issue.cases),
            issue.code,
            issue.title,
        ),
    )


def visible_length(text: str) -> int:
    width = 0
    for character in ANSI_RE.sub("", text):
        if unicodedata.combining(character):
            continue
        width += 2 if unicodedata.east_asian_width(character) in {"W", "F"} else 1
    return width


def escape_inline(text: str) -> str:
    """Keep evidence from injecting terminal controls or breaking a table row."""
    escaped: list[str] = []
    for character in text:
        codepoint = ord(character)
        if character == "\t":
            escaped.append(r"\t")
        elif character == "\r":
            escaped.append(r"\r")
        elif character == "\n":
            escaped.append(r"\n")
        elif codepoint == 0x1B:
            escaped.append(r"\x1b")
        elif codepoint < 0x20 or codepoint == 0x7F:
            escaped.append(f"\\x{codepoint:02x}")
        elif not character.isprintable():
            if codepoint <= 0xFF:
                escaped.append(f"\\x{codepoint:02x}")
            elif codepoint <= 0xFFFF:
                escaped.append(f"\\u{codepoint:04x}")
            else:
                escaped.append(f"\\U{codepoint:08x}")
        else:
            escaped.append(character)
    return "".join(escaped)


def truncate(text: str, width: int, theme: Theme) -> str:
    if width <= 0:
        return ""
    if visible_length(text) <= width:
        return text
    plain = ANSI_RE.sub("", text)
    ellipsis = "…" if theme.unicode else "..."
    ellipsis_width = visible_length(ellipsis)
    if width <= ellipsis_width:
        fitted: list[str] = []
        used = 0
        for character in plain:
            cells = visible_length(character)
            if used + cells > width:
                break
            fitted.append(character)
            used += cells
        return "".join(fitted)

    fitted = []
    used = 0
    budget = width - ellipsis_width
    for character in plain:
        cells = visible_length(character)
        if used + cells > budget:
            break
        fitted.append(character)
        used += cells
    return "".join(fitted) + ellipsis


def pad(text: str, width: int) -> str:
    return text + " " * max(0, width - visible_length(text))


def ascii_fallback(text: str) -> str:
    return (
        text.replace("·", "|")
        .replace("≥", ">=")
        .replace("…", "...")
        .encode("ascii", "backslashreplace")
        .decode("ascii")
    )


def box(
    title: str, lines: Sequence[str], width: int, theme: Theme, tone: str
) -> list[str]:
    inner = max(20, width - 2)
    if not theme.unicode:
        title = ascii_fallback(title)
    title_text = f" {truncate(title, max(1, inner - 3), theme)} "
    remaining = max(0, inner - 1 - visible_length(title_text))
    top = theme.chars["tl"] + theme.chars["h"] + title_text
    top += theme.chars["h"] * remaining + theme.chars["tr"]
    bottom = theme.chars["bl"] + theme.chars["h"] * inner + theme.chars["br"]
    rendered = [theme.paint(top, tone, "bold")]
    for line in lines:
        if not theme.unicode:
            line = ascii_fallback(line)
        fitted = truncate(line, inner - 2, theme)
        rendered.append(
            theme.paint(theme.chars["v"], tone)
            + " "
            + pad(fitted, inner - 2)
            + " "
            + theme.paint(theme.chars["v"], tone)
        )
    rendered.append(theme.paint(bottom, tone, "bold"))
    return rendered


def human_duration(seconds: float) -> str:
    if not math.isfinite(seconds) or seconds < 0:
        return "unknown"
    if seconds < 0.001:
        return "<1ms"
    if seconds < 1:
        return f"{seconds * 1000:.0f}ms"
    if seconds < 60:
        return f"{seconds:.2f}s"
    minutes, remainder = divmod(seconds, 60)
    return f"{int(minutes)}m {remainder:04.1f}s"


def friendly_lane(name: str) -> str:
    name = escape_inline(name)
    words = name.replace("_", "-").split("-")
    aliases = {
        "debug-fast": "Debug · Fast",
        "release-deep": "Optimized · Fast + Deep",
        "sanitizer": "ASan / UBSan",
        "fast": "Fast",
        "deep": "Fast + Deep",
        "adversarial": "Adversarial",
    }
    if name.lower() in aliases:
        return aliases[name.lower()]
    return " ".join(
        word.upper() if word.lower() in {"asan", "ubsan", "uci"} else word.title()
        for word in words
    )


def friendly_stage(name: str, unicode: bool = True) -> str:
    name = escape_inline(name)
    aliases = {
        "static-traceability": "Static traceability",
        "debug-fast": "Debug · Fast",
        "optimized-fast-deep": "Optimized · Fast + Deep",
        "execution-traceability": "Execution traceability",
        "coverage-evidence": "Coverage evidence",
        "coverage-instrumented-suite": "Instrumented suite",
        "line-coverage-95": (
            "Line coverage ≥ 95%" if unicode else "Line coverage >= 95%"
        ),
        "asan-ubsan": "ASan / UBSan",
        "persistent-fuzz": "Persistent fuzz",
        "curated-mutation": "Curated mutation",
        "input-coherence": "Input coherence",
    }
    return aliases.get(name, name.replace("-", " ").title())


def parse_timestamp(value: Any) -> datetime | None:
    if not isinstance(value, str) or not value:
        return None
    try:
        return datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError:
        return None


def stage_duration(stage: dict[str, Any]) -> float | None:
    start = parse_timestamp(stage.get("started_utc"))
    finish = parse_timestamp(stage.get("finished_utc"))
    if start is None or finish is None:
        return None
    try:
        seconds = (finish - start).total_seconds()
    except (OverflowError, TypeError):
        return None
    return max(0.0, seconds) if math.isfinite(seconds) else None


def status_style(status: str) -> tuple[str, str]:
    normalized = status.lower()
    if normalized in {"pass", "passed", "ok"}:
        return "pass", "green"
    if normalized == "blocked":
        return "blocked", "yellow"
    if normalized == "infra":
        return "infra", "magenta"
    if normalized in {"skip", "skipped", "disabled"}:
        return "skip", "dim"
    if normalized in {"running"}:
        return "running", "cyan"
    return "fail", "red"


def progress_bar(counts: dict[str, int], width: int, theme: Theme) -> str:
    total = sum(counts.values())
    if total <= 0:
        return theme.paint(theme.chars["bar_empty"] * width, "dim")
    ordered = (
        ("pass", "green"),
        ("fail", "red"),
        ("blocked", "yellow"),
        ("infra", "magenta"),
        ("skip", "dim"),
    )
    exact = [width * counts.get(status, 0) / total for status, _ in ordered]
    allocations = [int(value) for value in exact]
    unallocated = width - sum(allocations)
    remainder_order = sorted(
        range(len(ordered)),
        key=lambda index: (exact[index] - allocations[index], -index),
        reverse=True,
    )
    for index in remainder_order[:unallocated]:
        allocations[index] += 1

    chunks: list[str] = []
    for (status, colour), cells in zip(ordered, allocations):
        if cells:
            if theme.colour:
                glyph = (
                    theme.chars["bar"] if status != "skip" else theme.chars["bar_empty"]
                )
            else:
                glyph = {
                    "pass": theme.chars["bar"],
                    "fail": theme.chars["fail"],
                    "blocked": theme.chars["blocked"],
                    "infra": theme.chars["infra"],
                    "skip": theme.chars["bar_empty"],
                }[status]
            chunks.append(theme.paint(glyph * cells, colour))
    return "".join(chunks)


def _issue_source(case: TestCase, project_root: Path) -> str | None:
    matches = list(SOURCE_RE.finditer(case.output))
    if not matches:
        return None
    path = Path(matches[-1].group(1).replace("\\", "/"))
    try:
        relative = path.resolve().relative_to(project_root.resolve()).as_posix()
    except (OSError, ValueError):
        relative = path.as_posix()
    return escape_inline(f"{relative}:{matches[-1].group(2)}")


def _affected_line(issue: Issue, theme: Theme) -> str:
    unique_ids = list(OrderedDict.fromkeys(case.test_id for case in issue.cases))
    shown = ", ".join(escape_inline(test_id) for test_id in unique_ids[:4])
    if len(unique_ids) > 4:
        shown += f", +{len(unique_ids) - 4} more"
    noun = "execution" if len(issue.cases) == 1 else "executions"
    return (
        theme.paint("Affected", "dim")
        + f"  {len(issue.cases)} {noun}"
        + (f" · {shown}" if shown else "")
    )


def promotion_lines(issue: Issue, theme: Theme, project_root: Path) -> list[str]:
    piece_names = {"1": "Queen", "4": "Rook", "2": "Bishop", "3": "Knight"}
    rows: OrderedDict[tuple[str, str, str], None] = OrderedDict()
    for case in issue.cases:
        for match in re.finditer(
            r"PieceType=(\d+)\s+expected=(\S+)\s+actual=(\S+)", case.output
        ):
            rows[(match.group(1), match.group(2), match.group(3))] = None
    lines = [_affected_line(issue, theme), ""]
    lines.append(f"{'Piece':<18}{'Expected':<13}{'Actual':<13}{'Result':<8}")
    for piece, expected, actual in rows:
        label = f"{piece_names.get(piece, 'Piece')} ({piece})"
        result = theme.paint("FAIL", "red", "bold")
        lines.append(f"{label:<18}{expected!r:<13}{actual!r:<13}{result}")
    if rows and len({actual for _, _, actual in rows}) == 1:
        actual = next(iter(rows))[2]
        lines.extend(["", f"Pattern   Every supported promotion returned {actual!r}."])
    source = _issue_source(issue.cases[0], project_root)
    if source:
        lines.append(theme.paint("Source", "dim") + f"    {source}")
    return lines


def bitboard_lines(issue: Issue, theme: Theme, project_root: Path) -> list[str]:
    case = issue.cases[0]
    output = case.output
    board_match = re.search(r"\bboard=(0x[0-9a-fA-F]+)", output)
    expected_match = re.search(
        r"expected_first=(-?\d+)\s+expected_last=(-?\d+)", output
    )
    observed_match = re.search(r"FindLS1B=(-?\d+)\s+FindMS1B=(-?\d+)", output)
    lines = [_affected_line(issue, theme)]
    if board_match:
        lines.append(f"Input     {board_match.group(1)}")
    if expected_match and observed_match:
        comparisons = (
            ("FindLS1B", expected_match.group(1), observed_match.group(1)),
            ("FindMS1B", expected_match.group(2), observed_match.group(2)),
        )
        lines.extend(
            ["", f"{'Operation':<18}{'Expected':<13}{'Actual':<13}{'Result':<8}"]
        )
        mismatches = 0
        for operation, expected, actual in comparisons:
            mismatch = expected != actual
            mismatches += int(mismatch)
            result = theme.paint(
                "FAIL" if mismatch else "PASS", "red" if mismatch else "green", "bold"
            )
            lines.append(f"{operation:<18}{expected:<13}{actual:<13}{result}")
        if mismatches == 1:
            lines.extend(["", "Other observed bit operations matched the model."])
    source = _issue_source(case, project_root)
    if source:
        lines.append(theme.paint("Source", "dim") + f"    {source}")
    if len({candidate.output for candidate in issue.cases}) > 1:
        lines.append(
            theme.paint("Evidence", "dim")
            + f"  Showing {friendly_lane(case.lane)}; lane outputs differ."
        )
    return lines


@dataclass(frozen=True)
class UciVariant:
    name: str
    awaited: str
    observed: str
    elapsed: str


def parse_uci_variants(output: str) -> list[UciVariant]:
    starts = list(re.finditer(r"(?m)^\s*variant-id=([^\r\n]+)", output))
    variants: list[UciVariant] = []
    for index, start in enumerate(starts):
        end = starts[index + 1].start() if index + 1 < len(starts) else len(output)
        block = output[start.start() : end]
        operation = re.search(r"(?m)^\s*operation=wait-for-([^\s\r\n]+)", block)
        diagnostic = re.search(
            r"diagnostic ordinal=.*?elapsed=(\d+)ms", block, re.DOTALL
        )
        tokens = re.findall(r'token="([^"]+)"', block)
        stdout = re.search(r"stdout-bytes=(\d+)", block)
        awaited = operation.group(1) if operation else "response"
        if tokens:
            observed = f"last token {tokens[-1]}"
        elif stdout and stdout.group(1) != "0":
            observed = f"{stdout.group(1)} B stdout"
        else:
            observed = "no stdout"
        elapsed = f"{int(diagnostic.group(1)) / 1000:.2f}s" if diagnostic else "timeout"
        variants.append(
            UciVariant(
                name=escape_inline(start.group(1).strip()),
                awaited=escape_inline(awaited),
                observed=escape_inline(observed),
                elapsed=elapsed,
            )
        )
    return variants


def uci_lines(issue: Issue, theme: Theme) -> list[str]:
    variants: OrderedDict[tuple[str, str], UciVariant] = OrderedDict()
    all_output = "\n".join(case.output for case in issue.cases)
    lanes = list(OrderedDict.fromkeys(case.lane for case in issue.cases))
    for case in issue.cases:
        for variant in parse_uci_variants(case.output):
            variants[(case.lane, variant.name)] = variant
    lines = [_affected_line(issue, theme), ""]
    if variants:
        lines.append(f"{'Variant':<22}{'Awaited':<13}{'Observed':<22}{'Time':<9}")
        for (lane, _), variant in variants.items():
            label = variant.name
            if len(lanes) > 1:
                lane_tag = {
                    "debug-fast": "dbg",
                    "release-deep": "opt",
                    "coverage": "cov",
                    "sanitizer": "san",
                }.get(lane.lower(), escape_inline(lane))
                label = f"{label} [{lane_tag}]"
            lines.append(
                pad(truncate(label, 21, theme), 22)
                + pad(truncate(variant.awaited, 12, theme), 13)
                + pad(truncate(variant.observed, 21, theme), 22)
                + pad(variant.elapsed, 9)
            )
    else:
        lines.append(
            "The response deadline expired before the expected UCI token arrived."
        )

    cleanup_values = re.findall(r"cleanup-outcome=([^\s\r\n]+)", all_output)
    if cleanup_values:
        cleanup = (
            "successful"
            if set(cleanup_values) == {"ok"}
            else ", ".join(
                escape_inline(value) for value in sorted(set(cleanup_values))
            )
        )
        lines.extend(["", f"Cleanup   {cleanup} for all reported variants"])
    engine_hash = re.search(
        r"sha256=\s*([0-9a-fA-F][0-9a-fA-F\s]{31,})\s+pid=", all_output
    )
    if engine_hash:
        digest = re.sub(r"\s+", "", engine_hash.group(1))
        if len(digest) >= 12:
            lines.append(f"Engine    TDFA.exe {digest[:8]}…{digest[-4:]}")
    return lines


def fixture_lines(
    issue: Issue, fixtures: Sequence[FixtureIssue], theme: Theme
) -> list[str]:
    if issue.code == "FIXTURE_INTEGRITY":
        return [
            _affected_line(issue, theme),
            "",
            "The canonical fixture integrity guard itself failed.",
            "No line-ending cause was proven from the retained evidence.",
            "Inspect the raw JUnit and CTest log before treating dependent results as valid.",
        ]

    lines = [
        _affected_line(issue, theme),
        "",
        "These checks did not reach their behavioural assertions.",
        "",
    ]
    if fixtures:
        lines.append(f"{'Fixture':<42}{'Detected':<24}")
        for fixture in fixtures:
            line_count = fixture.lf_count or fixture.cr_count
            detected = f"CRLF on {fixture.cr_count}/{line_count} lines"
            lines.append(f"{escape_inline(fixture.path):<42}{detected:<24}")
        lines.extend(
            [
                "",
                "Expected   LF-only canonical fixture bytes",
                "Cause      Likely checkout line-ending conversion",
                "Observed   Checkout bytes at report finalization",
            ]
        )
    else:
        lines.append("A canonical fixture failed its format or checksum guard.")
    return lines


def generic_lines(issue: Issue, theme: Theme, project_root: Path) -> list[str]:
    case = issue.cases[0]
    lines = [_affected_line(issue, theme)]
    source = _issue_source(case, project_root)
    if source:
        lines.append(theme.paint("Source", "dim") + f"    {source}")

    output = case.output
    failed = re.search(r"(?ms)^.+?: FAILED:\s*\n(.*?)(?=^={20,}|\Z)", output)
    excerpt = failed.group(1).strip() if failed else output.strip()
    excerpt_lines = [
        line.rstrip()
        for line in excerpt.splitlines()
        if line.strip()
        and not line.strip().startswith(("Filters:", "Randomness seeded"))
    ]
    if excerpt_lines:
        lines.append("")
        lines.extend(escape_inline(line) for line in excerpt_lines[:8])
        if len(excerpt_lines) > 8:
            lines.append(
                f"… {len(excerpt_lines) - 8} more diagnostic lines in the raw evidence"
            )
    if len({case.output for case in issue.cases}) > 1:
        lines.extend(
            [
                "",
                theme.paint("Evidence", "dim")
                + f"  Showing {friendly_lane(case.lane)}; lane outputs differ.",
            ]
        )
    return lines


def issue_lines(
    issue: Issue,
    fixtures: Sequence[FixtureIssue],
    theme: Theme,
    project_root: Path,
) -> list[str]:
    if issue.code in {"FIXTURE_EOL", "FIXTURE_INTEGRITY"}:
        return fixture_lines(issue, fixtures, theme)
    if issue.key == "PROMOTION_MAPPING":
        return promotion_lines(issue, theme, project_root)
    if issue.key == "BIT_SCAN_MODEL":
        return bitboard_lines(issue, theme, project_root)
    if issue.code == "UCI_TIMEOUT":
        return uci_lines(issue, theme)
    return generic_lines(issue, theme, project_root)


def pipeline_lines(summary: dict[str, Any], theme: Theme, width: int) -> list[str]:
    stages = summary.get("stages")
    if not isinstance(stages, list):
        return []
    name_width = max(
        22,
        min(
            38,
            max(
                (
                    len(
                        friendly_stage(
                            str(stage.get("name", "")), unicode=theme.unicode
                        )
                    )
                    for stage in stages
                    if isinstance(stage, dict)
                ),
                default=22,
            ),
        ),
    )
    lines: list[str] = []
    for stage in stages:
        if not isinstance(stage, dict):
            continue
        status = str(stage.get("status", "UNKNOWN")).lower()
        icon_name, tone = status_style(status)
        icon = theme.paint(theme.chars[icon_name], tone, "bold")
        label = truncate(
            friendly_stage(str(stage.get("name", "unnamed")), unicode=theme.unicode),
            name_width,
            theme,
        )
        duration = stage_duration(stage)
        duration_text = human_duration(duration) if duration is not None else ""
        status_text = theme.paint(
            truncate(escape_inline(status.upper()), 14, theme), tone, "bold"
        )
        available = max(0, width - 18 - name_width)
        detail = escape_inline(str(stage.get("detail") or ""))
        if detail in {"ctest exited with code 8", "ctest exited with code 1"}:
            detail = ""
        suffix = truncate(duration_text, available, theme)
        if detail and available > len(duration_text) + 4:
            detail_width = available - len(duration_text) - 3
            suffix = f"{truncate(detail, detail_width, theme)}  {duration_text}".strip()
        lines.append(f"{icon} {pad(label, name_width)}  {pad(status_text, 14)}{suffix}")
    return lines


def lane_lines(
    cases: Sequence[TestCase],
    issues: Sequence[Issue],
    theme: Theme,
    width: int,
) -> list[str]:
    blocked_cases = {
        id(case)
        for issue in issues
        if issue.severity == "blocked"
        for case in issue.cases
    }
    infra_cases = {
        id(case)
        for issue in issues
        if issue.severity == "infra"
        for case in issue.cases
    }
    lanes: OrderedDict[str, list[TestCase]] = OrderedDict()
    for case in cases:
        lanes.setdefault(case.lane, []).append(case)
    lines: list[str] = []
    for lane, lane_cases in lanes.items():
        passed = sum(case.status == "pass" for case in lane_cases)
        skipped = sum(case.status in {"skip", "disabled"} for case in lane_cases)
        blocked = sum(id(case) in blocked_cases for case in lane_cases)
        infra = sum(id(case) in infra_cases for case in lane_cases)
        failed = sum(case.status == "fail" for case in lane_cases) - sum(
            case.status == "fail" and id(case) in (blocked_cases | infra_cases)
            for case in lane_cases
        )
        duration = sum(case.duration for case in lane_cases)
        state = (
            "FAIL"
            if failed or infra
            else (
                "BLOCKED"
                if blocked
                else "SKIPPED" if skipped and not passed else "PASS"
            )
        )
        icon_name, tone = status_style(state)
        icon = theme.paint(theme.chars[icon_name], tone, "bold")
        state_text = theme.paint(state, tone, "bold")
        result_parts = [f"{passed} pass"]
        if failed:
            result_parts.append(f"{failed} fail")
        if blocked:
            result_parts.append(f"{blocked} blocked")
        if infra:
            result_parts.append(f"{infra} infra")
        if skipped:
            result_parts.append(f"{skipped} skipped")
        result = " · ".join(result_parts)
        lane_label = friendly_lane(lane)
        duration_text = human_duration(duration)
        if width < 92:
            lines.append(
                truncate(
                    f"{icon} {lane_label}: {state_text} · {result} ({duration_text})",
                    width,
                    theme,
                )
            )
        else:
            lane_label = truncate(lane_label, 28, theme)
            result_width = max(20, width - 49)
            result = truncate(result, result_width, theme)
            lines.append(
                f"{icon} {pad(lane_label, 28)}{pad(state_text, 10)}"
                f"{pad(result, result_width)}{duration_text:>9}"
            )
    return lines


def _summary_status(
    summary: dict[str, Any] | None,
    failure_count: int,
    blocked_count: int,
    infra_count: int,
) -> str:
    if failure_count or infra_count:
        return "FAIL"
    if summary:
        pipeline_status = str(summary.get("pipeline_status", "")).upper()
        if pipeline_status == "FAIL":
            return pipeline_status
    if blocked_count:
        return "BLOCKED"
    if summary:
        pipeline_status = str(summary.get("pipeline_status", "")).upper()
        if pipeline_status in {"PASS", "RUNNING"}:
            return pipeline_status
    return "PASS"


def evidence_counts(
    cases: Sequence[TestCase],
    issues: Sequence[Issue],
    warnings: Sequence[str] = (),
) -> dict[str, int]:
    blocked_keys = {
        id(case)
        for issue in issues
        if issue.severity == "blocked"
        for case in issue.cases
    }
    infra_keys = {
        id(case)
        for issue in issues
        if issue.severity == "infra"
        for case in issue.cases
    }
    classified_non_failures = blocked_keys | infra_keys
    return {
        "pass": sum(case.status == "pass" for case in cases),
        "fail": sum(case.status == "fail" for case in cases)
        - sum(
            case.status == "fail" and id(case) in classified_non_failures
            for case in cases
        ),
        "blocked": len(blocked_keys),
        "infra": len(infra_keys) + len(OrderedDict.fromkeys(warnings)),
        "skip": sum(case.status in {"skip", "disabled"} for case in cases),
    }


def resolved_status(
    *,
    cases: Sequence[TestCase],
    counts: dict[str, int],
    summary: dict[str, Any] | None,
    status_override: str | None,
    external_failure_count: int = 0,
) -> str:
    derived = _summary_status(
        summary, counts["fail"], counts["blocked"], counts["infra"]
    )
    if external_failure_count:
        derived = "FAIL"
    # An explicit external gate can make the result stricter, but it cannot
    # paint failed or incomplete evidence green.
    if (
        derived == "PASS"
        and cases
        and counts["pass"] == 0
        and counts["fail"] == 0
        and counts["blocked"] == 0
        and counts["infra"] == 0
        and counts["skip"] > 0
    ):
        derived = "SKIPPED"
    override = status_override.upper() if status_override else None
    severity = {
        "PASS": 0,
        "SKIPPED": 1,
        "RUNNING": 2,
        "BLOCKED": 3,
        "FAIL": 4,
    }
    if override and severity.get(override, -1) > severity.get(derived, -1):
        return override
    return derived


def rerun_command(
    case: TestCase, build_directory: str | None = None
) -> str | None:
    trusted_id = TEST_ID_RE.search(case.name)
    if trusted_id is None:
        return None
    test_pattern = trusted_id.group(1).split(".", 1)[0]
    if build_directory:
        return subprocess.list2cmdline(
            [
                "ctest",
                "--test-dir",
                build_directory,
                "-R",
                test_pattern,
                "--output-on-failure",
            ]
        )
    lane = case.lane.lower()
    if "coverage" in lane:
        return (
            "ctest --test-dir build/coverage-gcc "
            f"-R {test_pattern} --output-on-failure"
        )
    preset = (
        "test-deep"
        if "deep" in lane
        else (
            "test-adversarial"
            if any(token in lane for token in ("sanit", "adversarial"))
            else "test-fast"
        )
    )
    return f"ctest --preset {preset} -R {test_pattern} --output-on-failure"


def wrapped_plain(text: str, width: int) -> list[str]:
    logical_lines = textwrap.wrap(
        text,
        width=max(1, width),
        break_long_words=True,
        break_on_hyphens=False,
        replace_whitespace=False,
    ) or [""]
    fitted: list[str] = []
    for logical_line in logical_lines:
        remaining = logical_line
        while visible_length(remaining) > width:
            used = 0
            split_at = 0
            for index, character in enumerate(remaining):
                cells = visible_length(character)
                if used + cells > width:
                    break
                used += cells
                split_at = index + 1
            if split_at == 0:
                split_at = 1
            fitted.append(remaining[:split_at])
            remaining = remaining[split_at:]
        fitted.append(remaining)
    return fitted


def render_dashboard(
    *,
    mode: str,
    cases: Sequence[TestCase],
    issues: Sequence[Issue],
    fixtures: Sequence[FixtureIssue],
    summary: dict[str, Any] | None,
    artifact_root: str | None,
    warnings: Sequence[str],
    width: int,
    theme: Theme,
    project_root: Path,
    max_issues: int,
    rerun_build_directory: str | None = None,
    status_override: str | None = None,
    external_failures: Sequence[str] = (),
) -> str:
    width = max(40, min(160, width))
    warnings = list(OrderedDict.fromkeys(warnings))
    external_failures = list(OrderedDict.fromkeys(external_failures))
    counts = evidence_counts(cases, issues, warnings)
    executions = len(cases)
    unique_cases = len({case.name for case in cases})
    run_status = resolved_status(
        cases=cases,
        counts=counts,
        summary=summary,
        status_override=status_override,
        external_failure_count=len(external_failures),
    )
    status_icon, status_tone = status_style(run_status)

    mode_label = escape_inline(mode.upper())
    system_label = f"{platform.system()} · {platform.machine() or 'unknown'}"
    cardinality = (
        f"{executions} tests"
        if executions == unique_cases
        else f"{executions} executions · {unique_cases} unique tests"
    )
    header_lines = [
        (
            theme.paint(
                f"{theme.chars[status_icon]} {mode_label} · {run_status}",
                status_tone,
                "bold",
            )
            + " " * 4
            + theme.paint(system_label, "dim")
        ),
        cardinality,
    ]
    output: list[str] = box(
        "TDFA VERIFICATION", header_lines, width, theme, status_tone
    )

    output.extend(["", theme.paint("  SUITE HEALTH", "bold", "white")])
    output.append("  " + progress_bar(counts, min(56, width - 4), theme))
    metrics = [
        ("pass", "green", f"{counts['pass']} passed"),
        ("fail", "red", f"{counts['fail']} failed"),
        ("blocked", "yellow", f"{counts['blocked']} blocked"),
        ("infra", "magenta", f"{counts['infra']} infra"),
        ("skip", "dim", f"{counts['skip']} skipped"),
    ]
    rendered_metrics = [
        theme.paint(f"{theme.chars[icon]} {label}", tone, "bold")
        for icon, tone, label in metrics
    ]
    combined_metrics = "    ".join(rendered_metrics)
    if visible_length(combined_metrics) <= width - 4:
        output.append("  " + combined_metrics)
    else:
        output.append(truncate("  " + "    ".join(rendered_metrics[:3]), width, theme))
        output.append(truncate("  " + "    ".join(rendered_metrics[3:]), width, theme))

    if cases:
        output.extend(["", theme.paint("  TEST LANES", "bold", "white")])
        output.extend(
            "  " + line for line in lane_lines(cases, issues, theme, width - 2)
        )

    if summary:
        stage_lines = pipeline_lines(summary, theme, width - 2)
        if stage_lines:
            output.extend(["", theme.paint("  PIPELINE", "bold", "white")])
            output.extend("  " + line for line in stage_lines)
        coverage = summary.get("coverage")
        if isinstance(coverage, dict):
            metrics_value = coverage.get("metrics")
            if isinstance(metrics_value, dict):
                lines_value = metrics_value.get("lines")
                if (
                    isinstance(lines_value, dict)
                    and lines_value.get("percent") is not None
                ):
                    coverage_line = (
                        "  "
                        + theme.paint("Coverage", "dim")
                        + "  "
                        + escape_inline(str(lines_value.get("percent")))
                        + "% lines ("
                        + escape_inline(str(lines_value.get("covered")))
                        + "/"
                        + escape_inline(str(lines_value.get("total")))
                        + ")"
                    )
                    output.append(truncate(coverage_line, width, theme))
        elif summary.get("line_percent") is not None:
            coverage_line = (
                "  "
                + theme.paint("Coverage", "dim")
                + "  "
                + escape_inline(str(summary.get("line_percent")))
                + "% lines ("
                + escape_inline(str(summary.get("line_covered", "?")))
                + "/"
                + escape_inline(str(summary.get("line_total", "?")))
                + ")"
            )
            output.append(truncate(coverage_line, width, theme))

    if external_failures:
        output.extend(["", theme.paint("  GATE FAILURES", "bold", "red")])
        for failure in external_failures:
            safe_failure = escape_inline(failure)
            wrapped = wrapped_plain(safe_failure, width - 6)
            output.append(
                "  " + theme.paint(f"{theme.chars['fail']} {wrapped[0]}", "red", "bold")
            )
            output.extend("    " + line for line in wrapped[1:])

    if issues:
        output.extend(["", theme.paint("  DIAGNOSTICS", "bold", "white")])
        for ordinal, issue in enumerate(issues[:max_issues], start=1):
            tone = (
                "yellow"
                if issue.severity == "blocked"
                else "magenta" if issue.severity == "infra" else "red"
            )
            icon_name = (
                "blocked"
                if issue.severity == "blocked"
                else "infra" if issue.severity == "infra" else "fail"
            )
            icon = theme.chars[icon_name]
            title = (
                f"{icon} {issue.severity.upper()} · {issue.code} · "
                f"{escape_inline(issue.title)} · {len(issue.cases)}"
            )
            output.extend(
                box(
                    title,
                    issue_lines(issue, fixtures, theme, project_root),
                    width,
                    theme,
                    tone,
                )
            )
            if ordinal != min(len(issues), max_issues):
                output.append("")
        if len(issues) > max_issues:
            output.append(
                truncate(
                    theme.paint(
                        f"  … {len(issues) - max_issues} additional diagnostic groups are in the evidence.",
                        "dim",
                    ),
                    width,
                    theme,
                )
            )

    true_failure = next(
        (
            case
            for issue in issues
            if issue.severity in {"fail", "infra"}
            for case in issue.cases
        ),
        None,
    )
    if true_failure or artifact_root or warnings or external_failures:
        output.extend(["", theme.paint("  NEXT", "bold", "white")])
    if true_failure:
        command = rerun_command(true_failure, rerun_build_directory)
        labelled_width = width - 13
        if command and (width < 90 or len(command) > labelled_width):
            output.append("  " + theme.paint("Rerun", "cyan", "bold"))
            output.extend("    " + line for line in wrapped_plain(command, width - 4))
        elif command:
            output.append(
                "  " + theme.paint("Rerun", "cyan", "bold") + f"     {command}"
            )
    if artifact_root:
        safe_artifact_root = escape_inline(artifact_root)
        if width < 90:
            output.append("  " + theme.paint("Evidence", "cyan", "bold"))
            output.append("    " + truncate(safe_artifact_root, width - 4, theme))
        else:
            output.append(
                "  "
                + theme.paint("Evidence", "cyan", "bold")
                + "  "
                + truncate(safe_artifact_root, width - 12, theme)
            )
    for warning in warnings:
        safe_warning = truncate(escape_inline(warning), width - 4, theme)
        output.append(
            "  " + theme.paint(f"{theme.chars['infra']} {safe_warning}", "magenta")
        )

    report = "\n".join(output) + "\n"
    if not theme.unicode:
        report = ascii_fallback(report)
        report = (
            "\n".join(truncate(line, width, theme) for line in report.splitlines())
            + "\n"
        )
    return report


def diagnostics_document(
    *,
    mode: str,
    cases: Sequence[TestCase],
    issues: Sequence[Issue],
    fixtures: Sequence[FixtureIssue],
    warnings: Sequence[str] = (),
    summary: dict[str, Any] | None = None,
    status_override: str | None = None,
    external_failures: Sequence[str] = (),
) -> dict[str, Any]:
    warnings = list(OrderedDict.fromkeys(warnings))
    counts = evidence_counts(cases, issues, warnings)
    return {
        "schema_version": 3,
        "mode": mode,
        "status": resolved_status(
            cases=cases,
            counts=counts,
            summary=summary,
            status_override=status_override,
            external_failure_count=len(external_failures),
        ),
        "counts": {
            "executions": len(cases),
            "unique_tests": len({case.name for case in cases}),
            "passed": counts["pass"],
            "failed": counts["fail"],
            "blocked": counts["blocked"],
            "infrastructure": counts["infra"],
            "skipped": counts["skip"],
        },
        "fixture_observation": "checkout bytes at report finalization",
        "fixtures": [asdict(fixture) for fixture in fixtures],
        "warnings": warnings,
        "external_failures": list(OrderedDict.fromkeys(external_failures)),
        "issues": [
            {
                "code": issue.code,
                "severity": issue.severity,
                "title": issue.title,
                "executions": len(issue.cases),
                "tests": [
                    {
                        "lane": case.lane,
                        "name": case.name,
                        "test_id": case.test_id,
                        "junit": case.source_junit,
                    }
                    for case in issue.cases
                ],
            }
            for issue in issues
        ],
    }


def write_json_atomic(path: Path, document: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(f".{path.name}.{os.getpid()}.tmp")
    try:
        with temporary.open("w", encoding="utf-8", newline="\n") as stream:
            stream.write(json.dumps(document, indent=2, sort_keys=False) + "\n")
        os.replace(temporary, path)
    except BaseException:
        try:
            temporary.unlink()
        except OSError:
            pass
        raise


def supports_unicode(stream: Any) -> bool:
    encoding = getattr(stream, "encoding", None) or "utf-8"
    try:
        "╭✓◆…".encode(encoding)
    except (LookupError, UnicodeEncodeError):
        return False
    return True


def enable_windows_ansi() -> bool:
    if os.name != "nt":
        return True
    try:
        import ctypes

        kernel32 = ctypes.windll.kernel32
        handle = kernel32.GetStdHandle(-11)
        mode = ctypes.c_uint32()
        if handle in (0, -1) or not kernel32.GetConsoleMode(handle, ctypes.byref(mode)):
            return False
        return bool(kernel32.SetConsoleMode(handle, mode.value | 0x0004))
    except (AttributeError, OSError):
        return False


def should_colour(mode: str) -> bool:
    if mode == "always":
        return True
    if (
        mode == "never"
        or os.environ.get("NO_COLOR") is not None
        or os.environ.get("TERM", "").lower() == "dumb"
    ):
        return False
    return bool(sys.stdout.isatty() and enable_windows_ansi())


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Render TDFA CTest JUnit evidence as a terminal dashboard."
    )
    parser.add_argument(
        "--junit",
        action="append",
        default=[],
        metavar="[LANE=]PATH",
        help="CTest JUnit file; repeat for multiple build/test lanes.",
    )
    parser.add_argument("--summary", type=Path, help="Verification summary.json.")
    parser.add_argument(
        "--project-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="TDFA checkout root (defaults to this script's parent checkout).",
    )
    parser.add_argument("--artifact-root", help="Human-readable evidence location.")
    parser.add_argument(
        "--rerun-build-directory",
        help="Exact existing CMake tree to use in suggested CTest reruns.",
    )
    parser.add_argument("--mode", default="Verification", help="Run mode label.")
    parser.add_argument(
        "--status",
        choices=("PASS", "FAIL", "BLOCKED", "RUNNING", "SKIPPED"),
        help="Explicit overall presentation status for focused non-JUnit gates.",
    )
    parser.add_argument(
        "--colour",
        "--color",
        choices=("auto", "always", "never"),
        default="auto",
        help="ANSI colour policy (default: auto).",
    )
    parser.add_argument(
        "--ascii",
        action="store_true",
        help="Use ASCII borders and status glyphs.",
    )
    parser.add_argument(
        "--plain",
        action="store_true",
        help="Stable ASCII output with no ANSI colour.",
    )
    parser.add_argument(
        "--width",
        type=int,
        help="Report width; defaults to the current terminal width.",
    )
    parser.add_argument(
        "--max-issues",
        type=int,
        default=8,
        help="Maximum diagnostic groups shown inline (default: 8).",
    )
    parser.add_argument(
        "--diagnostics-json",
        type=Path,
        help="Write the normalized human-diagnostic index as JSON.",
    )
    parser.add_argument(
        "--failure-detail",
        action="append",
        default=[],
        help="External gate failure to show and retain; repeat when needed.",
    )
    parser.add_argument(
        "--fail-on-test-failure",
        action="store_true",
        help=(
            "Exit 1 for any raw JUnit failure, infrastructure/evidence warning, "
            "or external gate failure."
        ),
    )
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    project_root = args.project_root.resolve()
    parsed = ParsedEvidence()
    if not args.junit:
        parsed.warnings.append("No JUnit evidence was provided.")
    for specification in args.junit:
        lane_evidence = parse_junit(specification)
        parsed.cases.extend(lane_evidence.cases)
        parsed.warnings.extend(lane_evidence.warnings)
        parsed.timestamps.extend(lane_evidence.timestamps)

    summary, summary_warnings = load_summary(args.summary)
    parsed.warnings.extend(summary_warnings)
    fixtures = discover_fixture_issues(project_root)
    issues = group_issues(parsed.cases, fixtures)

    width = args.width or shutil.get_terminal_size((108, 24)).columns
    plain = bool(args.plain)
    theme = Theme(
        colour=False if plain else should_colour(args.colour),
        unicode=False if plain or args.ascii else supports_unicode(sys.stdout),
    )
    report = render_dashboard(
        mode=args.mode,
        cases=parsed.cases,
        issues=issues,
        fixtures=fixtures,
        summary=summary,
        artifact_root=args.artifact_root,
        warnings=parsed.warnings,
        width=width,
        theme=theme,
        project_root=project_root,
        max_issues=max(1, args.max_issues),
        rerun_build_directory=args.rerun_build_directory,
        status_override=args.status,
        external_failures=args.failure_detail,
    )
    try:
        sys.stdout.write(report)
    except UnicodeEncodeError:
        fallback = Theme(colour=False, unicode=False)
        sys.stdout.write(
            render_dashboard(
                mode=args.mode,
                cases=parsed.cases,
                issues=issues,
                fixtures=fixtures,
                summary=summary,
                artifact_root=args.artifact_root,
                warnings=parsed.warnings,
                width=width,
                theme=fallback,
                project_root=project_root,
                max_issues=max(1, args.max_issues),
                rerun_build_directory=args.rerun_build_directory,
                status_override=args.status,
                external_failures=args.failure_detail,
            )
        )

    if args.diagnostics_json:
        write_json_atomic(
            args.diagnostics_json,
            diagnostics_document(
                mode=args.mode,
                cases=parsed.cases,
                issues=issues,
                fixtures=fixtures,
                warnings=parsed.warnings,
                summary=summary,
                status_override=args.status,
                external_failures=args.failure_detail,
            ),
        )

    if args.fail_on_test_failure and (
        parsed.warnings
        or args.failure_detail
        or any(case.status in {"fail", "infra"} for case in parsed.cases)
    ):
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
