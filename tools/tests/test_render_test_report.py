from __future__ import annotations

import contextlib
import importlib.util
import io
import json
import re
import sys
import tempfile
import unicodedata
import unittest
from pathlib import Path


MODULE_PATH = Path(__file__).resolve().parents[1] / "render_test_report.py"
SPEC = importlib.util.spec_from_file_location("render_test_report", MODULE_PATH)
assert SPEC is not None and SPEC.loader is not None
reporter = importlib.util.module_from_spec(SPEC)
sys.modules[SPEC.name] = reporter
SPEC.loader.exec_module(reporter)


def junit_document(cases: str, *, tests: int = 1, failures: int = 0) -> str:
    return f"""<?xml version="1.0" encoding="UTF-8"?>
<testsuite name="probe" tests="{tests}" failures="{failures}" disabled="0" skipped="0" time="1">
{cases}
</testsuite>
"""


def testcase(
    name: str,
    *,
    status: str = "run",
    marker: str = "",
    output: str = "",
    time: str = "0.25",
) -> str:
    return f"""  <testcase name="{name}" classname="{name}" time="{time}" status="{status}">
    {marker}
    <properties><property name="cmake_labels" value="fast"/></properties>
    <system-out>{output}</system-out>
  </testcase>"""


def display_cells(text: str) -> int:
    plain = re.sub(r"\x1b\[[0-9;]*m", "", text)
    cells = 0
    for character in plain:
        if unicodedata.combining(character):
            continue
        if unicodedata.category(character) in {"Cc", "Cf"}:
            continue
        cells += 2 if unicodedata.east_asian_width(character) in {"W", "F"} else 1
    return cells


class JunitParsingTests(unittest.TestCase):
    def test_ctest_outcomes_are_not_flattened_to_skip(self) -> None:
        cases = "\n".join(
            [
                testcase("pass", status="run"),
                testcase(
                    "timeout",
                    status="fail",
                    marker='<failure message="Timeout"/>',
                    output="before timeout",
                ),
                testcase(
                    "consumer",
                    status="notrun",
                    marker='<skipped message="Fixture dependency failed"/>',
                ),
                testcase(
                    "missing",
                    status="notrun",
                    marker='<skipped message="Unable to find executable"/>',
                ),
                testcase(
                    "ordinary skip",
                    status="notrun",
                    marker='<skipped message="SKIP_RETURN_CODE=4"/>',
                ),
                testcase("disabled", status="disabled", output="Disabled"),
            ]
        )
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "synthetic.xml"
            path.write_text(
                junit_document(cases, tests=6, failures=1), encoding="utf-8"
            )
            evidence = reporter.parse_junit(f"probe={path}")

        self.assertEqual(
            [case.status for case in evidence.cases],
            ["pass", "fail", "blocked", "infra", "skip", "disabled"],
        )
        timeout = evidence.cases[1]
        self.assertEqual(timeout.failure_message, "Timeout")
        self.assertIn("CTest failure: Timeout", timeout.output)

    def test_zero_test_junit_is_visibly_suspicious(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "empty.xml"
            path.write_text(junit_document("", tests=0), encoding="utf-8")
            evidence = reporter.parse_junit(str(path))
        self.assertEqual(evidence.cases, [])
        self.assertTrue(any("zero tests" in warning for warning in evidence.warnings))


class ClassificationTests(unittest.TestCase):
    def test_fixture_checksum_failure_requires_matching_eol_evidence(self) -> None:
        raw_hash = "1" * 64
        lf_hash = "2" * 64
        fixture = reporter.FixtureIssue(
            path="tests/data/oracle_cases.tsv",
            cr_count=46,
            lf_count=46,
            raw_sha256=raw_hash,
            lf_sha256=lf_hash,
        )
        unrelated = reporter.TestCase(
            lane="debug-fast",
            name="fast::T-FIX-001 unrelated checksum mismatch",
            status="fail",
            duration=0.01,
            output=(
                "fixture scoped bytes do not match data_sha256\n"
                "path=tests/data/oracle_perft.tsv\n"
                f"raw={'3' * 64} normalized={'4' * 64}"
            ),
        )
        path_match = reporter.TestCase(
            lane="debug-fast",
            name="fast::T-FIX-002 matching fixture path",
            status="fail",
            duration=0.01,
            output=(
                "fixture scoped bytes do not match data_sha256\n"
                "path=tests/data/oracle_cases.tsv"
            ),
        )
        hash_match = reporter.TestCase(
            lane="release-deep",
            name="deep::T-FIX-003 matching fixture hashes",
            status="fail",
            duration=0.01,
            output=(
                "sha256_hex(whole_file) == file_hash\n"
                f"raw={raw_hash} normalized={lf_hash}"
            ),
        )

        issues = reporter.group_issues([unrelated, path_match, hash_match], [fixture])
        by_code = {issue.code: issue for issue in issues}
        self.assertEqual(by_code["FIXTURE_INTEGRITY"].severity, "fail")
        self.assertEqual(by_code["FIXTURE_INTEGRITY"].cases, [unrelated])
        self.assertEqual(by_code["FIXTURE_EOL"].severity, "blocked")
        self.assertEqual(by_code["FIXTURE_EOL"].cases, [path_match, hash_match])

    def test_ctest_native_outcomes_classify_and_count_by_lane(self) -> None:
        crash_tokens = (
            "SEGFAULT",
            "ILLEGAL",
            "INTERRUPT",
            "NUMERICAL",
            "OTHER_FAULT",
            "CHILD_ABORTED",
        )
        infra_tokens = ("BAD_COMMAND", "FAILED_TO_START")
        cases = [
            reporter.TestCase(
                lane="debug-fast",
                name=f"fast::T-CRASH-{index:03d} {token}",
                status="fail",
                duration=0.01,
                failure_message=token,
            )
            for index, token in enumerate(crash_tokens, start=1)
        ]
        cases.extend(
            reporter.TestCase(
                lane="debug-fast",
                name=f"fast::T-INFRA-{index:03d} {token}",
                status="fail",
                duration=0.01,
                failure_message=token,
            )
            for index, token in enumerate(infra_tokens, start=1)
        )

        issues = reporter.group_issues(cases, [])
        self.assertEqual(
            sum(issue.code == "PROCESS_CRASH" for issue in issues),
            len(crash_tokens),
        )
        self.assertEqual(
            sum(issue.code == "CTEST_INFRA" for issue in issues),
            len(infra_tokens),
        )
        self.assertTrue(
            all(
                issue.severity == "fail"
                for issue in issues
                if issue.code == "PROCESS_CRASH"
            )
        )
        self.assertTrue(
            all(
                issue.severity == "infra"
                for issue in issues
                if issue.code == "CTEST_INFRA"
            )
        )
        counts = reporter.evidence_counts(cases, issues)
        self.assertEqual(
            counts,
            {"pass": 0, "fail": 6, "blocked": 0, "infra": 2, "skip": 0},
        )
        lane = reporter.lane_lines(
            cases,
            issues,
            reporter.Theme(colour=False, unicode=False),
            100,
        )[0]
        self.assertIn("FAIL", lane)
        self.assertIn("6 fail", lane)
        self.assertIn("2 infra", lane)

    def test_fixture_cascades_collapse_into_one_blocked_issue(self) -> None:
        cases = [
            reporter.TestCase(
                lane="debug-fast",
                name=f"fast::T-FIX-{index:03d} fixture consumer",
                status="fail",
                duration=0.01,
                output="due to unexpected exception with message:\n  TSV contains CR",
            )
            for index in range(26)
        ]
        fixture = reporter.FixtureIssue(
            path="tests/data/oracle_cases.tsv",
            cr_count=46,
            lf_count=46,
            raw_sha256="a" * 64,
            lf_sha256="b" * 64,
        )
        issues = reporter.group_issues(cases, [fixture])
        self.assertEqual(len(issues), 1)
        self.assertEqual(issues[0].code, "FIXTURE_EOL")
        self.assertEqual(issues[0].severity, "blocked")
        self.assertEqual(len(issues[0].cases), 26)

    def test_promotion_rows_and_bitboard_values_are_preserved(self) -> None:
        promotion = reporter.TestCase(
            lane="debug-fast",
            name="fast::T-UTL-001 promotion",
            status="fail",
            duration=0.1,
            output=(
                "PieceType=1 expected=q actual=z\n"
                "PieceType=4 expected=r actual=z\n"
                "PieceType=2 expected=b actual=z\n"
                "PieceType=3 expected=n actual=z"
            ),
        )
        bitboard = reporter.TestCase(
            lane="release-deep",
            name="deep::T-BB-003 bit scan",
            status="fail",
            duration=0.1,
            output=(
                "corpus=singleton index=0 board=0x0000000000000001\n"
                "expected_first=0 expected_last=0\n"
                "GetLS1B=0x1 FindLS1B=0 FindMS1B=63\n"
                "a nonzero bit query/removal disagrees with the ordered-set model"
            ),
        )
        issues = reporter.group_issues([promotion, bitboard], [])
        theme = reporter.Theme(colour=False, unicode=False)
        promotion_issue = next(
            issue for issue in issues if issue.key == "PROMOTION_MAPPING"
        )
        bitboard_issue = next(
            issue for issue in issues if issue.key == "BIT_SCAN_MODEL"
        )

        promotion_text = "\n".join(
            reporter.promotion_lines(promotion_issue, theme, Path.cwd())
        )
        bitboard_text = "\n".join(
            reporter.bitboard_lines(bitboard_issue, theme, Path.cwd())
        )
        self.assertIn("Queen (1)", promotion_text)
        self.assertIn("Knight (3)", promotion_text)
        self.assertIn("FindMS1B", bitboard_text)
        self.assertIn("63", bitboard_text)
        self.assertIn("0x0000000000000001", bitboard_text)

    def test_uci_transcript_becomes_a_variant_matrix(self) -> None:
        output = """
variant-id=uci-extra-lf
operation=wait-for-uciok
diagnostic ordinal=3 outcome=timeout elapsed=10023ms message="response deadline expired"
summary stdout-bytes=0
cleanup-outcome=ok
variant-id=isready-padded
operation=wait-for-readyok
output ordinal=6 token="uciok"
diagnostic ordinal=8 outcome=timeout elapsed=10074ms message="response deadline expired"
summary stdout-bytes=108
cleanup-outcome=ok
"""
        variants = reporter.parse_uci_variants(output)
        self.assertEqual(
            [variant.name for variant in variants], ["uci-extra-lf", "isready-padded"]
        )
        self.assertEqual(variants[0].observed, "no stdout")
        self.assertEqual(variants[0].elapsed, "10.02s")
        self.assertEqual(variants[1].observed, "last token uciok")

    def test_cross_lane_uci_variants_retain_both_observations(self) -> None:
        debug = reporter.TestCase(
            lane="debug-fast",
            name="uci::T-APP-017.standard-slice protocol variants",
            status="fail",
            duration=10.02,
            output="""
variant-id=uci-padded-lf
operation=wait-for-uciok
diagnostic ordinal=3 outcome=timeout elapsed=10023ms message="response deadline expired"
summary stdout-bytes=0
cleanup-outcome=ok
""",
        )
        optimized = reporter.TestCase(
            lane="release-deep",
            name="uci::T-APP-017.standard-slice protocol variants",
            status="fail",
            duration=10.07,
            output="""
variant-id=uci-padded-lf
operation=wait-for-uciok
output ordinal=6 token="id"
diagnostic ordinal=8 outcome=timeout elapsed=10074ms message="response deadline expired"
summary stdout-bytes=108
cleanup-outcome=ok
""",
        )

        issues = reporter.group_issues([debug, optimized], [])
        self.assertEqual(len(issues), 1)
        rows = reporter.uci_lines(
            issues[0], reporter.Theme(colour=False, unicode=False)
        )
        debug_row = next(row for row in rows if "uci-padded-lf [dbg]" in row)
        optimized_row = next(row for row in rows if "uci-padded-lf [opt]" in row)
        self.assertIn("no stdout", debug_row)
        self.assertIn("last token id", optimized_row)

    def test_malicious_uci_cleanup_outcome_is_terminal_safe(self) -> None:
        case = reporter.TestCase(
            lane="debug-fast",
            name="uci::T-APP-017.standard-slice protocol variants",
            status="fail",
            duration=10.02,
            output=(
                "variant-id=uci-padded-lf\n"
                "operation=wait-for-uciok\n"
                "diagnostic ordinal=3 outcome=timeout elapsed=10023ms "
                'message="response deadline expired"\n'
                "summary stdout-bytes=0\n"
                "cleanup-outcome=owned\x1b[31m\x9b31m\n"
            ),
        )
        issues = reporter.group_issues([case], [])
        report = reporter.render_dashboard(
            mode="Routine",
            cases=[case],
            issues=issues,
            fixtures=[],
            summary=None,
            artifact_root=None,
            warnings=[],
            width=100,
            theme=reporter.Theme(colour=False, unicode=True),
            project_root=Path.cwd(),
            max_issues=8,
        )

        self.assertNotIn("\x1b", report)
        self.assertNotIn("\x9b", report)
        self.assertIn(r"\x1b", report)
        self.assertIn(r"\x9b", report)


class RenderingTests(unittest.TestCase):
    def test_external_failure_forces_fail_and_is_persisted(self) -> None:
        reason = "line coverage gate failed: 94.99% is below 95%"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            junit = root / "focused.xml"
            diagnostics = root / "diagnostics.json"
            junit.write_text(
                junit_document(testcase("fast::T-PASS-001 valid evidence"), tests=1),
                encoding="utf-8",
            )
            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                exit_code = reporter.main(
                    [
                        "--junit",
                        f"debug-fast={junit}",
                        "--project-root",
                        str(root),
                        "--mode",
                        "Coverage",
                        "--status",
                        "PASS",
                        "--failure-detail",
                        reason,
                        "--diagnostics-json",
                        str(diagnostics),
                        "--plain",
                        "--width",
                        "100",
                    ]
                )
            document = json.loads(diagnostics.read_text(encoding="utf-8"))

        report = stdout.getvalue()
        self.assertEqual(exit_code, 0)
        self.assertIn("COVERAGE | FAIL", report)
        self.assertIn("GATE FAILURES", report)
        self.assertIn(reason, report)
        self.assertEqual(document["schema_version"], 3)
        self.assertEqual(document["status"], "FAIL")
        self.assertEqual(document["external_failures"], [reason])

    def test_fail_on_test_failure_uses_raw_failure_before_blocking(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            fixture = root / "tests" / "data" / "fixture.tsv"
            fixture.parent.mkdir(parents=True)
            fixture.write_bytes(b"name\tvalue\r\nprobe\t1\r\n")
            junit = root / "debug-fast.xml"
            junit.write_text(
                junit_document(
                    testcase(
                        "fast::T-FIX-001 fixture consumer",
                        status="fail",
                        marker='<failure message="Failed"/>',
                        output="TSV contains CR",
                    ),
                    tests=1,
                    failures=1,
                ),
                encoding="utf-8",
            )

            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                exit_code = reporter.main(
                    [
                        "--junit",
                        f"debug-fast={junit}",
                        "--project-root",
                        str(root),
                        "--mode",
                        "Routine",
                        "--fail-on-test-failure",
                        "--plain",
                        "--width",
                        "100",
                    ]
                )

        self.assertEqual(exit_code, 1)
        self.assertIn("ROUTINE | BLOCKED", stdout.getvalue())
        self.assertIn("1 blocked", stdout.getvalue())

    def test_pass_override_cannot_weaken_derived_status(self) -> None:
        blocked_case = reporter.TestCase(
            lane="debug-fast",
            name="fast::T-FIX-001 fixture consumer",
            status="fail",
            duration=0.01,
            output="TSV contains CR",
        )
        blocked_issue = reporter.Issue(
            key="FIXTURE_EOL",
            code="FIXTURE_EOL",
            severity="blocked",
            title="Canonical fixture line endings",
            cases=[blocked_case],
        )
        skipped_case = reporter.TestCase(
            lane="debug-fast",
            name="fast::T-SKIP-001 unavailable capability",
            status="skip",
            duration=0.0,
        )
        running_case = reporter.TestCase(
            lane="debug-fast",
            name="fast::T-PASS-001 active pipeline",
            status="pass",
            duration=0.01,
        )
        scenarios = (
            ("BLOCKED", [blocked_case], [blocked_issue], None),
            ("SKIPPED", [skipped_case], [], None),
            ("RUNNING", [running_case], [], {"pipeline_status": "RUNNING"}),
        )

        for expected, cases, issues, summary in scenarios:
            with self.subTest(expected=expected):
                report = reporter.render_dashboard(
                    mode="Override",
                    cases=cases,
                    issues=issues,
                    fixtures=[],
                    summary=summary,
                    artifact_root=None,
                    warnings=[],
                    width=100,
                    theme=reporter.Theme(colour=False, unicode=False),
                    project_root=Path.cwd(),
                    max_issues=8,
                    status_override="PASS",
                )
                self.assertIn(f"OVERRIDE | {expected}", report)

    def test_untrusted_test_name_never_becomes_a_rerun_command(self) -> None:
        payload = "$(Write-Output PWNED); Remove-Item -Recurse C:\\"
        untrusted = reporter.TestCase(
            lane="debug-fast",
            name=f"malicious {payload}",
            status="fail",
            duration=0.01,
            output="sample.cpp:42: FAILED:\n  assertion failed",
        )
        untrusted_issues = reporter.group_issues([untrusted], [])
        untrusted_report = reporter.render_dashboard(
            mode="Routine",
            cases=[untrusted],
            issues=untrusted_issues,
            fixtures=[],
            summary=None,
            artifact_root=None,
            warnings=[],
            width=100,
            theme=reporter.Theme(colour=False, unicode=False),
            project_root=Path.cwd(),
            max_issues=8,
        )
        next_section = untrusted_report.split("  NEXT", 1)[1]
        self.assertIsNone(reporter.rerun_command(untrusted))
        self.assertNotIn("Rerun", next_section)
        self.assertNotIn("ctest", next_section)
        self.assertNotIn(payload, next_section)

        trusted = reporter.TestCase(
            lane="debug-fast",
            name=f"fast::T-SAFE-001 trusted evidence {payload}",
            status="fail",
            duration=0.01,
            output="sample.cpp:42: FAILED:\n  assertion failed",
        )
        trusted_issues = reporter.group_issues([trusted], [])
        trusted_report = reporter.render_dashboard(
            mode="Routine",
            cases=[trusted],
            issues=trusted_issues,
            fixtures=[],
            summary=None,
            artifact_root=None,
            warnings=[],
            width=100,
            theme=reporter.Theme(colour=False, unicode=False),
            project_root=Path.cwd(),
            max_issues=8,
        )
        trusted_next = trusted_report.split("  NEXT", 1)[1]
        self.assertEqual(
            reporter.rerun_command(trusted),
            "ctest --preset test-fast -R T-SAFE-001 --output-on-failure",
        )
        self.assertIn("Rerun", trusted_next)
        self.assertIn("-R T-SAFE-001", trusted_next)
        self.assertNotIn(payload, trusted_next)

    def test_summary_status_is_not_weakened_by_all_skipped_junit(self) -> None:
        skipped = reporter.TestCase(
            lane="debug-fast",
            name="fast::T-SKIP-001 unavailable capability",
            status="skip",
            duration=0.0,
        )
        for pipeline_status in ("FAIL", "RUNNING"):
            with self.subTest(pipeline_status=pipeline_status):
                report = reporter.render_dashboard(
                    mode="Routine",
                    cases=[skipped],
                    issues=[],
                    fixtures=[],
                    summary={"pipeline_status": pipeline_status, "stages": []},
                    artifact_root=None,
                    warnings=[],
                    width=100,
                    theme=reporter.Theme(colour=False, unicode=False),
                    project_root=Path.cwd(),
                    max_issues=8,
                )
                self.assertIn(f"ROUTINE | {pipeline_status}", report)

    def test_mixed_valid_and_missing_junit_is_failed_infrastructure(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            valid = root / "debug-fast.xml"
            missing = root / "release-deep.xml"
            valid.write_text(
                junit_document(
                    testcase("fast::T-PASS-001 valid evidence"),
                    tests=1,
                ),
                encoding="utf-8",
            )

            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                exit_code = reporter.main(
                    [
                        "--junit",
                        f"debug-fast={valid}",
                        "--junit",
                        f"release-deep={missing}",
                        "--project-root",
                        str(root),
                        "--mode",
                        "Routine",
                        "--plain",
                        "--width",
                        "100",
                    ]
                )

        report = stdout.getvalue()
        self.assertEqual(exit_code, 0)
        self.assertIn("ROUTINE | FAIL", report)
        self.assertIn("1 passed", report)
        self.assertIn("1 infra", report)
        self.assertIn("JUnit evidence is missing:", report)

    def test_explicit_status_override_is_rendered_by_api_and_cli(self) -> None:
        case = reporter.TestCase(
            lane="debug-fast",
            name="fast::T-PASS-001 valid evidence",
            status="pass",
            duration=0.25,
        )
        direct = reporter.render_dashboard(
            mode="Focused",
            cases=[case],
            issues=[],
            fixtures=[],
            summary=None,
            artifact_root=None,
            warnings=[],
            width=100,
            theme=reporter.Theme(colour=False, unicode=False),
            project_root=Path.cwd(),
            max_issues=8,
            status_override="FAIL",
        )
        self.assertIn("FOCUSED | FAIL", direct)

        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            junit = root / "focused.xml"
            junit.write_text(
                junit_document(
                    testcase("fast::T-PASS-001 valid evidence"),
                    tests=1,
                ),
                encoding="utf-8",
            )
            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                reporter.main(
                    [
                        "--junit",
                        f"debug-fast={junit}",
                        "--project-root",
                        str(root),
                        "--mode",
                        "Focused",
                        "--status",
                        "FAIL",
                        "--plain",
                        "--width",
                        "100",
                    ]
                )
        self.assertIn("FOCUSED | FAIL", stdout.getvalue())

    def test_all_skipped_run_and_lane_are_skipped(self) -> None:
        cases = [
            reporter.TestCase(
                lane="debug-fast",
                name="fast::T-SKIP-001 unavailable capability",
                status="skip",
                duration=0.0,
            ),
            reporter.TestCase(
                lane="debug-fast",
                name="fast::T-SKIP-002 disabled capability",
                status="disabled",
                duration=0.0,
            ),
        ]
        report = reporter.render_dashboard(
            mode="Routine",
            cases=cases,
            issues=[],
            fixtures=[],
            summary=None,
            artifact_root=None,
            warnings=[],
            width=100,
            theme=reporter.Theme(colour=False, unicode=False),
            project_root=Path.cwd(),
            max_issues=8,
        )
        self.assertIn("ROUTINE | SKIPPED", report)
        lane_line = next(line for line in report.splitlines() if "Debug | Fast" in line)
        self.assertIn("SKIPPED", lane_line)
        self.assertIn("2 skipped", lane_line)

    def test_all_pass_progress_bar_has_no_empty_cells(self) -> None:
        bar = reporter.progress_bar(
            {
                "pass": 7,
                "fail": 0,
                "blocked": 0,
                "infra": 0,
                "skip": 0,
            },
            23,
            reporter.Theme(colour=False, unicode=False),
        )
        self.assertEqual(bar, "#" * 23)
        self.assertNotIn(".", bar)

    def test_mixed_status_plain_progress_bar_uses_distinct_glyphs(self) -> None:
        bar = reporter.progress_bar(
            {
                "pass": 1,
                "fail": 1,
                "blocked": 1,
                "infra": 1,
                "skip": 1,
            },
            5,
            reporter.Theme(colour=False, unicode=False),
        )
        self.assertEqual(display_cells(bar), 5)
        self.assertEqual(len(set(bar)), 5)

    def test_summary_failure_dominates_blocked_only_junit(self) -> None:
        case = reporter.TestCase(
            lane="debug-fast",
            name="fast::T-FIX-001 fixture consumer",
            status="fail",
            duration=0.01,
            output="due to unexpected exception with message:\n  TSV contains CR",
        )
        fixture = reporter.FixtureIssue(
            path="tests/data/oracle_cases.tsv",
            cr_count=46,
            lf_count=46,
            raw_sha256="a" * 64,
            lf_sha256="b" * 64,
        )
        issues = reporter.group_issues([case], [fixture])
        report = reporter.render_dashboard(
            mode="Routine",
            cases=[case],
            issues=issues,
            fixtures=[fixture],
            summary={"pipeline_status": "FAIL", "stages": []},
            artifact_root=None,
            warnings=[],
            width=100,
            theme=reporter.Theme(colour=False, unicode=False),
            project_root=Path.cwd(),
            max_issues=8,
        )
        self.assertIn("ROUTINE | FAIL", report)
        self.assertIn("1 blocked", report)

    def test_cli_writes_atomic_diagnostics_json(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            junit = root / "debug-fast.xml"
            diagnostics = root / "nested" / "diagnostics.json"
            junit.write_text(
                junit_document(
                    testcase("fast::T-PASS-001 valid evidence"),
                    tests=1,
                ),
                encoding="utf-8",
            )

            stdout = io.StringIO()
            with contextlib.redirect_stdout(stdout):
                exit_code = reporter.main(
                    [
                        "--junit",
                        f"debug-fast={junit}",
                        "--project-root",
                        str(root),
                        "--mode",
                        "Routine",
                        "--diagnostics-json",
                        str(diagnostics),
                        "--plain",
                        "--width",
                        "100",
                    ]
                )

            document = json.loads(diagnostics.read_text(encoding="utf-8"))
            temporary_files = list(
                diagnostics.parent.glob(f".{diagnostics.name}.*.tmp")
            )

        self.assertEqual(exit_code, 0)
        self.assertEqual(document["schema_version"], 3)
        self.assertEqual(document["mode"], "Routine")
        self.assertEqual(document["counts"]["passed"], 1)
        self.assertEqual(temporary_files, [])
        self.assertIn("ROUTINE | PASS", stdout.getvalue())

    def test_nonfinite_junit_durations_do_not_crash_renderer(self) -> None:
        cases = "\n".join(
            [
                testcase("fast::T-TIME-001 nan duration", time="NaN"),
                testcase("fast::T-TIME-002 infinite duration", time="inf"),
            ]
        )
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "nonfinite.xml"
            path.write_text(
                junit_document(cases, tests=2),
                encoding="utf-8",
            )
            evidence = reporter.parse_junit(f"debug-fast={path}")

        report = reporter.render_dashboard(
            mode="Routine",
            cases=evidence.cases,
            issues=[],
            fixtures=[],
            summary=None,
            artifact_root=None,
            warnings=evidence.warnings,
            width=100,
            theme=reporter.Theme(colour=False, unicode=False),
            project_root=Path.cwd(),
            max_issues=8,
        )
        self.assertIn("2 tests", report)
        self.assertIn("Debug | Fast", report)

    def test_mixed_timezone_stage_timestamps_do_not_crash_renderer(self) -> None:
        summary = {
            "pipeline_status": "PASS",
            "stages": [
                {
                    "name": "aware-start",
                    "status": "PASS",
                    "started_utc": "2026-07-31T00:00:00+00:00",
                    "finished_utc": "2026-07-31T00:00:01",
                },
                {
                    "name": "naive-start",
                    "status": "PASS",
                    "started_utc": "2026-07-31T00:00:00",
                    "finished_utc": "2026-07-31T00:00:01+00:00",
                },
            ],
        }
        report = reporter.render_dashboard(
            mode="Routine",
            cases=[],
            issues=[],
            fixtures=[],
            summary=summary,
            artifact_root=None,
            warnings=[],
            width=100,
            theme=reporter.Theme(colour=False, unicode=False),
            project_root=Path.cwd(),
            max_issues=8,
        )
        self.assertIn("Aware Start", report)
        self.assertIn("Naive Start", report)

    def test_adversarial_fields_are_sanitized_and_width_bounded(self) -> None:
        case = reporter.TestCase(
            lane=("coverage\tlane-" + "very-long-" * 20),
            name="audit::T-CTRL-001 adversarial rendering",
            status="fail",
            duration=0.25,
            output="sample.cpp:42: FAILED:\n  expected\x00safe output",
        )
        issue = reporter.Issue(
            key="ADVERSARIAL",
            code="ASSERTION",
            severity="fail",
            title="issue\x00title\twith\rcontrols\x1b[31m-" + "long-" * 30,
            cases=[case],
        )
        summary = {
            "pipeline_status": "FAIL",
            "stages": [
                {
                    "name": "stage\nname-" + "long-" * 30,
                    "status": "FAIL",
                    "detail": "detail\twith\x1b[32m controls " + "wide-" * 30,
                }
            ],
        }
        artifact = "build/\x1b[35martifact-" + "deep-path-" * 30
        warning = "warning\rwith\tcontrols\x1b[33m-" + "wide-" * 30

        for width in (72, 100):
            with self.subTest(width=width):
                report = reporter.render_dashboard(
                    mode="Routine",
                    cases=[case],
                    issues=[issue],
                    fixtures=[],
                    summary=summary,
                    artifact_root=artifact,
                    warnings=[warning],
                    width=width,
                    theme=reporter.Theme(colour=False, unicode=False),
                    project_root=Path.cwd(),
                    max_issues=8,
                )
                self.assertEqual(
                    [line for line in report.splitlines() if len(line) > width],
                    [],
                )
                self.assertIsNone(re.search(r"[\x00-\x09\x0b-\x1f\x7f]", report))

    def test_coverage_lane_rerun_uses_coverage_build_tree(self) -> None:
        case = reporter.TestCase(
            lane="coverage",
            name="audit::T-COV-001 instrumented coverage failure",
            status="fail",
            duration=0.25,
            output="coverage_probe.cpp:42: FAILED:\n  observed a mismatch",
        )
        issues = reporter.group_issues([case], [])
        report = reporter.render_dashboard(
            mode="Coverage",
            cases=[case],
            issues=issues,
            fixtures=[],
            summary=None,
            artifact_root="build/coverage-gcc/coverage",
            warnings=[],
            width=100,
            theme=reporter.Theme(colour=False, unicode=False),
            project_root=Path.cwd(),
            max_issues=8,
        )
        self.assertIn("--test-dir build/coverage-gcc", report)
        self.assertIn("-R T-COV-001", report)
        self.assertNotIn("--preset test-fast", report)

    def test_existing_build_rerun_uses_exact_tree(self) -> None:
        case = reporter.TestCase(
            lane="debug-fast",
            name="fast::T-CLION-001 existing-tree failure",
            status="fail",
            duration=0.25,
            output="clion_probe.cpp:42: FAILED:\n  observed a mismatch",
        )
        issues = reporter.group_issues([case], [])
        build_directory = r"D:\coding\TDFA with spaces\cmake-build-debug"
        command = reporter.rerun_command(case, build_directory)
        self.assertEqual(
            command,
            'ctest --test-dir "D:\\coding\\TDFA with spaces\\cmake-build-debug" '
            "-R T-CLION-001 --output-on-failure",
        )
        report = reporter.render_dashboard(
            mode="Fast",
            cases=[case],
            issues=issues,
            fixtures=[],
            summary=None,
            artifact_root="build/verification/runs/example",
            warnings=[],
            width=120,
            theme=reporter.Theme(colour=False, unicode=False),
            project_root=Path.cwd(),
            max_issues=8,
            rerun_build_directory=build_directory,
        )
        self.assertIn(command, report)
        self.assertNotIn("--preset test-fast", report)

    def test_plain_box_rows_keep_border_after_unicode_ellipsis_fallback(self) -> None:
        case = reporter.TestCase(
            lane="debug-fast",
            name="fast::T-ELLIPSIS-001 boxed diagnostic",
            status="fail",
            duration=0.01,
            output=("diagnostic \u2026 remains bounded " * 12).strip(),
        )
        issue = reporter.Issue(
            key="ELLIPSIS",
            code="ASSERTION",
            severity="fail",
            title="Unicode ellipsis \u2026 inside a deliberately long boxed title",
            cases=[case],
        )
        report = reporter.render_dashboard(
            mode="Routine",
            cases=[case],
            issues=[issue],
            fixtures=[],
            summary=None,
            artifact_root=None,
            warnings=[],
            width=72,
            theme=reporter.Theme(colour=False, unicode=False),
            project_root=Path.cwd(),
            max_issues=8,
        )

        boxed_rows = [line for line in report.splitlines() if line.startswith("| ")]
        self.assertTrue(boxed_rows)
        self.assertTrue(all(line.endswith(" |") for line in boxed_rows))
        self.assertTrue(all(len(line) == 72 for line in boxed_rows))
        self.assertNotIn("\u2026", report)
        self.assertIn("...", report)

    def test_plain_coverage_stage_uses_ascii_greater_than_or_equal(self) -> None:
        report = reporter.render_dashboard(
            mode="Coverage",
            cases=[],
            issues=[],
            fixtures=[],
            summary={
                "pipeline_status": "PASS",
                "stages": [{"name": "line-coverage-95", "status": "PASS"}],
            },
            artifact_root=None,
            warnings=[],
            width=100,
            theme=reporter.Theme(colour=False, unicode=False),
            project_root=Path.cwd(),
            max_issues=8,
        )
        self.assertIn("Line coverage >= 95%", report)

    def test_direct_gcovr_summary_shape_renders_line_coverage(self) -> None:
        report = reporter.render_dashboard(
            mode="Coverage",
            cases=[],
            issues=[],
            fixtures=[],
            summary={
                "line_percent": 96.25,
                "line_covered": 385,
                "line_total": 400,
            },
            artifact_root=None,
            warnings=[],
            width=100,
            theme=reporter.Theme(colour=False, unicode=False),
            project_root=Path.cwd(),
            max_issues=8,
        )
        self.assertIn("Coverage  96.25% lines (385/400)", report)

    def test_renderer_escapes_c1_and_bidi_controls(self) -> None:
        controls = "\x85\x9b\u202e\u2066"
        case = reporter.TestCase(
            lane=f"debug-{controls}-fast",
            name="fast::T-CTRL-002 terminal control probe",
            status="fail",
            duration=0.01,
            output=f"diagnostic={controls}",
        )
        issue = reporter.Issue(
            key="CONTROLS",
            code="ASSERTION",
            severity="fail",
            title=f"control-title={controls}",
            cases=[case],
        )
        report = reporter.render_dashboard(
            mode=f"Control{controls}",
            cases=[case],
            issues=[issue],
            fixtures=[],
            summary=None,
            artifact_root=f"artifact-{controls}",
            warnings=[f"warning-{controls}"],
            width=100,
            theme=reporter.Theme(colour=False, unicode=True),
            project_root=Path.cwd(),
            max_issues=8,
        )

        for control in controls:
            self.assertNotIn(control, report)
        for escaped in (r"\x85", r"\x9b", r"\u202e", r"\u2066"):
            self.assertIn(escaped, report)

    def test_plain_report_is_deterministic_and_has_no_ansi(self) -> None:
        case = reporter.TestCase(
            lane="debug-fast",
            name="fast::T-DEMO-001 demo",
            status="pass",
            duration=0.25,
        )
        report = reporter.render_dashboard(
            mode="Routine",
            cases=[case],
            issues=[],
            fixtures=[],
            summary=None,
            artifact_root="build/verification/demo",
            warnings=[],
            width=88,
            theme=reporter.Theme(colour=False, unicode=False),
            project_root=Path.cwd(),
            max_issues=8,
        )
        self.assertIn("TDFA VERIFICATION", report)
        self.assertIn("1 passed", report)
        self.assertIn("Debug | Fast", report)
        self.assertNotRegex(report, re.compile(r"\x1b\[[0-9;]*m"))
        self.assertTrue(all(ord(character) < 128 for character in report))

    def test_diagnostic_json_separates_failure_blocked_and_infra(self) -> None:
        failed = reporter.TestCase("fast", "T-FAIL", "fail", 0.1, "FAILED")
        blocked = reporter.TestCase("fast", "T-BLOCK", "fail", 0.1, "TSV contains CR")
        infra = reporter.TestCase("fast", "T-INFRA", "infra", 0.0, "missing")
        fixture = reporter.FixtureIssue("fixture.tsv", 1, 1, "a", "b")
        cases = [failed, blocked, infra]
        issues = reporter.group_issues(cases, [fixture])
        document = reporter.diagnostics_document(
            mode="Routine", cases=cases, issues=issues, fixtures=[fixture]
        )
        self.assertEqual(
            document["counts"],
            {
                "executions": 3,
                "unique_tests": 3,
                "passed": 0,
                "failed": 1,
                "blocked": 1,
                "infrastructure": 1,
                "skipped": 0,
            },
        )
        json.dumps(document)

    def test_requested_width_60_is_respected_in_unicode_and_plain_modes(self) -> None:
        case = reporter.TestCase(
            lane="debug-fast",
            name="fast::T-WIDTH-001 compact dashboard",
            status="pass",
            duration=0.01,
        )
        for unicode in (True, False):
            with self.subTest(unicode=unicode):
                report = reporter.render_dashboard(
                    mode="Routine",
                    cases=[case],
                    issues=[],
                    fixtures=[],
                    summary=None,
                    artifact_root=None,
                    warnings=[],
                    width=60,
                    theme=reporter.Theme(colour=False, unicode=unicode),
                    project_root=Path.cwd(),
                    max_issues=8,
                )
                self.assertEqual(
                    [line for line in report.splitlines() if display_cells(line) > 60],
                    [],
                )
                self.assertEqual(display_cells(report.splitlines()[0]), 60)

    def test_cjk_and_combining_evidence_obeys_display_cell_width(self) -> None:
        case = reporter.TestCase(
            lane="debug-fast",
            name="fast::T-WIDTH-002 cell-aware diagnostic",
            status="fail",
            duration=0.01,
            output=("combining=" + ("e\u0301" * 30) + "\n" "wide=" + ("\u754c" * 50)),
        )
        issue = reporter.Issue(
            key="DISPLAY_WIDTH",
            code="ASSERTION",
            severity="fail",
            title="CJK and combining display width",
            cases=[case],
        )
        theme = reporter.Theme(colour=False, unicode=True)
        report = reporter.render_dashboard(
            mode="Routine",
            cases=[case],
            issues=[issue],
            fixtures=[],
            summary=None,
            artifact_root=None,
            warnings=[],
            width=72,
            theme=theme,
            project_root=Path.cwd(),
            max_issues=8,
        )

        boxed = [
            line
            for line in report.splitlines()
            if line
            and line[0]
            in {
                theme.chars["tl"],
                theme.chars["bl"],
                theme.chars["v"],
            }
        ]
        self.assertTrue(boxed)
        self.assertTrue(all(display_cells(line) == 72 for line in boxed))
        self.assertEqual(
            [line for line in report.splitlines() if display_cells(line) > 72],
            [],
        )
        self.assertIn("e\u0301", report)
        self.assertIn("\u754c", report)

    def test_compact_report_respects_requested_width(self) -> None:
        case = reporter.TestCase(
            lane="release-deep",
            name="deep::T-DEMO-002 deliberately long failing test name",
            status="fail",
            duration=61.25,
            output="sample.cpp:42: FAILED:\n  expected 1 but observed 2",
        )
        issues = reporter.group_issues([case], [])
        summary = {
            "pipeline_status": "FAIL",
            "stages": [
                {
                    "name": "static-traceability",
                    "status": "FAIL",
                    "detail": "A deliberately long stage diagnostic that must be truncated",
                    "started_utc": "2026-07-30T10:00:00+00:00",
                    "finished_utc": "2026-07-30T10:00:00.5+00:00",
                }
            ],
        }
        report = reporter.render_dashboard(
            mode="Routine",
            cases=[case],
            issues=issues,
            fixtures=[],
            summary=summary,
            artifact_root="D:/" + "very-long-run-directory/" * 8,
            warnings=[],
            width=72,
            theme=reporter.Theme(colour=False, unicode=False),
            project_root=Path.cwd(),
            max_issues=8,
        )
        over_width = [line for line in report.splitlines() if len(line) > 72]
        self.assertEqual(over_width, [])

    def test_untrusted_controls_are_escaped(self) -> None:
        self.assertEqual(
            reporter.escape_inline("hello\tworld\x1b[31m\r\n"),
            r"hello\tworld\x1b[31m\r\n",
        )


if __name__ == "__main__":
    unittest.main()
