#!/usr/bin/env python3
"""Validate TDFA semantic traceability and optional CTest execution evidence."""

from __future__ import annotations

import argparse
import ast
from collections import Counter
from dataclasses import dataclass
import fnmatch
import hashlib
import json
from pathlib import Path
import re
import sys
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]
TRACEABILITY = ROOT / "tests/spec/traceability.yaml"
PURPOSE_MAP = ROOT / "tests/spec/intended-purpose-v1.md"
SNAPSHOT_MANIFEST = ROOT / "tests/spec/source-snapshot.sha256"
TEST_ID_PATTERN = r"T-(?:[A-Z0-9]+-)+[0-9]{3}"
TEST_ID = re.compile(rf"^{TEST_ID_PATTERN}$")
EXECUTABLE_STATUSES = {"DESIGNED", "EXPECTATION_FROZEN", "IMPLEMENTED", "REVIEWED"}


@dataclass(frozen=True)
class CatalogueRecord:
    status: str
    symbols: tuple[str, ...]
    path: Path


@dataclass(frozen=True)
class TestImplementation:
    test_id: str
    title: str
    tags: str
    path: Path
    body: str

    @property
    def routine_scheduled(self) -> bool:
        if "[deep]" in self.tags:
            return "[audit]" not in self.tags
        return all(
            excluded not in self.tags
            for excluded in ("[adversarial]", "[mutation]", "[audit]")
        )

    @property
    def routine_ctest_name(self) -> str:
        if "[deep]" in self.tags:
            return f"deep::{self.title}"
        if "[uci]" in self.tags:
            return f"uci::{self.title}"
        return f"fast::{self.title}"


def fail(errors: list[str], message: str) -> None:
    errors.append(message)


def nonempty(value: object) -> bool:
    return isinstance(value, str) and bool(value.strip())


def purpose_symbols(text: str) -> list[str]:
    result: list[str] = []
    for line in text.splitlines():
        if not line.startswith("|"):
            continue
        cells = line.split("|")
        if len(cells) < 3:
            continue
        first = cells[1].strip()
        code_spans = re.findall(r"`([^`]+)`", first)
        if not code_spans or first.startswith("Callable/function group"):
            continue
        symbol = code_spans[-1]
        if first.startswith("legacy "):
            symbol = f"legacy {symbol}"
        result.append(symbol)
    return result


def split_flow_list(value: str) -> list[str]:
    value = value.strip()
    if not (value.startswith("[") and value.endswith("]")):
        raise ValueError("flow list must be enclosed by []")
    content = value[1:-1]
    result: list[str] = []
    start = 0
    quote: str | None = None
    escaped = False
    nesting = 0
    pairs = {"(": ")", "<": ">", "{": "}"}
    closers = set(pairs.values())
    for index, char in enumerate(content):
        if quote is not None:
            if escaped:
                escaped = False
            elif char == "\\":
                escaped = True
            elif char == quote:
                quote = None
            continue
        if char in {'"', "'"}:
            quote = char
        elif char in pairs:
            nesting += 1
        elif char in closers:
            nesting = max(0, nesting - 1)
        elif char == "," and nesting == 0:
            result.append(content[start:index].strip())
            start = index + 1
    tail = content[start:].strip()
    if tail:
        result.append(tail)
    decoded: list[str] = []
    for item in result:
        if not item:
            raise ValueError("empty flow-list item")
        if item[0:1] in {'"', "'"}:
            parsed = ast.literal_eval(item)
            if not isinstance(parsed, str):
                raise ValueError("quoted symbol is not a string")
            decoded.append(parsed)
        else:
            decoded.append(item)
    return decoded


def parse_record_symbols(block: str, context: str, errors: list[str]) -> tuple[str, ...]:
    matches = list(re.finditer(r"^    symbols:[ \t]*(.*)$", block, re.MULTILINE))
    if len(matches) != 1:
        fail(errors, f"{context}: expected exactly one symbols field, found {len(matches)}")
        return ()
    match = matches[0]
    inline = match.group(1).strip()
    try:
        if inline:
            values = split_flow_list(inline)
        else:
            values = []
            for line in block[match.end():].splitlines():
                if line.startswith("      - "):
                    values.append(line[8:].strip().strip('"'))
                elif line.strip() == "":
                    continue
                else:
                    break
    except (ValueError, SyntaxError) as exc:
        fail(errors, f"{context}: cannot parse symbols: {exc}")
        return ()
    if not values or any(not value.strip() for value in values):
        fail(errors, f"{context}: symbols must be a nonempty list")
    return tuple(values)


def catalogue_records(path: Path, expected_snapshot: str, errors: list[str]) -> dict[str, CatalogueRecord]:
    text = path.read_text(encoding="utf-8")
    header = re.search(
        r"^snapshot:\s+(?:(?P<anchor>&[A-Za-z0-9_-]+)\s+)?(?P<hash>[0-9a-f]{64})\s*$",
        text,
        re.MULTILINE,
    )
    if header is None or header.group("hash") != expected_snapshot:
        fail(errors, f"{path.relative_to(ROOT)}: catalogue snapshot mismatch")
    anchor = header.group("anchor")[1:] if header and header.group("anchor") else None

    starts = list(re.finditer(r"^tests:\s*$", text, re.MULTILINE))
    if len(starts) != 1:
        fail(errors, f"{path.relative_to(ROOT)}: expected exactly one top-level tests section")
        return {}
    section_start = starts[0].end()
    next_section = re.search(r"^[A-Za-z_][A-Za-z0-9_]*:\s*", text[section_start:], re.MULTILINE)
    section_end = section_start + next_section.start() if next_section else len(text)
    section = text[section_start:section_end]
    entry_matches = list(re.finditer(r"^  - id:\s*(.*?)\s*$", section, re.MULTILINE))
    stray_entries = [
        line for line in section.splitlines()
        if line.startswith("  - ") and not line.startswith("  - id:")
    ]
    if stray_entries:
        fail(errors, f"{path.relative_to(ROOT)}: malformed tests entries: {stray_entries[:3]}")

    records: dict[str, CatalogueRecord] = {}
    for index, match in enumerate(entry_matches):
        test_id = match.group(1)
        context = f"{path.relative_to(ROOT)}:{test_id}"
        if TEST_ID.fullmatch(test_id) is None:
            fail(errors, f"{context}: invalid unquoted test id")
            continue
        end = entry_matches[index + 1].start() if index + 1 < len(entry_matches) else len(section)
        block = section[match.start():end]
        statuses = re.findall(r"^    status:\s*([A-Z0-9_]+)\s*$", block, re.MULTILINE)
        if len(statuses) != 1:
            fail(errors, f"{context}: expected exactly one status, found {len(statuses)}")
            status = "INVALID"
        else:
            status = statuses[0]
        snapshots = re.findall(r"^    snapshot:\s*(.*?)\s*$", block, re.MULTILINE)
        if len(snapshots) != 1:
            fail(errors, f"{context}: expected exactly one snapshot, found {len(snapshots)}")
        else:
            record_snapshot = snapshots[0]
            if record_snapshot.startswith("*"):
                if anchor is None or record_snapshot != f"*{anchor}":
                    fail(errors, f"{context}: unknown snapshot alias {record_snapshot}")
            elif record_snapshot != expected_snapshot:
                fail(errors, f"{context}: record snapshot mismatch")
        symbols = parse_record_symbols(block, context, errors)
        if test_id in records:
            fail(errors, f"{path.relative_to(ROOT)}: duplicate test record {test_id}")
        records[test_id] = CatalogueRecord(status, symbols, path)
    if not records:
        fail(errors, f"{path.relative_to(ROOT)}: no test records parsed")
    return records


def skip_cpp_literal_or_comment(text: str, index: int) -> int | None:
    if text.startswith("//", index):
        end = text.find("\n", index + 2)
        return len(text) if end < 0 else end
    if text.startswith("/*", index):
        end = text.find("*/", index + 2)
        return len(text) if end < 0 else end + 2
    if text.startswith('R"', index):
        delimiter_end = text.find("(", index + 2)
        if delimiter_end < 0:
            return len(text)
        delimiter = text[index + 2:delimiter_end]
        end = text.find(")" + delimiter + '"', delimiter_end + 1)
        return len(text) if end < 0 else end + len(delimiter) + 2
    if text[index:index + 1] in {'"', "'"}:
        quote = text[index]
        index += 1
        while index < len(text):
            if text[index] == "\\":
                index += 2
            elif text[index] == quote:
                return index + 1
            else:
                index += 1
        return len(text)
    return None


def matching_delimiter(text: str, start: int, opening: str, closing: str) -> int:
    depth = 0
    index = start
    while index < len(text):
        skipped = skip_cpp_literal_or_comment(text, index)
        if skipped is not None:
            index = skipped
            continue
        if text[index] == opening:
            depth += 1
        elif text[index] == closing:
            depth -= 1
            if depth == 0:
                return index
        index += 1
    raise ValueError(f"unterminated {opening}{closing} block")


def top_level_comma(text: str) -> int:
    parens = brackets = braces = 0
    index = 0
    while index < len(text):
        skipped = skip_cpp_literal_or_comment(text, index)
        if skipped is not None:
            index = skipped
            continue
        char = text[index]
        if char == "(":
            parens += 1
        elif char == ")":
            parens -= 1
        elif char == "[":
            brackets += 1
        elif char == "]":
            brackets -= 1
        elif char == "{":
            braces += 1
        elif char == "}":
            braces -= 1
        elif char == "," and parens == brackets == braces == 0:
            return index
        index += 1
    raise ValueError("TEST_CASE has no top-level title/tag comma")


CPP_STRING = re.compile(r'(?:u8|u|U|L)?"(?:\\.|[^"\\])*"', re.DOTALL)


def concatenated_cpp_strings(text: str) -> str:
    values: list[str] = []
    for match in CPP_STRING.finditer(text):
        token = match.group(0)
        token = token[token.find('"'):]
        value = ast.literal_eval(token)
        if not isinstance(value, str):
            raise ValueError("non-string TEST_CASE argument")
        values.append(value)
    if not values:
        raise ValueError("TEST_CASE argument contains no string literal")
    return "".join(values)


def parse_test_implementations(errors: list[str]) -> dict[str, list[TestImplementation]]:
    result: dict[str, list[TestImplementation]] = {}
    for path in sorted((ROOT / "tests").rglob("*.cpp")):
        if "fuzz" in path.relative_to(ROOT / "tests").parts:
            continue
        text = path.read_text(encoding="utf-8")
        index = 0
        while index < len(text):
            skipped = skip_cpp_literal_or_comment(text, index)
            if skipped is not None:
                index = skipped
                continue
            if text[index].isalpha() or text[index] == "_":
                end = index + 1
                while end < len(text) and (text[end].isalnum() or text[end] == "_"):
                    end += 1
                identifier = text[index:end]
                cursor = end
                while cursor < len(text) and text[cursor].isspace():
                    cursor += 1
                if identifier == "TEST_CASE" and cursor < len(text) and text[cursor] == "(":
                    try:
                        close = matching_delimiter(text, cursor, "(", ")")
                        arguments = text[cursor + 1:close]
                        comma = top_level_comma(arguments)
                        title = concatenated_cpp_strings(arguments[:comma])
                        tags = concatenated_cpp_strings(arguments[comma + 1:])
                        body_start = close + 1
                        while body_start < len(text) and text[body_start].isspace():
                            body_start += 1
                        if body_start >= len(text) or text[body_start] != "{":
                            raise ValueError("TEST_CASE body does not start with {")
                        body_end = matching_delimiter(text, body_start, "{", "}")
                    except (ValueError, SyntaxError) as exc:
                        fail(errors, f"{path.relative_to(ROOT)}:{text.count(chr(10), 0, index) + 1}: {exc}")
                        index = end
                        continue
                    id_match = re.match(rf"({TEST_ID_PATTERN})(?:\.|\s|$)", title)
                    if id_match is not None:
                        implementation = TestImplementation(
                            id_match.group(1), title, tags, path, text[body_start:body_end + 1]
                        )
                        result.setdefault(implementation.test_id, []).append(implementation)
                    index = body_end + 1
                    continue
                index = end
                continue
            index += 1
    return result


def normalized_symbol(value: str) -> str:
    value = re.sub(r"<[^<>]*>", "", value)
    namespaced = re.findall(r"(?:[A-Za-z_]\w*::)+(?:operator[^\s(]+|~?[A-Za-z_]\w*|\*)", value)
    if namespaced:
        return namespaced[-1]
    operator = re.search(r"operator[^\s(]+", value)
    if operator:
        return operator.group(0)
    value = value.removeprefix("legacy ").strip()
    callables = re.findall(r"\b([A-Za-z_]\w*)\s*\(", value)
    if callables:
        return callables[-1]
    bare = re.match(r"([A-Za-z_]\w*)", value)
    return bare.group(1) if bare else value


def callable_shape(value: str) -> tuple[bool, str | None]:
    """Return template-ness and a normalized parameter shape when specified."""
    base = normalized_symbol(value)
    location = value.rfind(base)
    if location < 0:
        return False, None
    cursor = location + len(base)
    while cursor < len(value) and value[cursor].isspace():
        cursor += 1

    templated = bool(re.search(r"\btemplate\s*<", value[:location]))
    if cursor < len(value) and value[cursor] == "<":
        templated = True
        depth = 0
        while cursor < len(value):
            if value[cursor] == "<":
                depth += 1
            elif value[cursor] == ">":
                depth -= 1
                if depth == 0:
                    cursor += 1
                    break
            cursor += 1
        while cursor < len(value) and value[cursor].isspace():
            cursor += 1

    if cursor >= len(value) or value[cursor] != "(":
        return templated, None
    end = matching_delimiter(value, cursor, "(", ")")
    parameters = re.sub(r"\s+", "", value[cursor + 1:end])
    parameters = re.sub(r"[A-Za-z_]\w*\.\.\.", "...", parameters)
    return templated, parameters


def record_names_symbol(
    record_symbol: str,
    purpose_symbol: str,
    overload_counts: Counter[str],
) -> bool:
    record = normalized_symbol(record_symbol)
    purpose = normalized_symbol(purpose_symbol)
    if "*" in record:
        return fnmatch.fnmatchcase(purpose, record)
    if record != purpose:
        return False
    if overload_counts[purpose] == 1:
        return True

    record_template, record_parameters = callable_shape(record_symbol)
    purpose_template, purpose_parameters = callable_shape(purpose_symbol)
    if record_template != purpose_template:
        return False
    # A table label may omit empty parentheses; treat that as the same zero-arity
    # shape, but never let an entirely bare overloaded name cover all overloads.
    if record_parameters == "" and purpose_parameters is None:
        purpose_parameters = ""
    if purpose_parameters == "" and record_parameters is None:
        record_parameters = ""
    if record_parameters is None or purpose_parameters is None:
        return False
    return record_parameters == purpose_parameters


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--junit", action="append", default=[], type=Path,
        help="fresh CTest --output-junit result (repeatable) proving Routine execution",
    )
    args = parser.parse_args(argv)
    errors: list[str] = []
    try:
        document = json.loads(TRACEABILITY.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"traceability: cannot read {TRACEABILITY}: {exc}", file=sys.stderr)
        return 2
    if not isinstance(document, dict):
        print("traceability: root must be an object", file=sys.stderr)
        return 2

    required = {
        "schema_version", "snapshot", "purpose_map_sha256", "routine_policy",
        "catalogues", "multi_case_implementations", "blocked_implementation_slices",
        "function_groups",
    }
    missing_root = sorted(required - set(document))
    if missing_root:
        fail(errors, f"traceability is missing required root fields: {missing_root}")
    if document.get("schema_version") != 2:
        fail(errors, "traceability schema_version must be 2")
    policy = document.get("routine_policy")
    if not isinstance(policy, dict) or not nonempty(policy.get("gate")) or not nonempty(policy.get("not_a_gate")):
        fail(errors, "routine_policy requires nonempty gate and not_a_gate strings")

    snapshot = document.get("snapshot")
    if not isinstance(snapshot, str) or re.fullmatch(r"[0-9a-f]{64}", snapshot) is None:
        fail(errors, "traceability snapshot must be a lowercase 64-character SHA-256")
        snapshot = ""
    manifest_text = SNAPSHOT_MANIFEST.read_text(encoding="utf-8")
    manifest_snapshot = re.search(r"^# Snapshot-SHA256: ([0-9a-f]{64})$", manifest_text, re.MULTILINE)
    if manifest_snapshot is None or manifest_snapshot.group(1) != snapshot:
        fail(errors, "traceability snapshot does not match source-snapshot.sha256")

    configured = document.get("catalogues")
    if not isinstance(configured, list) or not all(nonempty(item) for item in configured):
        fail(errors, "catalogues must be a nonempty repository-relative path list")
        configured = []
    actual_catalogues = {
        path.relative_to(ROOT).as_posix()
        for path in (ROOT / "tests/spec/catalogue").glob("*.yaml")
    }
    if set(configured) != actual_catalogues or len(configured) != len(set(configured)):
        fail(errors, f"catalogue list mismatch: expected {sorted(actual_catalogues)}, got {configured}")

    records: dict[str, CatalogueRecord] = {}
    for relative in configured:
        path = ROOT / relative
        if not path.is_file():
            fail(errors, f"catalogue does not exist: {relative}")
            continue
        for test_id, record in catalogue_records(path, snapshot, errors).items():
            if test_id in records:
                fail(errors, f"test record appears in multiple catalogues: {test_id}")
            records[test_id] = record

    implementations = parse_test_implementations(errors)
    for test_id, record in sorted(records.items()):
        if record.status in EXECUTABLE_STATUSES and test_id not in implementations:
            fail(errors, f"{test_id} is {record.status} but has no compiled-source TEST_CASE")
        if record.status not in EXECUTABLE_STATUSES | {"BLOCKED"}:
            fail(errors, f"{test_id} has unsupported status {record.status}")

    multi_entries = document.get("multi_case_implementations", [])
    if not isinstance(multi_entries, list):
        fail(errors, "multi_case_implementations must be a list")
        multi_entries = []
    multi_counts: dict[str, int] = {}
    for entry in multi_entries:
        if not isinstance(entry, dict):
            fail(errors, "every multi-case approval must be an object")
            continue
        test_id = entry.get("id")
        count = entry.get("exact_cases")
        if (not isinstance(test_id, str) or TEST_ID.fullmatch(test_id) is None or
                test_id in multi_counts or not isinstance(count, int) or count < 2 or
                not nonempty(entry.get("reason"))):
            fail(errors, f"invalid multi-case implementation approval: {entry!r}")
            continue
        multi_counts[test_id] = count

    slice_entries = document.get("blocked_implementation_slices", [])
    if not isinstance(slice_entries, list):
        fail(errors, "blocked_implementation_slices must be a list")
        slice_entries = []
    slices: dict[str, dict[str, object]] = {}
    for entry in slice_entries:
        if not isinstance(entry, dict):
            fail(errors, "every blocked implementation slice must be an object")
            continue
        test_id = entry.get("id")
        marker = entry.get("title_marker")
        scope = entry.get("scope")
        if (not isinstance(test_id, str) or TEST_ID.fullmatch(test_id) is None or
                test_id in slices or not nonempty(marker) or not nonempty(scope)):
            fail(errors, f"invalid blocked implementation slice: {entry!r}")
            continue
        if records.get(test_id, CatalogueRecord("", (), ROOT)).status != "BLOCKED":
            fail(errors, f"blocked slice {test_id} does not map to a BLOCKED record")
        slices[test_id] = entry

    for test_id, cases in sorted(implementations.items()):
        expected_count = multi_counts.get(test_id, 1)
        if len(cases) != expected_count:
            fail(errors, f"{test_id} has {len(cases)} TEST_CASEs; expected exactly {expected_count}")
        if len({case.title for case in cases}) != len(cases):
            fail(errors, f"{test_id} TEST_CASE titles are not unique")
        record = records.get(test_id)
        if record is None:
            fail(errors, f"implemented {test_id} has no catalogue record")
            continue
        if record.status == "BLOCKED":
            entry = slices.get(test_id)
            if entry is None:
                fail(errors, f"BLOCKED record {test_id} is implemented without a narrow slice")
            else:
                marker = str(entry["title_marker"]).strip()
                matching = [case for case in cases if marker in case.title]
                if len(matching) != 1:
                    fail(errors, f"blocked slice {test_id} marker {marker!r} matches {len(matching)} titles")
    for test_id in set(slices) | set(multi_counts):
        if test_id not in implementations:
            fail(errors, f"approved special implementation {test_id} has no TEST_CASE")

    purpose_bytes = PURPOSE_MAP.read_bytes().replace(b"\r\n", b"\n")
    expected_hash = document.get("purpose_map_sha256")
    actual_hash = hashlib.sha256(purpose_bytes).hexdigest()
    if expected_hash != actual_hash:
        fail(errors, f"intended-purpose-v1.md hash mismatch: expected {expected_hash}, got {actual_hash}")
    inventory = purpose_symbols(purpose_bytes.decode("utf-8"))
    if len(inventory) != len(set(inventory)):
        fail(errors, "intended-purpose inventory contains duplicate callable labels")
    overload_counts = Counter(normalized_symbol(symbol) for symbol in inventory)

    groups = document.get("function_groups")
    if not isinstance(groups, list):
        fail(errors, "function_groups must be a list")
        groups = []
    mapped: dict[str, int] = {}
    tested_count = blocked_count = 0
    tested_test_ids: set[str] = set()
    for index, group in enumerate(groups):
        label = f"function_groups[{index}]"
        if not isinstance(group, dict):
            fail(errors, f"{label} must be an object")
            continue
        symbols = group.get("symbols")
        disposition = group.get("disposition")
        if (not isinstance(symbols, list) or not symbols or
                not all(nonempty(symbol) for symbol in symbols)):
            fail(errors, f"{label}.symbols must be a nonempty string list")
            continue
        if disposition not in {"TESTED", "BLOCKED"}:
            fail(errors, f"{label}.disposition must be TESTED or BLOCKED")
            continue
        for symbol in symbols:
            if symbol in mapped:
                fail(errors, f"function is mapped more than once: {symbol}")
            mapped[symbol] = index

        if disposition == "TESTED":
            tested_count += len(symbols)
            tests = group.get("tests")
            if (not isinstance(tests, list) or not tests or
                    not all(isinstance(test_id, str) and TEST_ID.fullmatch(test_id) for test_id in tests)):
                fail(errors, f"{label}.tests must be a nonempty valid test-id list")
                tests = []
            if not nonempty(group.get("scope")):
                fail(errors, f"{label}.scope must state substantive behavior")
            linkage = group.get("covered_via")
            linkage_symbols: set[str] = set()
            if linkage is not None:
                if (not isinstance(linkage, dict) or linkage.get("kind") != "reviewed-integration" or
                        not nonempty(linkage.get("reason")) or
                        not isinstance(linkage.get("symbols"), list) or
                        not linkage.get("symbols") or
                        not all(nonempty(item) for item in linkage["symbols"]) or
                        not isinstance(linkage.get("reviewers"), list) or
                        not linkage.get("reviewers") or
                        not all(nonempty(item) for item in linkage["reviewers"])):
                    fail(errors, f"{label}.covered_via is not a reviewed-integration record")
                else:
                    linkage_symbols = set(linkage["symbols"])
                    unknown_linkage = sorted(linkage_symbols - set(symbols))
                    if unknown_linkage:
                        fail(errors, f"{label}.covered_via names symbols outside its group: {unknown_linkage}")
            for test_id in tests:
                tested_test_ids.add(test_id)
                cases = implementations.get(test_id, [])
                if not cases:
                    fail(errors, f"{label} claims absent implementation {test_id}")
                elif not any(case.routine_scheduled for case in cases):
                    fail(errors, f"{label} cites {test_id}, which Routine never schedules")
            for symbol in symbols:
                direct = any(
                    test_id in records and any(
                        record_names_symbol(record_symbol, symbol, overload_counts)
                        for record_symbol in records[test_id].symbols
                    )
                    for test_id in tests
                )
                if not direct and symbol not in linkage_symbols:
                    fail(errors, f"{label} has no card-symbol linkage for {symbol}; covered_via required")
        else:
            blocked_count += len(symbols)
            if not nonempty(group.get("reason")):
                fail(errors, f"{label}.reason must explain the block")
            covered = group.get("covered_tests", [])
            if (not isinstance(covered, list) or
                    not all(isinstance(test_id, str) and TEST_ID.fullmatch(test_id) for test_id in covered)):
                fail(errors, f"{label}.covered_tests must be a valid test-id list")
                covered = []
            for test_id in covered:
                if test_id not in implementations:
                    fail(errors, f"{label} cites absent partial coverage {test_id}")

    missing = sorted(set(inventory) - set(mapped))
    extra = sorted(set(mapped) - set(inventory))
    if missing:
        fail(errors, f"unmapped intended-purpose functions: {missing}")
    if extra:
        fail(errors, f"function mappings absent from intended-purpose-v1.md: {extra}")

    junit_cases: dict[str, list[bool]] = {}
    for junit_path in args.junit:
        resolved = junit_path if junit_path.is_absolute() else ROOT / junit_path
        try:
            junit_root = ET.parse(resolved).getroot()
        except (OSError, ET.ParseError) as exc:
            fail(errors, f"cannot read CTest JUnit evidence {resolved}: {exc}")
            continue
        for testcase in junit_root.iter("testcase"):
            name = testcase.get("name", "")
            skipped = testcase.find("skipped") is not None or testcase.get("status") == "notrun"
            junit_cases.setdefault(name, []).append(not skipped)

    if args.junit:
        for test_id in sorted(tested_test_ids):
            for case in implementations.get(test_id, []):
                matching = junit_cases.get(case.routine_ctest_name, [])
                if not matching:
                    fail(
                        errors,
                        "TESTED case was not discovered under its exact Routine "
                        f"CTest name: {case.routine_ctest_name}",
                    )
                elif not any(matching):
                    fail(
                        errors,
                        "every Routine execution skipped TESTED case: "
                        f"{case.routine_ctest_name}",
                    )

    if errors:
        print("TDFA semantic traceability FAILED:", file=sys.stderr)
        for error in errors:
            print(f"  - {error}", file=sys.stderr)
        return 1
    executed_titles = sum(any(values) for values in junit_cases.values())
    suffix = f", {executed_titles} Routine-executed Catch titles." if args.junit else "."
    print(
        "TDFA semantic traceability OK: "
        f"{len(records)} catalogue records, {len(implementations)} implemented test IDs, "
        f"{tested_count} substantively tested functions, {blocked_count} explicitly blocked functions"
        + suffix
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
