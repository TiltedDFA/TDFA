# Finding ledger

Reviewers report findings here; they do not silently edit another stage's work.

Allowed classifications:

- `PRODUCTION_DEFECT`: approved expectation is violated; tests stay unchanged.
- `TEST_TRANSLATION_DEFECT`: implementation does not match its frozen test spec.
- `ORACLE_DEFECT`: independent authority or model is demonstrably wrong.
- `CONTRACT_DEFECT`: card is contradictory, ambiguous, or conflicts with higher
  authority.
- `ENVIRONMENT_DEFECT`: build/tool/host issue unrelated to semantics.
- `BLOCKED_BY_TESTABILITY`: behavior cannot be tested deterministically within
  the approved no-production-change boundary.

```yaml
id: F-NNN
snapshot: <snapshot id>
status: OPEN | RESOLVED | REJECTED
severity: BLOCKER | HIGH | MEDIUM | LOW
classification: one of the values above
contracts: []
tests: []
evidence: exact reproducible evidence; no unsupported judgement
owner_stage: CONTRACT | DESIGN | WRITING | INTEGRATION | PRODUCTION | TOOLING
required_correction: narrow action that resolves the evidence
acceptance_evidence: command/result/review needed to close
reported_by: agent/person id
reviewed_by: []
contract_version: 0
```

No open blocker/high finding is compatible with final suite approval. Two
reviewers must approve an `ORACLE_DEFECT`, `CONTRACT_DEFECT`, equivalent mutant,
or test deletion.

## Findings

Append records below this line; never renumber or reuse IDs.

```yaml
id: F-001
snapshot: 6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592
status: OPEN
severity: HIGH
classification: PRODUCTION_DEFECT
contracts: [C-BIT-010, C-FEN-010]
tests: []
evidence: >-
  In the Clang 18 ASan/UBSan build, supplying the valid process transcript
  "uci\nquit\n" terminates at src/Position.cpp:317 with "load of value 2,
  which is not a valid value for type Colour". HashCurrentPostion increments a
  Colour loop variable beyond Black in the loop increment expression, creating
  an invalid enum value even though the next condition would terminate.
owner_stage: PRODUCTION
required_correction: >-
  Iterate the two valid colours without ever constructing an out-of-domain
  Colour value; do not weaken or suppress the enum sanitizer.
acceptance_evidence: >-
  The same transcript and the semantic hash tests complete under
  -fsanitize=address,undefined with no invalid-enum diagnostic.
reported_by: root-integration
reviewed_by: []
contract_version: 1
```

```yaml
id: F-002
snapshot: 6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592
status: OPEN
severity: HIGH
classification: PRODUCTION_DEFECT
contracts: [C-FEN-010, C-CHS-011]
tests: [T-STA-003, T-STA-004]
evidence: >-
  The independent exact-FEN model expects the standard one-based fullmove
  counter to advance after Black moves. From fullmove 1, both c7c5 and e8g8
  leave Position::FullMoves() at 1 instead of 2. The mismatch reproduces in
  the single-ply castling fixture and the nested e2e4 c7c5 prefix.
owner_stage: PRODUCTION
required_correction: >-
  Advance fullmove exactly after a successfully applied Black move and restore
  it exactly on matching unmake.
acceptance_evidence: >-
  T-STA-003 and T-STA-004 pass for their exact Black-move prefixes
  without altering the frozen expected FENs.
reported_by: root-integration
reviewed_by: []
contract_version: 1
```

```yaml
id: F-003
snapshot: 6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592
status: OPEN
severity: HIGH
classification: PRODUCTION_DEFECT
contracts: [C-UCI-010, C-MOV-010]
tests: [T-UTL-001, T-MOV-004]
evidence: >-
  For the four ordinary decoded promotion PieceTypes, UTIL::PromotionChar
  returns the fallback 'z': queen, rook, bishop, and knight should produce
  q, r, b, and n respectively. T-MOV-004 independently reproduces the same
  failure for all eight committed legal promotion tokens through
  UTIL::UciToMove followed by UTIL::MoveToStr. This is the same type domain
  supplied by Moves::PTypeOfProm, so legal promotion notation is not
  protocol-valid UCI.
owner_stage: PRODUCTION
required_correction: >-
  Make the promotion decoder and formatter share one explicitly valid semantic
  PieceType domain and emit exactly q/r/b/n.
acceptance_evidence: >-
  T-UTL-001 and the corpus promotion round-trips pass for all four suffixes.
reported_by: root-integration
reviewed_by: []
contract_version: 1
```

```yaml
id: F-004
snapshot: 6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592
status: OPEN
severity: HIGH
classification: PRODUCTION_DEFECT
contracts: [C-BIT-010]
tests: [T-BB-003]
evidence: >-
  On the nonzero singleton bitboard 0x1, the independent ordered-set model
  expects both least- and most-significant set-bit indices to be 0.
  Magics::FindMS1B returns 63. The implementation XORs the least-significant
  index with 63, which mirrors an index rather than locating the most-
  significant set bit.
owner_stage: PRODUCTION
required_correction: >-
  Implement the most-significant-set-bit index for every nonzero 64-bit input
  without relying on a mirrored least-significant index.
acceptance_evidence: >-
  T-BB-003 passes for all 64 singleton boards, boundary patterns, and the fixed
  deterministic random corpus.
reported_by: root-integration
reviewed_by: []
contract_version: 1
```

```yaml
id: F-005
snapshot: 6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592
status: RESOLVED
severity: MEDIUM
classification: TEST_TRANSLATION_DEFECT
contracts: []
tests: [T-PERFT-003]
evidence: >-
  The frozen perftsuite.epd checksum describes CRLF bytes, but the initial C++
  translation rejected every terminal CR before parsing any of its 126 rows.
owner_stage: INTEGRATION
required_correction: >-
  Normalize exactly one terminal CR in each CRLF record while retaining the
  original whole-file bytes for SHA-256 and rejecting embedded CR bytes.
acceptance_evidence: >-
  T-PERFT-003 passes all 770 checksum, cardinality, duplicate-line, per-depth,
  and total-count assertions against the unchanged perftsuite.epd.
reported_by: root-integration
reviewed_by: [root-mechanical-review]
contract_version: 0
```

```yaml
id: F-006
snapshot: 6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592
status: RESOLVED
severity: HIGH
classification: TEST_TRANSLATION_DEFECT
contracts: []
tests: [T-ORACLE-001, T-PERFT-001]
evidence: >-
  Several fixture consumers initially compared only the untrusted
  data_sha256 metadata text with the frozen digest. Altering a TSV data row
  while leaving that metadata line unchanged could therefore preserve the
  apparent checksum assertion.
owner_stage: INTEGRATION
required_correction: >-
  Recompute SHA-256 over the exact declared checksum scope (the column header
  and data rows through the final LF) during every TSV load, and compare both
  the recomputed value and metadata value with the frozen digest.
acceptance_evidence: >-
  BlindFixtureData rejects a scoped-byte/metadata mismatch before returning a
  fixture, and each frozen corpus test independently compares the recomputed
  scoped digest with its committed constant.
reported_by: blind-translation-review-state
reviewed_by: [root-mechanical-review]
contract_version: 0
```

```yaml
id: F-007
snapshot: 6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592
status: RESOLVED
severity: HIGH
classification: TEST_TRANSLATION_DEFECT
contracts: [C-BRD-010, C-FEN-010]
tests: [T-FEN-001, T-FEN-003, T-POS-002, T-STA-001, T-STA-003, T-STA-004]
evidence: >-
  The initial independent FEN model constructed its expected castling value
  from the production Magics::CASTLE_* constants and compared the resulting
  raw U8. Aliased representation constants could therefore agree with their
  own oracle while representing the wrong named-right set.
owner_stage: INTEGRATION
required_correction: >-
  Retain expected rights solely as a test-owned set of FEN names K/Q/k/q.
  Project the observed raw value to those names, reject unknown bits, and
  compare named sets rather than raw representation values.
acceptance_evidence: >-
  ExpectedPosition contains no production castling constants; all exact FEN
  comparisons use named-right set equality, while raw bits remain diagnostic
  data only.
reported_by: blind-translation-review-state
reviewed_by: [root-mechanical-review]
contract_version: 3
```

```yaml
id: F-008
snapshot: 6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592
status: RESOLVED
severity: HIGH
classification: TEST_TRANSLATION_DEFECT
contracts: [C-APP-001]
tests: [T-APP-001, T-APP-002, T-APP-003, T-APP-017, T-APP-018, T-APP-019]
evidence: >-
  Under UCRT64 MinGW, all six process cases initially reported an immediate
  output-drain timeout even though the child had exited with code 0 and both
  captured streams had reached EOF. ProcessHarness treated winpthreads'
  pthread_t token from std::thread::native_handle as a Win32 HANDLE;
  WaitForSingleObject returned WAIT_FAILED, which the harness mislabeled as a
  timeout.
owner_stage: TOOLING
required_correction: >-
  On MinGW, translate pthread_t with pthread_gethandle before passing it to
  Win32 wait/cancellation functions; retain the native MSVC path unchanged.
acceptance_evidence: >-
  After the conversion, the five process cases not containing a deliberately
  unsupported input variant exit cleanly, drain both streams, and pass with
  hard_cleanup_attempted=false.
reported_by: root-integration
reviewed_by: [harness-diagnose]
contract_version: 3
```

```yaml
id: F-009
snapshot: 6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592
status: OPEN
severity: HIGH
classification: PRODUCTION_DEFECT
contracts: [C-APP-002, C-APP-003, C-APP-014]
tests: [T-APP-017]
evidence: >-
  Exact inputs "\t uci \t\n" and " \tisready\t \n" receive no uciok or
  readyok within the fixed 10-second response deadline. Canonical LF, padded
  literal-space cases, unknown input, and CRLF all remain responsive. The UCI
  input grammar permits arbitrary whitespace between and around tokens.
owner_stage: PRODUCTION
required_correction: >-
  Tokenize the UCI command line using the protocol's ASCII whitespace class,
  including horizontal tabs, without weakening command ordering or unknown-
  token handling.
acceptance_evidence: >-
  The uci-padded-lf and isready-padded dynamic sections of T-APP-017 complete
  their causally anchored handshakes/readiness replies and idle quit cleanup.
reported_by: root-integration
reviewed_by: []
contract_version: 3
```

```yaml
id: F-010
snapshot: 6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592
status: OPEN
severity: HIGH
classification: PRODUCTION_DEFECT
contracts: [C-APP-002, C-APP-014]
tests: [T-APP-017]
evidence: >-
  Sending the exact four bytes "uci\r" as the first command produces no
  handshake output within the fixed 10-second response deadline. The same
  command framed with LF and CRLF succeeds. C-APP-014 freezes lone CR as one of
  the ordinary UCI line endings.
owner_stage: PRODUCTION
required_correction: >-
  Recognize a lone carriage return as a complete UCI command terminator while
  continuing to treat CRLF as one terminator and preserving exact input order.
acceptance_evidence: >-
  The uci-cr dynamic section of T-APP-017 receives its ordered handshake,
  readyok, and clean idle-quit exit without appending an LF.
reported_by: root-integration
reviewed_by: []
contract_version: 3
```

```yaml
id: F-011
snapshot: 6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592
status: RESOLVED
severity: HIGH
classification: TEST_TRANSLATION_DEFECT
contracts: [C-APP-001, C-APP-002, C-APP-003, C-APP-010, C-APP-014, C-APP-015]
tests: [T-APP-001, T-APP-002, T-APP-003, T-APP-017, T-APP-018, T-APP-019]
evidence: >-
  Independent translation review found that the first process-test harness
  used capture ceilings below the frozen card values, treated root-process
  exit as sufficient cleanup evidence, did not assert hard-cleanup completion
  on response failures, and allowed a registration "ok" line without a
  preceding "checking" line.  A subsequent attempt to wait directly on the
  Job Object also produced false ten-second exit timeouts because a Job Object
  is not process-signaled in that way.
owner_stage: INTEGRATION
required_correction: >-
  Use the exact frozen byte, line, and event ceilings; preserve every dynamic
  variant as an independent section; require cleanup attempted, completed, and
  group-empty on every failing operation; require group-empty on successful
  exit; recognize only valid registration state transitions; and observe
  JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO through an associated completion port.
acceptance_evidence: >-
  T-APP-001 now exits and drains in about one tenth of a second with the Job
  Object empty and no hard cleanup. T-APP-017 executes all eight variants;
  its 262 passing assertions include bounded transcript capture and complete
  group cleanup for the three independently reported production failures.
reported_by: blind-uci-translation-review
reviewed_by: [root-mechanical-review]
contract_version: 3
```

```yaml
id: F-012
snapshot: 6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592
status: RESOLVED
severity: HIGH
classification: TEST_TRANSLATION_DEFECT
contracts: [C-BIT-010]
tests: [tdfa_fuzz_bit_primitives]
evidence: >-
  The first fuzz translation discarded the value returned by PopLS1B and
  PopMS1B, then compared the unchanged input variable with the independently
  expected cleared board. Once F-004 were fixed, every nonzero fuzz input would
  therefore have produced a false test abort.
owner_stage: INTEGRATION
required_correction: >-
  Capture each value-returning primitive's result and compare that result with
  the independent cleared-bit model.
acceptance_evidence: >-
  bit_primitives.cpp assigns both return values; the target builds under the
  Clang ASan/UBSan/libFuzzer configuration, while the existing F-004 seed still
  fails at the earlier genuine FindMS1B assertion.
reported_by: test-translation-audit
reviewed_by: [root-mechanical-review]
contract_version: 1
```

```yaml
id: F-013
snapshot: 6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592
status: OPEN
severity: HIGH
classification: BLOCKED_BY_TESTABILITY
contracts: [C-APP-001, C-APP-002, C-APP-003]
tests: [T-APP-001, T-APP-002, T-APP-003]
evidence: >-
  ProcessHarness assigns an output ordinal when a reader thread publishes a
  completed line, not when the child emitted its first byte. A stale identical
  uciok/readyok already buffered before a later SEND can be scheduled and
  published after that SEND. More fundamentally, independent stdin/stdout pipes
  cannot distinguish a byte-identical autonomous reply emitted immediately
  before a request from the same reply caused immediately after it. An arbitrary
  ordinary Windows exit code also cannot distinguish every abort implementation
  from an intentional equal numeric return code.
owner_stage: TOOLING
required_correction: >-
  Make the harness's observable temporal watermark race-free with overlapped IO
  and reader checkpoints. For exact request/response causation and orderly-exit
  proof, add the smallest release-neutral TDFA_DEBUG audit channel carrying only
  command/response sequence IDs and an orderly-termination marker, or explicitly
  narrow the claimed black-box evidence.
acceptance_evidence: >-
  Controlled helper-process tests reproduce and reject pre-request buffered and
  partial-line responses; debug-audited tests prove one-to-one request sequence
  linkage and orderly quit while public stdout remains independently validated.
reported_by: test-translation-audit
reviewed_by: []
contract_version: 3
```

```yaml
id: F-014
snapshot: 6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592
status: RESOLVED
severity: HIGH
classification: TEST_TRANSLATION_DEFECT
contracts: []
tests: [ProcessHarness]
evidence: >-
  On a timed-out SEND, ProcessHarness could detach its synchronous writer and
  then read written/error fields while the detached worker continued modifying
  them. The shared_ptr preserved allocation lifetime but did not prevent a C++
  data race.
owner_stage: TOOLING
required_correction: >-
  Publish progress and terminal error through atomics before any controller read;
  a later overlapped-IO rewrite remains tracked separately by F-013.
acceptance_evidence: >-
  WriteState::written and error are atomic, the worker uses a thread-local
  offset/error, and controller reads use acquire loads on both normal and timeout
  paths.
reported_by: test-translation-audit
reviewed_by: [root-mechanical-review]
contract_version: 0
```

```yaml
id: F-015
snapshot: 6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592
status: OPEN
severity: MEDIUM
classification: BLOCKED_BY_TESTABILITY
contracts: [C-RTM-SRCH-001]
tests: [T-RTM-SRCH-001]
evidence: >-
  Clang reports that Search.cpp compares unsigned Move last_best_move with the
  negative Eval::NEG_INF sentinel, so the predicate is always false. Search
  completion, timeout, and sentinel policy remain explicitly unresolved, so the
  intended replacement condition cannot be inferred safely from this warning.
owner_stage: CONTRACT
required_correction: >-
  Decide the intended previous-best-move sentinel and deterministic Search test
  seam, then freeze and test that semantic condition before changing production.
acceptance_evidence: >-
  An approved Search contract and deterministic test kill an always-false
  previous-best predicate without using wall-clock sleeps.
reported_by: root-integration
reviewed_by: []
contract_version: 3
```

```yaml
id: F-016
snapshot: 6ea2c5c04bc83b7be7633936c53851154b7203669505af50a6095531a38be592
status: OPEN
severity: HIGH
classification: PRODUCTION_DEFECT
contracts: [C-RTM-PERFT-001, C-RTM-PERFT-002]
tests: [T-RTM-PERFT-001, T-RTM-PERFT-002]
evidence: >-
  RunBulkPerft<false>(1) publishes GetNodes()==0 for each of the six frozen
  perft roots, whose independent depth-one totals are respectively
  20, 48, 14, 6, 44, and 46. The bulk recursion returns the legal root count
  immediately at depth one, while the public entry point discards that return;
  total_nodes_ is therefore never populated for this boundary. Depths two and
  three produce their frozen totals and every covered run restores the full
  public FEN-semantic snapshot.
owner_stage: PRODUCTION
required_correction: >-
  Publish the complete latest RunBulkPerft result through GetNodes for every
  supported positive depth, including the depth-one root boundary, without
  accumulating a prior run or changing public Position state.
acceptance_evidence: >-
  All 18 sequential oracle comparisons in T-RTM-PERFT-001 pass. Each depth-one
  precondition in T-RTM-PERFT-002 is nonzero and equal to its frozen count, so
  the test reaches the caller's single ResetData call and then observes zero
  while the outside Position snapshot remains identical.
reported_by: root-integration
reviewed_by: [runtime-translation-review]
contract_version: 3
```
