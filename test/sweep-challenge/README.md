# MiniZinc Challenge Sweeps

`test/sweep-challenge/minizinc_challenge_sweep.py` is the maintained workflow for running Atlantis against the MiniZinc Challenge corpus. It handles five separate concerns:

- fetching or updating the upstream challenge archive
- materializing a filtered local corpus under `.challenge/`
- applying a committed compatibility patch pack to old models when needed
- compiling selected cases to FlatZinc
- running Atlantis, then aggregating and reporting the results

The script is intended to make challenge campaigns repeatable. A named run root captures the selected archive revision, the applied patch pack, the discovered case set, the per-case compile and run artifacts, and the final aggregate summaries.

## Directory Layout

All artifacts live under `.challenge/` in the repository root:

- `.challenge/repos/mzn-challenge/`
  - cached upstream archive checkout
- `.challenge/corpus/<archive_commit>/`
  - raw materialized challenge corpus for one upstream archive revision
- `.challenge/patched/<archive_commit>/<patch_pack_id>/`
  - patched corpus snapshot used for compile and run phases
- `.challenge/runs/<run_name>/analysis/`
  - `plan.json`, `summary.json`, and generated reports
- `.challenge/runs/<run_name>/runs/`
  - per-case `compile` and `run` JSON/stdout/stderr artifacts

The raw corpus is never edited in place. Patch packs are applied into `.challenge/patched/...`, which means a run can always be traced back to a specific upstream archive revision and a specific committed set of compatibility rewrites.

## Workflow

The normal lifecycle is:

1. `fetch`
   - updates the upstream archive checkout
   - materializes the selected archive revision
   - applies the selected patch pack
   - writes the planned case list
2. `generate`
   - discovers cases from the selected years and problems
   - compiles them with MiniZinc to FlatZinc
   - records `compile_ok`, `compile_timeout`, or `compile_error`
3. `run`
   - runs Atlantis only on compile-success cases
   - records `run_ok`, `run_timeout`, `run_crash`, `run_error`, or `run_skipped`
4. `analyze`
   - aggregates the per-case artifacts into `analysis/summary.json`
5. `report`
   - renders a concise Markdown report from the aggregate summaries

### Minimal smoke run

```sh
test/sweep-challenge/minizinc_challenge_sweep.py fetch --name smoke --years 2025 --limit 2
test/sweep-challenge/minizinc_challenge_sweep.py generate --name smoke --years 2025 --limit 2
test/sweep-challenge/minizinc_challenge_sweep.py run --name smoke --years 2025 --limit 2
test/sweep-challenge/minizinc_challenge_sweep.py analyze --name smoke
test/sweep-challenge/minizinc_challenge_sweep.py report --name smoke
```

### Typical modern-years campaign

```sh
test/sweep-challenge/minizinc_challenge_sweep.py fetch \
  --name modern-years \
  --years 2021,2022,2023,2024

test/sweep-challenge/minizinc_challenge_sweep.py generate \
  --name modern-years \
  --years 2021,2022,2023,2024 \
  --workers 5

test/sweep-challenge/minizinc_challenge_sweep.py run \
  --name modern-years \
  --years 2021,2022,2023,2024 \
  --workers 5 \
  --run-timeout 5 \
  --solver-timelimit-ms 2000

test/sweep-challenge/minizinc_challenge_sweep.py analyze --name modern-years
test/sweep-challenge/minizinc_challenge_sweep.py report --name modern-years
```

### Typical all-years campaign

```sh
test/sweep-challenge/minizinc_challenge_sweep.py fetch --name all-years
test/sweep-challenge/minizinc_challenge_sweep.py generate --name all-years --workers 5
test/sweep-challenge/minizinc_challenge_sweep.py run \
  --name all-years \
  --workers 4 \
  --run-timeout 5 \
  --solver-timelimit-ms 2000
test/sweep-challenge/minizinc_challenge_sweep.py analyze --name all-years
test/sweep-challenge/minizinc_challenge_sweep.py report --name all-years
```

## Important Options

- `--name`
  - required run-root name under `.challenge/runs/`
- `--years`
  - comma-separated year filter
- `--problems`
  - comma-separated problem-directory filter
- `--limit`
  - cap the number of discovered cases after filtering
- `--force`
  - rerun phases even when terminal artifacts already exist
  - also prunes stale artifacts that are outside the newly selected case set
- `--patch-pack`
  - compatibility patch pack to apply after corpus materialization
  - use `none` to run against the raw materialized corpus
- `--compile-timeout-overrides`
  - JSON file with per-problem compile budgets
  - use `none` to disable overrides
- `--workers`
  - in-process worker-pool size for `generate` and `run`
  - defaults to `1`
- `--run-timeout`
  - external wall-clock budget per Atlantis subprocess
- `--solver-timelimit-ms`
  - internal Atlantis time limit passed through to the solver

## Expected Run Times

Exact runtimes are machine-dependent, and compile times in particular depend strongly on MiniZinc flattening cost, not just Atlantis. Still, some rough expectations are stable enough to document.

### Smoke runs

- `--limit 2` or similarly tiny validation runs should usually finish in well under a minute once the archive is already materialized.

### Runtime sweeps

The runtime phase is easy to estimate:

- lower bound:
  - `(compile_ok_cases * run_timeout) / workers`
- real wall-clock:
  - usually that bound plus solver startup, process management, and artifact-writing overhead

Examples:

- about `377` runnable cases, `--run-timeout 5`, `--workers 5`
  - theoretical lower bound: about `6.3` minutes
  - practical expectation: about `8-15` minutes
- about `1634` runnable cases, `--run-timeout 5`, `--workers 4`
  - theoretical lower bound: about `34` minutes
  - practical expectation: about `35-60` minutes

### Compile sweeps

Compile time is less predictable because a few flattening-heavy families dominate wall-clock time.

Practical guidance:

- `2025` only:
  - usually tens of minutes, not hours
- `2021-2024`:
  - expect a substantial run; often around an hour, sometimes longer if several families hit large compile-time budgets
- all years:
  - expect a multi-hour job on a cold run root

If the compile phase stalls on the same families repeatedly, check `test/sweep-challenge/challenge-config/compile-timeouts.json` before changing Atlantis itself. Several problems are flattening-bound rather than solver-bound.

## Repeatability and Review

The intended unit of repeatability is the named run root. A reviewer should be able to inspect:

- `analysis/plan.json`
  - what was selected
- `analysis/summary.json`
  - what happened in aggregate
- `reports/report.md`
  - the human-facing rollup
- `runs/.../*.json`
  - the per-case ground truth

If you need to rerun a subset, keep the same `--name` and use narrower `--years` or `--problems` filters together with `--force`. The script will rerun the selected subset and prune stale artifacts that are no longer part of the filtered plan.

## Patch Packs

Older challenge models do not always compile unchanged on current MiniZinc. The patch-pack mechanism exists to make those compatibility fixes explicit, reviewable, and repeatable.

The default pack is `challenge-compat-v2`. Its rules are documented in [challenge-patches/README.md](challenge-patches/README.md).

Use `--patch-pack none` when you want to measure raw upstream compatibility instead of patched compatibility.

## Failure Interpretation

The status codes are intended to separate phases cleanly.

Compile statuses:

- `compile_ok`
- `compile_timeout`
- `compile_error`
- `compile_missing_artifact`

Runtime statuses:

- `run_ok`
- `run_timeout`
- `run_crash`
- `run_error`
- `run_skipped`

Interpretation:

- `compile_timeout` and `compile_error` are MiniZinc-side failures before Atlantis runs.
- `run_timeout` is a bounded Atlantis run that did not finish inside the external timeout.
- `run_crash` means Atlantis exited abnormally or hit a crash-pattern signature.
- `run_error` is a runner-side execution failure, distinct from a solver crash.
- `run_skipped` usually means the case had no usable compile artifact.

## Practical Advice

- Start with a smoke run before a long campaign.
- Use more workers only when the machine can sustain the extra subprocess load.
- Keep the archive revision, patch pack, and compile-timeout configuration stable during one reviewable campaign.
- Treat the aggregate summaries as the report, but use the per-case artifacts as the source of truth.
