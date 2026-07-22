# TDFA test-authoring pipeline

This directory separates test intent from the current implementation. Passing
tests are not the authoring objective: an approved test is allowed to expose a
TDFA defect. Expected results may change only when their contract or independent
oracle is shown to be wrong.

## Information firewall

| Stage | May inspect implementation bodies? | May run TDFA? | Output |
|---|---:|---:|---|
| Snapshot/redaction | Yes, mechanically only | No | Hash manifest and body-free dossier |
| Contract authoring | No | No | Purpose lines, contract cards, decision requests |
| Blind test design | No | No | Test specifications and independent oracles |
| Blind test writing | No | No | C++ tests implementing frozen specifications |
| Compilation integration | Yes, for mechanical fixes | Compile only | Buildable tests with unchanged expectations |
| Execution/triage | Yes | Yes | Classified failures; never automatic rebaselining |
| Gap/mutation audit | Yes | Yes | Behavioral gap requests and killed/surviving mutants |

The orchestrator records a different agent for authoring and approval. A person
or agent that has seen a function body must not later claim blind authorship of
that function's expectations.

## Snapshot and dossier

The frozen manifest hashes the raw bytes of every `src/**/*.hpp` and
`src/**/*.cpp`. The dossier contains header declarations only. It removes source
comments, literals, declaration initializers, and executable bodies and is
intentionally non-compilable.

Validate both artifacts without changing any file:

```powershell
python tools/generate_api_dossier.py --check
```

After intentionally freezing a new source snapshot, review these outputs and
apply them as ordinary patches:

```powershell
python tools/generate_api_dossier.py --print-manifest
python tools/generate_api_dossier.py --print-dossier
```

The generator never edits source or generated artifacts. A snapshot mismatch is
a hard stop: contracts and tests remain attached to the old snapshot until the
new manifest and dossier are reviewed.

## Contract authority

The application boundary is only what the TDFA process exposes through stdin,
stdout/stderr, UCI commands, positions, and emitted chess moves. Those contracts
come from UCI, FEN, and chess rules plus explicit owner decisions.

Every C++ function and class is an internal API, including symbols declared
`public`. Obvious mathematical properties may be specified independently.
Ambiguous preconditions, ordering, mutation, ownership, and failure behavior
must remain `UNRESOLVED` until the owner decides them; current bodies and current
output are not authorities.

## Workflow

1. Validate and freeze `spec/source-snapshot.sha256`.
2. Give contract agents only `spec/api-dossier.md`, external standards, and the
   files under `spec/contracts/`.
3. Record owner answers in `spec/decision-ledger.md`, then version and freeze the
   affected cards.
4. Give fresh design agents only the frozen cards, approved oracle descriptions,
   and `spec/traceability.schema.yaml`.
5. Freeze test inputs and expectations before compiling or running against TDFA.
6. Let a source-aware integrator make mechanical build corrections only.
7. Classify failures using `spec/finding-ledger.md`.
8. After the semantic suite is frozen, expose coverage, implementation bodies,
   old defects, and mutants to separate audit agents.
9. Reopen only the stage that owns a validated finding. Never weaken an
   expectation merely because TDFA fails it.

## Routine verification versus the coverage audit

Routine development verification is contract- and function-oriented. Every
internal callable is mapped either to substantive behavioral tests or to an
explicit blocked record naming the missing authority/test seam. Routine Fast
and Deep runs do not impose a source-line threshold, so ordinary refactors do
not create churn merely by moving or splitting implementation lines.

The Coverage tier is an explicit audit pass. It instruments all production
sources, runs the complete Fast+Deep semantic suite plus tests carrying the
dedicated `[audit]` tag, emits LCOV/HTML/JSON/text reports, and enforces 95%
**line** coverage. Audit-tagged cases may be implementation-aware and exercise
diagnostic or defensive paths solely to measure reachability; the traceability
validator explicitly forbids them from satisfying a Routine `TESTED` mapping.
Function, branch, condition, and decision percentages remain reported as gap
evidence but are not independent 95% gates. Executing a line is never treated
as proof of its correctness; the behavioral contracts remain the oracle.

The default verification command is the routine semantic gate:

```powershell
tools\verify.cmd Routine
```

It first checks the frozen source/dossier and validates
`spec/traceability.yaml`, then runs the checking-build fast suite and the
optimized Fast+Deep suite. Traceability requires every callable listed in
`spec/intended-purpose-v1.md` to be mapped exactly once: `TESTED` needs at least
one implemented behavioral record and a precise tested scope; `BLOCKED` needs a
concrete missing-authority or missing-observation reason. An executed function
or source line cannot satisfy this gate by itself.

The complete one-time evidence collection is:

```powershell
tools\verify.cmd Audit
```

Audit attempts traceability, debug, optimized, coverage-evidence generation,
instrumented-suite status, the independent 95%-line threshold, sanitizer,
persistent-fuzz, and curated-mutation stages even when an earlier stage exposes
a defect. Thus a semantic test failure cannot be misreported as coverage below
95%. It exits unsuccessfully if any stage failed. Each invocation owns an
immutable `build/verification/runs/<run-id>/` directory containing hashed JUnit,
coverage, sanitizer, fuzz, mutation, and aggregate evidence;
`build/verification/audit-summary.json` is an atomic copy of the latest run
summary. Persistent fuzz crash corpora are labelled as retained artifacts and
carry both pre-run and immutable post-run manifests, rather than being presented
as newly discovered inputs. Reports begin as `RUNNING`, retain that state if
interrupted, and become terminal only after an input-coherence stage seals the
exact evidence object proving that source, tests, fixtures (including
`perftsuite.epd`), catalogues, validators, and build configuration did not change
during the run. Individual `Fast`, `Deep`, `Coverage`, `Adversarial`, `Fuzz`, and
`Mutation` tiers remain available for focused work.

## Private access

`TDFA_TESTING` access is exceptional. A probe may arrange a legal but expensive-
to-reach state only after an independent oracle proves reachability and two
reviewers approve the test. It must not calculate expected results, emulate the
production algorithm, manufacture impossible state for coverage, or change
release behavior. Public-path and private-access coverage are reported
separately.
