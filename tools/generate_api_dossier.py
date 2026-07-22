#!/usr/bin/env python3
"""Create or validate TDFA's implementation-blind API authoring dossier.

The source tree is never modified.  Normal operation writes generated text to
stdout; --check only reads the repository and validates the frozen snapshot.
This deliberately uses a small lexical redactor rather than a C++ compiler: the
result is documentation, must not compile, and must not expose function bodies.
"""

from __future__ import annotations

import argparse
import hashlib
import re
import sys
from dataclasses import dataclass
from pathlib import Path, PurePosixPath


GENERATOR_VERSION = "1"
SOURCE_SUFFIXES = frozenset({".cpp", ".hpp"})
MANIFEST_RELATIVE = PurePosixPath("tests/spec/source-snapshot.sha256")
DOSSIER_RELATIVE = PurePosixPath("tests/spec/api-dossier.md")


@dataclass(frozen=True, order=True)
class SnapshotEntry:
    path: str
    digest: str


def repository_root(explicit: str | None) -> Path:
    if explicit:
        root = Path(explicit).resolve()
    else:
        root = Path(__file__).resolve().parents[1]
    if not (root / "src").is_dir():
        raise ValueError(f"not a TDFA repository root: {root}")
    return root


def source_paths(root: Path) -> list[Path]:
    return sorted(
        (path for path in (root / "src").rglob("*")
         if path.is_file() and path.suffix.lower() in SOURCE_SUFFIXES),
        key=lambda path: path.relative_to(root).as_posix(),
    )


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def current_snapshot(root: Path) -> list[SnapshotEntry]:
    return [
        SnapshotEntry(path.relative_to(root).as_posix(), sha256(path))
        for path in source_paths(root)
    ]


def snapshot_id(entries: list[SnapshotEntry]) -> str:
    canonical = "".join(f"{entry.digest}  {entry.path}\n" for entry in entries)
    return hashlib.sha256(canonical.encode("utf-8")).hexdigest()


def render_manifest(entries: list[SnapshotEntry]) -> str:
    return "\n".join(
        [
            "# TDFA source snapshot; raw-byte SHA-256; paths are repository-relative.",
            f"# Generator-Version: {GENERATOR_VERSION}",
            f"# Snapshot-SHA256: {snapshot_id(entries)}",
            "# Scope: every regular src/**/*.hpp and src/**/*.cpp file, sorted by path.",
            *(f"{entry.digest}  {entry.path}" for entry in entries),
            "",
        ]
    )


def parse_manifest(path: Path) -> tuple[str | None, list[SnapshotEntry]]:
    declared_id: str | None = None
    entries: list[SnapshotEntry] = []
    entry_pattern = re.compile(r"^([0-9a-f]{64})  (src/.+)$")
    for line_number, raw_line in enumerate(path.read_text(encoding="utf-8").splitlines(), 1):
        line = raw_line.strip()
        if line.startswith("# Snapshot-SHA256:"):
            declared_id = line.partition(":")[2].strip()
        if not line or line.startswith("#"):
            continue
        match = entry_pattern.fullmatch(line)
        if not match:
            raise ValueError(f"{path}:{line_number}: malformed manifest line")
        entries.append(SnapshotEntry(match.group(2), match.group(1)))
    if entries != sorted(entries):
        raise ValueError(f"{path}: entries are not sorted by path")
    if len({entry.path for entry in entries}) != len(entries):
        raise ValueError(f"{path}: duplicate path")
    return declared_id, entries


def validate_manifest(root: Path, manifest: Path) -> list[SnapshotEntry]:
    declared_id, expected = parse_manifest(manifest)
    actual = current_snapshot(root)
    errors: list[str] = []
    expected_by_path = {entry.path: entry.digest for entry in expected}
    actual_by_path = {entry.path: entry.digest for entry in actual}

    for path in sorted(expected_by_path.keys() - actual_by_path.keys()):
        errors.append(f"missing source file: {path}")
    for path in sorted(actual_by_path.keys() - expected_by_path.keys()):
        errors.append(f"unrecorded source file: {path}")
    for path in sorted(expected_by_path.keys() & actual_by_path.keys()):
        if expected_by_path[path] != actual_by_path[path]:
            errors.append(
                f"changed source file: {path}\n"
                f"  expected {expected_by_path[path]}\n"
                f"  actual   {actual_by_path[path]}"
            )

    computed_id = snapshot_id(expected)
    if declared_id != computed_id:
        errors.append(
            "manifest snapshot id mismatch\n"
            f"  declared {declared_id or '<missing>'}\n"
            f"  computed {computed_id}"
        )
    if errors:
        raise ValueError("snapshot validation failed:\n" + "\n".join(errors))
    return expected


def validate_dossier(root: Path, entries: list[SnapshotEntry]) -> None:
    dossier = root / Path(DOSSIER_RELATIVE.as_posix())
    actual = dossier.read_text(encoding="utf-8").replace("\r\n", "\n").rstrip("\n")
    expected = render_dossier(root, entries).replace("\r\n", "\n").rstrip("\n")
    if actual != expected:
        raise ValueError(
            f"{DOSSIER_RELATIVE.as_posix()} does not match the frozen source snapshot; "
            "review `--print-dossier` output"
        )


def is_literal_quote(source: str, index: int) -> bool:
    if source[index] == '"':
        return True
    if source[index] != "'":
        return False
    before = source[index - 1] if index else ""
    after = source[index + 1] if index + 1 < len(source) else ""
    return not (before.isalnum() and after.isalnum())


def strip_comments(source: str) -> str:
    """Remove C/C++ comments while preserving literals and line structure."""
    out: list[str] = []
    index = 0
    state = "code"
    quote = ""
    while index < len(source):
        char = source[index]
        nxt = source[index + 1] if index + 1 < len(source) else ""
        if state == "code":
            if char == "/" and nxt == "/":
                state = "line_comment"
                out.extend("  ")
                index += 2
                continue
            if char == "/" and nxt == "*":
                state = "block_comment"
                out.extend("  ")
                index += 2
                continue
            if is_literal_quote(source, index):
                state = "literal"
                quote = char
            out.append(char)
            index += 1
            continue
        if state == "line_comment":
            if char == "\n":
                state = "code"
                out.append(char)
            else:
                out.append(" ")
            index += 1
            continue
        if state == "block_comment":
            if char == "*" and nxt == "/":
                state = "code"
                out.extend("  ")
                index += 2
            else:
                out.append("\n" if char == "\n" else " ")
                index += 1
            continue

        out.append(char)
        if char == "\\" and index + 1 < len(source):
            out.append(source[index + 1])
            index += 2
            continue
        if char == quote:
            state = "code"
        index += 1
    return "".join(out)


def matching_brace(source: str, opening: int) -> int:
    depth = 0
    state = "code"
    quote = ""
    for index in range(opening, len(source)):
        char = source[index]
        if state == "literal":
            if char == "\\":
                state = "escape"
            elif char == quote:
                state = "code"
            continue
        if state == "escape":
            state = "literal"
            continue
        if is_literal_quote(source, index):
            state = "literal"
            quote = char
        elif char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return index
    raise ValueError("unbalanced brace in source")


SCOPE_PREFIX = re.compile(
    r"(?:^|\s)(?:class|struct|union|enum(?:\s+class)?|namespace)\s+[_A-Za-z]"
)


def scope_kind(prefix: str) -> str | None:
    match = re.search(
        r"(?:^|\s)(class|struct|union|enum(?:\s+class)?|namespace)\s+[_A-Za-z]",
        prefix,
    )
    return match.group(1).split()[0] if match else None


def initializer_end(source: str, start: int, enum_value: bool) -> int:
    """Find a declaration initializer delimiter without interpreting its value."""
    parens = brackets = braces = 0
    index = start
    while index < len(source):
        char = source[index]
        if is_literal_quote(source, index):
            quote = char
            index += 1
            while index < len(source):
                if source[index] == "\\":
                    index += 2
                    continue
                if source[index] == quote:
                    index += 1
                    break
                index += 1
            continue
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
            if braces:
                braces -= 1
            elif enum_value and not parens and not brackets:
                return index
        elif not parens and not brackets and not braces:
            if char == ";" or (enum_value and char == ","):
                return index
        index += 1
    raise ValueError("unterminated declaration initializer")


def redact_bodies(source: str) -> str:
    """Keep declaration scopes but replace executable bodies and initializers."""
    source = strip_comments(source)
    out: list[str] = []
    statement_start = 0
    index = 0
    paren_depth = 0
    scopes: list[str] = []
    while index < len(source):
        char = source[index]
        if char == "#" and (index == 0 or source[index - 1] == "\n"):
            end = source.find("\n", index)
            if end < 0:
                break
            directive = source[index:end].strip()
            macro = re.match(r"#\s*define\s+([_A-Za-z]\w*)", directive)
            if macro:
                out.append(f"#define {macro.group(1)} <replacement omitted>\n")
            index = end + 1
            statement_start = index
            continue
        if is_literal_quote(source, index):
            quote = char
            out.append("<literal>")
            index += 1
            while index < len(source):
                if source[index] == "\\":
                    index += 2
                    continue
                if source[index] == quote:
                    index += 1
                    break
                index += 1
            continue
        if char == "(":
            paren_depth += 1
        elif char == ")":
            paren_depth = max(0, paren_depth - 1)
        elif char == "{" and paren_depth == 0:
            prefix = source[statement_start:index]
            kind = scope_kind(prefix)
            if kind:
                out.append(char)
                scopes.append(kind)
                index += 1
                statement_start = index
                continue
            close = matching_brace(source, index)
            if ")" in prefix or re.search(r"\b(?:operator|~?[_A-Za-z]\w*)\s*$", prefix):
                out.append("; /* body omitted */")
            else:
                out.append("{ <initializer omitted> }")
            index = close + 1
            statement_start = index
            continue
        elif char == "=" and paren_depth == 0:
            prefix = source[statement_start:index].strip()
            previous = source[index - 1] if index else ""
            following = source[index + 1] if index + 1 < len(source) else ""
            if (
                not prefix.startswith("using ")
                and not prefix.endswith("operator")
                and previous not in "=!<>"
                and following != "="
            ):
                end = initializer_end(source, index + 1, bool(scopes and scopes[-1] == "enum"))
                out.append("= <initializer omitted>")
                index = end
                continue
        elif char == "}":
            if scopes:
                scopes.pop()
            statement_start = index + 1
        elif char == ";":
            statement_start = index + 1
        out.append(char)
        index += 1

    text = "".join(out)
    text = re.sub(
        r"(?ms)(^[ \t]*(?:(?:constexpr|explicit|inline)\s+)*"
        r"~?[_A-Za-z]\w*\s*\([^;\n]*\)(?:\s*noexcept)?)"
        r"\s*:\s*.*?(;\s*/\* body omitted \*/)",
        r"\1\n\2",
        text,
    )
    text = re.sub(r"(?m)^[ \t]+$", "", text)
    text = re.sub(r"\n{3,}", "\n\n", text)
    return text.strip()


def render_dossier(root: Path, entries: list[SnapshotEntry]) -> str:
    lines = [
        "# TDFA Sanitized API Dossier",
        "",
        f"Snapshot: `{snapshot_id(entries)}`  ",
        f"Generator version: `{GENERATOR_VERSION}`",
        "",
        "> This is intentionally non-compilable authoring input. Source comments,",
        "> literals, initializers, and executable bodies are omitted. It states no",
        "> behavioral contract; use the separately approved contract cards.",
        "",
    ]
    for entry in entries:
        if not entry.path.endswith(".hpp"):
            continue
        path = root / entry.path
        lines.extend(
            [
                f"## `{entry.path}`",
                "",
                "```cpp",
                redact_bodies(path.read_text(encoding="utf-8-sig")),
                "```",
                "",
            ]
        )
    return "\n".join(lines)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", help="repository root; defaults to script parent")
    parser.add_argument(
        "--manifest",
        help=f"manifest path; defaults to {MANIFEST_RELATIVE.as_posix()}",
    )
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--check", action="store_true", help="validate the frozen snapshot")
    mode.add_argument("--print-manifest", action="store_true", help="print current manifest")
    mode.add_argument(
        "--print-dossier",
        action="store_true",
        help="validate the snapshot, then print its sanitized dossier",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    try:
        root = repository_root(args.root)
        manifest = (
            Path(args.manifest).resolve()
            if args.manifest
            else root / Path(MANIFEST_RELATIVE.as_posix())
        )
        if args.print_manifest:
            sys.stdout.write(render_manifest(current_snapshot(root)))
            return 0
        entries = validate_manifest(root, manifest)
        if args.print_dossier:
            sys.stdout.write(render_dossier(root, entries))
        else:
            validate_dossier(root, entries)
            print(f"snapshot ok: {snapshot_id(entries)} ({len(entries)} files)")
        return 0
    except (OSError, UnicodeError, ValueError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
