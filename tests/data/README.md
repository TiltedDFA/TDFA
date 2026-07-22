# Offline chess oracle fixtures

These fixtures are plain UTF-8 TSV. C++ tests can read them without Python or
network access. Lines beginning with `#` are deterministic metadata; the first
non-comment line contains the column names. A single hyphen (`-`) represents an
empty list or unavailable scalar.

`oracle_cases.tsv` contains canonical six-field FENs, sorted legal UCI moves,
check/draw/game-over flags, and outcomes. History and random-walk rows retain a
root FEN and their complete UCI replay, so repetition-dependent answers can be
reconstructed instead of inferred from FEN alone. `game_over`, `result`,
`termination`, and `winner` use `claim_draw=False`; claimable 50-move and
threefold draws have separate columns. `is_threefold_repetition` distinguishes an
already repeated position from a prospective threefold claim enabled by one move.

`oracle_perft.tsv` is long-form: one row per position and depth. Its six positions
are the conventional public perft suite, limited to depth 3 so regeneration stays
quick.

The checksum is SHA-256 over the exact UTF-8 bytes beginning with the column-name
line and ending with its final LF. Metadata and comment lines are excluded. Legal
move lists are ASCII-lexicographically sorted.

## Regeneration

Install the isolated pinned dependency and regenerate the exact committed corpus:

```powershell
python -m pip install -r tools/requirements-oracle.txt
python tools/generate_oracle_cases.py --profile committed
python tools/generate_oracle_cases.py --profile committed --check
```

The `committed` profile fixes the seed, random count, walk bounds, and perft depth;
it rejects parameter overrides. The normalized invocation is also embedded as
`generation_args` metadata. For a larger offline corpus, use
`--profile custom --count 10000`. The generator uses a local SplitMix64
implementation and a fixed default seed, so output is independent of Python's
random-number implementation. No timestamp is written.
