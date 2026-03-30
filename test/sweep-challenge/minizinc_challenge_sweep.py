#!/usr/bin/env -S uv run --script
# /// script
# requires-python = ">=3.11"
# ///

from __future__ import annotations

import argparse
import datetime as dt
import hashlib
import json
import os
import platform
import re
import shutil
import socket
import subprocess
import sys
import textwrap
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Iterable


SCRIPT_PATH = Path(__file__).resolve()
DEFAULT_ARCHIVE_REPO = "https://github.com/MiniZinc/mzn-challenge.git"
DEFAULT_ARCHIVE_REF = "develop"
CHALLENGE_ROOT_NAME = ".challenge"
UPSTREAM_REPO_NAME = "mzn-challenge"
TERMINAL_STATUSES = {
    "compile_ok",
    "compile_timeout",
    "compile_error",
    "compile_missing_artifact",
    "run_ok",
    "run_timeout",
    "run_crash",
    "run_error",
    "run_skipped",
}
CRASH_PATTERNS = (
    "addresssanitizer",
    "undefinedbehaviorsanitizer",
    "segmentation fault",
    "assertion",
    "abort",
)


class SweepError(RuntimeError):
    pass


@dataclass(frozen=True)
class TimeStrategy:
    kind: str
    command_prefix: list[str]


@dataclass(frozen=True)
class CommandResult:
    command: list[str]
    cwd: str
    elapsed_wall_sec: float
    stdout: str
    stderr: str
    return_code: int | None
    signal: int | None
    timed_out: bool
    peak_memory_kib: int | None


@dataclass(frozen=True)
class Case:
    year: str
    problem: str
    model_path: Path
    instance_path: Path | None
    case_id: str

    def to_json(self) -> dict[str, Any]:
        return {
            "year": self.year,
            "problem": self.problem,
            "model_path": str(self.model_path),
            "instance_path": str(self.instance_path) if self.instance_path else None,
            "case_id": self.case_id,
        }


def log(message: str) -> None:
    print(message, flush=True)


def utc_now() -> str:
    return dt.datetime.now(dt.timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def slugify(value: str) -> str:
    value = re.sub(r"[^A-Za-z0-9._-]+", "-", value.strip())
    value = re.sub(r"-{2,}", "-", value).strip("-")
    return value or "unknown"


def short_hash(value: str, length: int = 8) -> str:
    return hashlib.sha256(value.encode("utf-8")).hexdigest()[:length]


def write_json(path: Path, payload: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True) + "\n")


def read_json(path: Path) -> Any:
    return json.loads(path.read_text())


def maybe_read_json(path: Path) -> Any | None:
    if not path.exists():
        return None
    try:
        return read_json(path)
    except Exception:
        return None


def ensure_dir(path: Path) -> Path:
    path.mkdir(parents=True, exist_ok=True)
    return path


def parse_csv_arg(raw: str | None) -> list[str] | None:
    if raw is None:
        return None
    values = [part.strip() for part in raw.split(",")]
    filtered = [value for value in values if value]
    return filtered or None


def repo_root_from_args(args: argparse.Namespace) -> Path:
    if getattr(args, "repo_root", None):
        return Path(args.repo_root).expanduser().resolve()
    return Path.cwd().resolve()


def challenge_root(repo_root: Path) -> Path:
    return repo_root / CHALLENGE_ROOT_NAME


def run_root(repo_root: Path, name: str) -> Path:
    return challenge_root(repo_root) / "runs" / name


def runs_dir(repo_root: Path, name: str) -> Path:
    return run_root(repo_root, name) / "runs"


def analysis_dir(repo_root: Path, name: str) -> Path:
    return run_root(repo_root, name) / "analysis"


def reports_dir(repo_root: Path, name: str) -> Path:
    return run_root(repo_root, name) / "reports"


def plots_dir(repo_root: Path, name: str) -> Path:
    return run_root(repo_root, name) / "plots"


def upstream_dir(repo_root: Path) -> Path:
    return challenge_root(repo_root) / "upstream" / UPSTREAM_REPO_NAME


def corpus_dir(repo_root: Path, archive_commit: str) -> Path:
    return challenge_root(repo_root) / "corpus" / archive_commit


def compiled_root(repo_root: Path, archive_commit: str, minizinc_cache_key: str) -> Path:
    return challenge_root(repo_root) / "compiled" / archive_commit / f"mzn-{minizinc_cache_key}"


def ensure_run_layout(repo_root: Path, name: str) -> None:
    for path in (
        challenge_root(repo_root),
        run_root(repo_root, name),
        runs_dir(repo_root, name),
        analysis_dir(repo_root, name),
        reports_dir(repo_root, name),
        plots_dir(repo_root, name),
    ):
        ensure_dir(path)


def git_output(cwd: Path, *args: str) -> str:
    completed = subprocess.run(
        ["git", *args],
        cwd=cwd,
        check=True,
        capture_output=True,
        text=True,
    )
    return completed.stdout.strip()


def repo_git_commit(repo_root: Path) -> str:
    try:
        return git_output(repo_root, "rev-parse", "HEAD")
    except Exception:
        return "unknown"


def resolve_time_strategy() -> TimeStrategy:
    gtime = shutil.which("gtime")
    if gtime:
        return TimeStrategy(kind="gnu", command_prefix=[gtime, "-v"])
    linux_time = Path("/usr/bin/time")
    if linux_time.exists():
        probe = subprocess.run(
            [str(linux_time), "-v", "true"],
            capture_output=True,
            text=True,
        )
        if probe.returncode == 0:
            return TimeStrategy(kind="gnu", command_prefix=[str(linux_time), "-v"])
        probe = subprocess.run(
            [str(linux_time), "-l", "true"],
            capture_output=True,
            text=True,
        )
        if probe.returncode == 0:
            return TimeStrategy(kind="bsd", command_prefix=[str(linux_time), "-l"])
    return TimeStrategy(kind="none", command_prefix=[])


def parse_peak_memory_kib(stderr: str, strategy: TimeStrategy) -> int | None:
    if strategy.kind == "gnu":
        match = re.search(r"Maximum resident set size \(kbytes\):\s*(\d+)", stderr)
        if match:
            return int(match.group(1))
        match = re.search(r"Maximum resident set size \(KB\):\s*(\d+)", stderr)
        if match:
            return int(match.group(1))
        return None
    if strategy.kind == "bsd":
        match = re.search(r"(\d+)\s+maximum resident set size", stderr)
        if match:
            bytes_value = int(match.group(1))
            return bytes_value // 1024
    return None


def run_command(
    command: list[str],
    *,
    cwd: Path,
    timeout_sec: int | None,
    time_strategy: TimeStrategy,
) -> CommandResult:
    full_command = [*time_strategy.command_prefix, *command]
    started = time.perf_counter()
    try:
        completed = subprocess.run(
            full_command,
            cwd=cwd,
            capture_output=True,
            text=True,
            timeout=timeout_sec,
        )
        return_code = completed.returncode
        signal = -return_code if return_code is not None and return_code < 0 else None
        stderr = completed.stderr
        stdout = completed.stdout
        timed_out = False
    except subprocess.TimeoutExpired as exc:
        return_code = None
        signal = None
        stdout = exc.stdout or ""
        stderr = exc.stderr or ""
        timed_out = True
    elapsed_wall_sec = time.perf_counter() - started
    return CommandResult(
        command=command,
        cwd=str(cwd),
        elapsed_wall_sec=elapsed_wall_sec,
        stdout=stdout,
        stderr=stderr,
        return_code=return_code,
        signal=signal,
        timed_out=timed_out,
        peak_memory_kib=parse_peak_memory_kib(stderr, time_strategy),
    )


def parse_solver_status(stdout: str) -> str:
    lowered = stdout.lower()
    if "=====optimal=====" in lowered:
        return "optimal"
    if "=====unsatisfiable=====" in lowered:
        return "unsat"
    if "----------" in lowered:
        return "satisfied"
    if "=====unknown=====" in lowered:
        return "unknown"
    return "none"


def detect_crash_signature(stderr: str) -> str | None:
    lowered = stderr.lower()
    for pattern in CRASH_PATTERNS:
        if pattern in lowered:
            return pattern
    return None


def resolve_minizinc_binary(raw: str | None) -> Path:
    candidate = raw or shutil.which("minizinc")
    if not candidate:
        raise SweepError("MiniZinc executable not found. Use --minizinc or add `minizinc` to PATH.")
    path = Path(candidate).expanduser().resolve()
    if not path.exists():
        raise SweepError(f"MiniZinc executable does not exist: {path}")
    return path


def minizinc_version(minizinc_binary: Path) -> str:
    completed = subprocess.run(
        [str(minizinc_binary), "--version"],
        capture_output=True,
        text=True,
        check=True,
    )
    first_line = next((line.strip() for line in completed.stdout.splitlines() if line.strip()), "")
    return first_line or "unknown"


def minizinc_cache_key(minizinc_binary: Path, version: str) -> str:
    return f"{slugify(version)}-{short_hash(str(minizinc_binary))}"


def resolve_build_artifacts(repo_root: Path, build_dir_raw: str | None) -> tuple[Path, Path]:
    build_dir = Path(build_dir_raw or "build")
    if not build_dir.is_absolute():
        build_dir = (repo_root / build_dir).resolve()
    solver_config = build_dir / "atlantis.msc"
    atlantis_binary = build_dir / "fzn-atlantis"
    if not solver_config.exists():
        raise SweepError(f"Missing Atlantis solver config: {solver_config}")
    if not atlantis_binary.exists():
        raise SweepError(f"Missing Atlantis binary: {atlantis_binary}")
    return solver_config, atlantis_binary


def copy_tree_contents(src: Path, dst: Path, *, force: bool) -> None:
    if dst.exists() and force:
        shutil.rmtree(dst)
    if dst.exists():
        return
    ensure_dir(dst.parent)
    shutil.copytree(src, dst)


def ensure_upstream_checkout(repo_root: Path, archive_repo: str) -> Path:
    dest = upstream_dir(repo_root)
    if dest.exists():
        return dest
    ensure_dir(dest.parent)
    log(f"Cloning upstream archive into {dest}")
    subprocess.run(
        ["git", "clone", "--origin", "origin", archive_repo, str(dest)],
        check=True,
        cwd=repo_root,
    )
    return dest


def fetch_upstream(repo_root: Path, archive_repo: str, archive_ref: str) -> tuple[Path, str]:
    checkout = ensure_upstream_checkout(repo_root, archive_repo)
    subprocess.run(
        ["git", "remote", "set-url", "origin", archive_repo],
        check=True,
        cwd=checkout,
    )
    log(f"Fetching upstream ref {archive_ref}")
    subprocess.run(
        ["git", "fetch", "--depth", "1", "origin", archive_ref],
        check=True,
        cwd=checkout,
    )
    commit = git_output(checkout, "rev-parse", "FETCH_HEAD")
    subprocess.run(
        ["git", "checkout", "--force", commit],
        check=True,
        cwd=checkout,
    )
    return checkout, commit


def available_years(checkout: Path) -> list[str]:
    years = [path.name for path in checkout.iterdir() if path.is_dir() and re.fullmatch(r"\d{4}", path.name)]
    return sorted(years)


def materialize_corpus(
    repo_root: Path,
    checkout: Path,
    archive_commit: str,
    selected_years: list[str] | None,
    *,
    force: bool,
) -> tuple[Path, list[str]]:
    years = available_years(checkout)
    if selected_years is not None:
        missing = sorted(set(selected_years) - set(years))
        if missing:
            raise SweepError(f"Requested years not found in archive: {', '.join(missing)}")
        years = [year for year in years if year in set(selected_years)]
    target_root = corpus_dir(repo_root, archive_commit)
    ensure_dir(target_root)
    for year in years:
        copy_tree_contents(checkout / year, target_root / year, force=force)
    return target_root, years


def discover_cases(
    corpus_root: Path,
    years: Iterable[str],
    selected_problems: list[str] | None,
    limit: int | None,
) -> tuple[list[Case], list[dict[str, Any]]]:
    allowed_problems = set(selected_problems or [])
    cases: list[Case] = []
    errors: list[dict[str, Any]] = []
    for year in years:
        year_dir = corpus_root / year
        if not year_dir.exists():
            continue
        problem_dirs = [path for path in sorted(year_dir.iterdir()) if path.is_dir()]
        for problem_dir in problem_dirs:
            if selected_problems is not None and problem_dir.name not in allowed_problems:
                continue
            model_paths = sorted(problem_dir.glob("*.mzn"))
            if len(model_paths) != 1:
                errors.append(
                    {
                        "year": year,
                        "problem": problem_dir.name,
                        "path": str(problem_dir),
                        "error": "expected exactly one top-level .mzn file",
                        "mzn_count": len(model_paths),
                    }
                )
                continue
            model_path = model_paths[0]
            instance_paths = sorted([*problem_dir.glob("*.dzn"), *problem_dir.glob("*.json")])
            if instance_paths:
                stem_counts: dict[str, int] = {}
                for instance_path in instance_paths:
                    stem_counts[instance_path.stem] = stem_counts.get(instance_path.stem, 0) + 1
                for instance_path in instance_paths:
                    case_id = slugify(instance_path.stem)
                    if stem_counts[instance_path.stem] > 1:
                        case_id = slugify(f"{instance_path.stem}-{instance_path.suffix.lstrip('.')}")
                    cases.append(
                        Case(
                            year=year,
                            problem=problem_dir.name,
                            model_path=model_path,
                            instance_path=instance_path,
                            case_id=case_id,
                        )
                    )
            else:
                cases.append(
                    Case(
                        year=year,
                        problem=problem_dir.name,
                        model_path=model_path,
                        instance_path=None,
                        case_id="model_only",
                    )
                )
    if limit is not None:
        cases = cases[:limit]
    return cases, errors


def artifact_paths(repo_root: Path, name: str, run_id: str) -> tuple[Path, Path, Path]:
    root = runs_dir(repo_root, name)
    return root / f"{run_id}.json", root / f"{run_id}.stdout", root / f"{run_id}.stderr"


def terminal_json(path: Path) -> dict[str, Any] | None:
    payload = maybe_read_json(path)
    if isinstance(payload, dict) and payload.get("status") in TERMINAL_STATUSES:
        return payload
    return None


def year_version_map() -> dict[str, str]:
    return {
        "2025": "2.9.3",
        "2024": "2.8.5",
        "2023": "2.7.6",
        "2022": "2.6.3",
        "2021": "2.5.5",
        "2020": "2.4.3",
        "2019": "2.3.1",
        "2018": "2.1.7",
        "2017": "2.1.5",
        "2016": "2.0.13",
        "2015": "2.0.4",
        "2014": "1.6",
        "2013": "1.6",
        "2012": "1.5",
        "2011": "1.3",
        "2010": "1.1",
        "2009": "1.0",
        "2008": "0.8",
    }


def common_provenance(repo_root: Path, archive_commit: str, archive_ref: str) -> dict[str, Any]:
    strategy = resolve_time_strategy()
    return {
        "repo_root": str(repo_root),
        "repo_git_commit": repo_git_commit(repo_root),
        "archive_commit": archive_commit,
        "archive_ref": archive_ref,
        "platform": platform.platform(),
        "python_version": platform.python_version(),
        "hostname": socket.gethostname(),
        "time_strategy": strategy.kind,
        "script_path": str(SCRIPT_PATH),
        "generated_at": utc_now(),
    }


def build_plan(
    *,
    repo_root: Path,
    name: str,
    archive_commit: str,
    archive_ref: str,
    archive_repo: str,
    selected_years: list[str],
    selected_problems: list[str] | None,
    limit: int | None,
    cases: list[Case],
    discovery_errors: list[dict[str, Any]],
    extra: dict[str, Any] | None = None,
) -> dict[str, Any]:
    payload = {
        "name": name,
        "archive_repo": archive_repo,
        "archive_ref": archive_ref,
        "archive_commit": archive_commit,
        "selected_years": selected_years,
        "selected_problems": selected_problems,
        "limit": limit,
        "case_count": len(cases),
        "cases": [case.to_json() for case in cases],
        "discovery_errors": discovery_errors,
        "expected_minizinc_versions_by_year": year_version_map(),
        "provenance": common_provenance(repo_root, archive_commit, archive_ref),
    }
    if extra:
        payload.update(extra)
    return payload


def compile_run_id(archive_commit: str, minizinc_key: str, case: Case) -> str:
    return "__".join(
        [
            "compile",
            archive_commit[:12],
            slugify(minizinc_key),
            case.year,
            slugify(case.problem),
            slugify(case.case_id),
        ]
    )


def runtime_run_id(archive_commit: str, minizinc_key: str, case: Case) -> str:
    return "__".join(
        [
            "run",
            archive_commit[:12],
            slugify(minizinc_key),
            case.year,
            slugify(case.problem),
            slugify(case.case_id),
        ]
    )


def compile_case(
    *,
    repo_root: Path,
    name: str,
    archive_commit: str,
    archive_ref: str,
    case: Case,
    solver_config: Path,
    minizinc_binary: Path,
    minizinc_version_value: str,
    minizinc_key: str,
    compiled_cache_root: Path,
    atlantis_binary: Path,
    time_strategy: TimeStrategy,
    timeout_sec: int,
    force: bool,
) -> dict[str, Any]:
    run_id = compile_run_id(archive_commit, minizinc_key, case)
    json_path, stdout_path, stderr_path = artifact_paths(repo_root, name, run_id)
    existing = terminal_json(json_path)
    if existing is not None and not force:
        return existing

    target_dir = ensure_dir(compiled_cache_root / case.year / case.problem)
    fzn_path = target_dir / f"{case.case_id}.fzn"
    command = [
        str(minizinc_binary),
        "--solver",
        str(solver_config),
        "-c",
        str(case.model_path),
    ]
    if case.instance_path is not None:
        command.append(str(case.instance_path))
    command.extend(["--fzn", str(fzn_path), "--no-output-ozn"])
    result = run_command(command, cwd=repo_root, timeout_sec=timeout_sec, time_strategy=time_strategy)
    stdout_path.write_text(result.stdout)
    stderr_path.write_text(result.stderr)
    if result.timed_out:
        status = "compile_timeout"
    elif result.return_code == 0 and fzn_path.exists():
        status = "compile_ok"
    elif result.return_code == 0:
        status = "compile_missing_artifact"
    else:
        status = "compile_error"
    payload = {
        "run_id": run_id,
        "phase": "compile",
        "archive_commit": archive_commit,
        "archive_ref": archive_ref,
        "year": case.year,
        "problem": case.problem,
        "model_path": str(case.model_path),
        "instance_path": str(case.instance_path) if case.instance_path else None,
        "case_id": case.case_id,
        "command": result.command,
        "cwd": result.cwd,
        "status": status,
        "return_code": result.return_code,
        "signal": result.signal,
        "elapsed_wall_sec": result.elapsed_wall_sec,
        "peak_memory_kib": result.peak_memory_kib,
        "stdout_path": str(stdout_path),
        "stderr_path": str(stderr_path),
        "fzn_path": str(fzn_path) if fzn_path.exists() else None,
        "minizinc_version": minizinc_version_value,
        "atlantis_binary": str(atlantis_binary),
        "repo_git_commit": repo_git_commit(repo_root),
        "timestamp_utc": utc_now(),
    }
    write_json(json_path, payload)
    return payload


def run_case(
    *,
    repo_root: Path,
    name: str,
    archive_commit: str,
    archive_ref: str,
    case: Case,
    atlantis_binary: Path,
    minizinc_key: str,
    compile_payload: dict[str, Any] | None,
    time_strategy: TimeStrategy,
    timeout_sec: int,
    solver_timelimit_ms: int,
    force: bool,
) -> dict[str, Any]:
    run_id = runtime_run_id(archive_commit, minizinc_key, case)
    json_path, stdout_path, stderr_path = artifact_paths(repo_root, name, run_id)
    existing = terminal_json(json_path)
    if existing is not None and not force:
        return existing

    if compile_payload is None or compile_payload.get("status") != "compile_ok" or not compile_payload.get("fzn_path"):
        payload = {
            "run_id": run_id,
            "phase": "run",
            "archive_commit": archive_commit,
            "archive_ref": archive_ref,
            "year": case.year,
            "problem": case.problem,
            "model_path": str(case.model_path),
            "instance_path": str(case.instance_path) if case.instance_path else None,
            "case_id": case.case_id,
            "command": [],
            "cwd": str(repo_root),
            "status": "run_skipped",
            "return_code": None,
            "signal": None,
            "elapsed_wall_sec": 0.0,
            "peak_memory_kib": None,
            "stdout_path": str(stdout_path),
            "stderr_path": str(stderr_path),
            "fzn_path": compile_payload.get("fzn_path") if compile_payload else None,
            "minizinc_version": compile_payload.get("minizinc_version") if compile_payload else None,
            "atlantis_binary": str(atlantis_binary),
            "repo_git_commit": repo_git_commit(repo_root),
            "timestamp_utc": utc_now(),
            "solver_status": "none",
            "crash_signature": None,
            "skip_reason": "compile_not_ok",
        }
        stdout_path.write_text("")
        stderr_path.write_text("")
        write_json(json_path, payload)
        return payload

    fzn_path = Path(compile_payload["fzn_path"])
    command = [
        str(atlantis_binary),
        str(fzn_path),
        "--time-limit",
        str(solver_timelimit_ms),
    ]
    result = run_command(command, cwd=repo_root, timeout_sec=timeout_sec, time_strategy=time_strategy)
    stdout_path.write_text(result.stdout)
    stderr_path.write_text(result.stderr)
    crash_signature = detect_crash_signature(result.stderr)
    if result.timed_out:
        status = "run_timeout"
    elif result.signal is not None or crash_signature is not None:
        status = "run_crash"
    elif result.return_code == 0:
        status = "run_ok"
    else:
        status = "run_error"
    payload = {
        "run_id": run_id,
        "phase": "run",
        "archive_commit": archive_commit,
        "archive_ref": archive_ref,
        "year": case.year,
        "problem": case.problem,
        "model_path": str(case.model_path),
        "instance_path": str(case.instance_path) if case.instance_path else None,
        "case_id": case.case_id,
        "command": result.command,
        "cwd": result.cwd,
        "status": status,
        "return_code": result.return_code,
        "signal": result.signal,
        "elapsed_wall_sec": result.elapsed_wall_sec,
        "peak_memory_kib": result.peak_memory_kib,
        "stdout_path": str(stdout_path),
        "stderr_path": str(stderr_path),
        "fzn_path": str(fzn_path),
        "minizinc_version": compile_payload.get("minizinc_version"),
        "atlantis_binary": str(atlantis_binary),
        "repo_git_commit": repo_git_commit(repo_root),
        "timestamp_utc": utc_now(),
        "solver_status": parse_solver_status(result.stdout),
        "crash_signature": crash_signature,
    }
    write_json(json_path, payload)
    return payload


def aggregate_status_counts(items: Iterable[dict[str, Any]]) -> dict[str, int]:
    counts: dict[str, int] = {}
    for item in items:
        status = str(item.get("status", "unknown"))
        counts[status] = counts.get(status, 0) + 1
    return dict(sorted(counts.items()))


def increment_nested(target: dict[str, dict[str, int]], bucket: str, status: str) -> None:
    if bucket not in target:
        target[bucket] = {}
    target[bucket][status] = target[bucket].get(status, 0) + 1


def collect_phase_payloads(repo_root: Path, name: str, phase: str) -> list[dict[str, Any]]:
    collected: list[dict[str, Any]] = []
    for json_path in sorted(runs_dir(repo_root, name).glob(f"{phase}__*.json")):
        payload = maybe_read_json(json_path)
        if isinstance(payload, dict):
            collected.append(payload)
    return collected


def analyze_run(repo_root: Path, name: str) -> dict[str, Any]:
    plan_path = analysis_dir(repo_root, name) / "plan.json"
    plan_payload = maybe_read_json(plan_path)
    if not isinstance(plan_payload, dict):
        raise SweepError(f"Missing plan.json for run {name}. Run fetch or generate first.")
    valid_cases = {
        (
            str(case.get("year")),
            str(case.get("problem")),
            str(case.get("case_id")),
        )
        for case in plan_payload.get("cases", [])
        if isinstance(case, dict)
    }

    def in_current_plan(payload: dict[str, Any]) -> bool:
        return (
            str(payload.get("year")),
            str(payload.get("problem")),
            str(payload.get("case_id")),
        ) in valid_cases

    compile_payloads = [payload for payload in collect_phase_payloads(repo_root, name, "compile") if in_current_plan(payload)]
    run_payloads = [payload for payload in collect_phase_payloads(repo_root, name, "run") if in_current_plan(payload)]
    by_year: dict[str, dict[str, int]] = {}
    by_problem: dict[str, dict[str, int]] = {}
    by_year_problem: dict[str, dict[str, int]] = {}
    failure_classes: dict[str, int] = {}
    for payload in [*compile_payloads, *run_payloads]:
        status = str(payload.get("status", "unknown"))
        year = str(payload.get("year", "unknown"))
        problem = str(payload.get("problem", "unknown"))
        increment_nested(by_year, year, status)
        increment_nested(by_problem, problem, status)
        increment_nested(by_year_problem, f"{year}/{problem}", status)
        if status not in {"compile_ok", "run_ok", "run_timeout", "run_skipped"}:
            failure_classes[status] = failure_classes.get(status, 0) + 1

    compile_failures = [payload for payload in compile_payloads if payload.get("status") not in {"compile_ok"}]
    runtime_crashes = [payload for payload in run_payloads if payload.get("status") == "run_crash"]
    runtime_errors = [payload for payload in run_payloads if payload.get("status") == "run_error"]
    summary = {
        "name": name,
        "generated_at": utc_now(),
        "archive_commit": plan_payload.get("archive_commit"),
        "archive_ref": plan_payload.get("archive_ref"),
        "repo_git_commit": plan_payload.get("provenance", {}).get("repo_git_commit"),
        "total_discovered": int(plan_payload.get("case_count", 0)),
        "discovery_error_count": len(plan_payload.get("discovery_errors", [])),
        "compile_status_counts": aggregate_status_counts(compile_payloads),
        "run_status_counts": aggregate_status_counts(run_payloads),
        "failure_classes": dict(sorted(failure_classes.items(), key=lambda item: (-item[1], item[0]))),
        "worst_problems": sorted(
            (
                {
                    "problem": problem,
                    "failure_count": sum(
                        count for status, count in statuses.items() if status not in {"compile_ok", "run_ok", "run_timeout", "run_skipped"}
                    ),
                }
                for problem, statuses in by_problem.items()
            ),
            key=lambda item: (-item["failure_count"], item["problem"]),
        )[:20],
        "worst_years": sorted(
            (
                {
                    "year": year,
                    "failure_count": sum(
                        count for status, count in statuses.items() if status not in {"compile_ok", "run_ok", "run_timeout", "run_skipped"}
                    ),
                }
                for year, statuses in by_year.items()
            ),
            key=lambda item: (-item["failure_count"], item["year"]),
        )[:20],
        "provenance": plan_payload.get("provenance", {}),
    }
    failures = {
        "compile_failures": compile_failures[:50],
        "runtime_crashes": runtime_crashes[:50],
        "runtime_errors": runtime_errors[:50],
        "discovery_errors": plan_payload.get("discovery_errors", [])[:50],
    }
    write_json(analysis_dir(repo_root, name) / "summary.json", summary)
    write_json(analysis_dir(repo_root, name) / "failures.json", failures)
    write_json(analysis_dir(repo_root, name) / "by-year.json", by_year)
    write_json(analysis_dir(repo_root, name) / "by-problem.json", by_problem)
    write_json(analysis_dir(repo_root, name) / "by-year-problem.json", by_year_problem)
    return {"summary": summary, "failures": failures}


def render_counts(title: str, counts: dict[str, int]) -> str:
    lines = [f"## {title}", ""]
    if not counts:
        lines.append("- none")
    else:
        for key, value in counts.items():
            lines.append(f"- `{key}`: {value}")
    lines.append("")
    return "\n".join(lines)


def render_ranked(title: str, items: list[dict[str, Any]], label_key: str) -> str:
    lines = [f"## {title}", ""]
    if not items:
        lines.append("- none")
    else:
        for item in items:
            lines.append(f"- `{item[label_key]}`: {item['failure_count']} failures")
    lines.append("")
    return "\n".join(lines)


def render_failure_examples(title: str, items: list[dict[str, Any]]) -> str:
    lines = [f"## {title}", ""]
    if not items:
        lines.append("- none")
        lines.append("")
        return "\n".join(lines)
    for item in items[:10]:
        label = f"{item.get('year')}/{item.get('problem')}/{item.get('case_id')}"
        lines.append(
            f"- `{label}` status=`{item.get('status')}` stderr=`{item.get('stderr_path')}` stdout=`{item.get('stdout_path')}`"
        )
    lines.append("")
    return "\n".join(lines)


def write_report(repo_root: Path, name: str) -> Path:
    summary_payload = maybe_read_json(analysis_dir(repo_root, name) / "summary.json")
    failures_payload = maybe_read_json(analysis_dir(repo_root, name) / "failures.json")
    if not isinstance(summary_payload, dict) or not isinstance(failures_payload, dict):
        analysis = analyze_run(repo_root, name)
        summary_payload = analysis["summary"]
        failures_payload = analysis["failures"]
    report_path = reports_dir(repo_root, name) / "report.md"
    body = [
        f"# MiniZinc Challenge Sweep Report: {name}",
        "",
        f"- Generated: {summary_payload.get('generated_at')}",
        f"- Archive commit: `{summary_payload.get('archive_commit')}`",
        f"- Archive ref: `{summary_payload.get('archive_ref')}`",
        f"- Repo git commit: `{summary_payload.get('repo_git_commit')}`",
        f"- Total discovered cases: {summary_payload.get('total_discovered')}",
        f"- Discovery errors: {summary_payload.get('discovery_error_count')}",
        "",
        render_counts("Compile Status Counts", summary_payload.get("compile_status_counts", {})),
        render_counts("Run Status Counts", summary_payload.get("run_status_counts", {})),
        render_counts("Failures By Class", summary_payload.get("failure_classes", {})),
        render_ranked("Worst Problems", summary_payload.get("worst_problems", []), "problem"),
        render_ranked("Worst Years", summary_payload.get("worst_years", []), "year"),
        "## Corpus Acquisition Problems",
        "",
    ]
    discovery_errors = failures_payload.get("discovery_errors", [])
    if discovery_errors:
        for item in discovery_errors[:10]:
            body.append(
                f"- `{item.get('year')}/{item.get('problem')}` path=`{item.get('path')}` error=`{item.get('error')}`"
            )
    else:
        body.append("- none")
    body.extend(
        [
            "",
            render_failure_examples("Compile Problems", failures_payload.get("compile_failures", [])),
            render_failure_examples("Runtime Problems", [*failures_payload.get("runtime_crashes", []), *failures_payload.get("runtime_errors", [])]),
            "## Provenance",
            "",
        ]
    )
    for key, value in sorted(summary_payload.get("provenance", {}).items()):
        body.append(f"- `{key}`: `{value}`")
    body.append("")
    report_path.write_text("\n".join(body))
    return report_path


def execute_fetch_or_generate_setup(
    args: argparse.Namespace,
    *,
    require_minizinc: bool,
) -> dict[str, Any]:
    repo_root = repo_root_from_args(args)
    ensure_run_layout(repo_root, args.name)
    checkout, archive_commit = fetch_upstream(repo_root, args.archive_repo, args.archive_ref)
    selected_years = parse_csv_arg(args.years)
    corpus_root, years = materialize_corpus(
        repo_root,
        checkout,
        archive_commit,
        selected_years,
        force=bool(getattr(args, "force", False)),
    )
    selected_problems = parse_csv_arg(args.problems)
    cases, discovery_errors = discover_cases(corpus_root, years, selected_problems, args.limit)
    payload: dict[str, Any] = {
        "repo_root": repo_root,
        "checkout": checkout,
        "archive_commit": archive_commit,
        "corpus_root": corpus_root,
        "selected_years": years,
        "selected_problems": selected_problems,
        "cases": cases,
        "discovery_errors": discovery_errors,
    }
    if require_minizinc:
        minizinc_binary = resolve_minizinc_binary(getattr(args, "minizinc", None))
        version = minizinc_version(minizinc_binary)
        payload["minizinc_binary"] = minizinc_binary
        payload["minizinc_version"] = version
        payload["minizinc_key"] = minizinc_cache_key(minizinc_binary, version)
    return payload


def cmd_fetch(args: argparse.Namespace) -> int:
    setup = execute_fetch_or_generate_setup(args, require_minizinc=False)
    plan = build_plan(
        repo_root=setup["repo_root"],
        name=args.name,
        archive_commit=setup["archive_commit"],
        archive_ref=args.archive_ref,
        archive_repo=args.archive_repo,
        selected_years=setup["selected_years"],
        selected_problems=setup["selected_problems"],
        limit=args.limit,
        cases=setup["cases"],
        discovery_errors=setup["discovery_errors"],
    )
    write_json(analysis_dir(setup["repo_root"], args.name) / "plan.json", plan)
    log(f"Wrote plan to {analysis_dir(setup['repo_root'], args.name) / 'plan.json'}")
    log(f"Discovered {len(setup['cases'])} cases across {len(setup['selected_years'])} years.")
    return 0


def cmd_generate(args: argparse.Namespace) -> int:
    setup = execute_fetch_or_generate_setup(args, require_minizinc=True)
    repo_root = setup["repo_root"]
    solver_config, atlantis_binary = resolve_build_artifacts(repo_root, args.build_dir)
    compiled_cache = ensure_dir(compiled_root(repo_root, setup["archive_commit"], setup["minizinc_key"]))
    time_strategy = resolve_time_strategy()
    plan = build_plan(
        repo_root=repo_root,
        name=args.name,
        archive_commit=setup["archive_commit"],
        archive_ref=args.archive_ref,
        archive_repo=args.archive_repo,
        selected_years=setup["selected_years"],
        selected_problems=setup["selected_problems"],
        limit=args.limit,
        cases=setup["cases"],
        discovery_errors=setup["discovery_errors"],
        extra={
            "minizinc_binary": str(setup["minizinc_binary"]),
            "minizinc_version": setup["minizinc_version"],
            "minizinc_cache_key": setup["minizinc_key"],
            "solver_config": str(solver_config),
            "atlantis_binary": str(atlantis_binary),
        },
    )
    write_json(analysis_dir(repo_root, args.name) / "plan.json", plan)
    compile_payloads = []
    for index, case in enumerate(setup["cases"], start=1):
        log(f"[compile {index}/{len(setup['cases'])}] {case.year}/{case.problem}/{case.case_id}")
        payload = compile_case(
            repo_root=repo_root,
            name=args.name,
            archive_commit=setup["archive_commit"],
            archive_ref=args.archive_ref,
            case=case,
            solver_config=solver_config,
            minizinc_binary=setup["minizinc_binary"],
            minizinc_version_value=setup["minizinc_version"],
            minizinc_key=setup["minizinc_key"],
            compiled_cache_root=compiled_cache,
            atlantis_binary=atlantis_binary,
            time_strategy=time_strategy,
            timeout_sec=args.compile_timeout,
            force=args.force,
        )
        compile_payloads.append(payload)
    log(f"Compile status counts: {aggregate_status_counts(compile_payloads)}")
    return 0


def cmd_run(args: argparse.Namespace) -> int:
    setup = execute_fetch_or_generate_setup(args, require_minizinc=True)
    repo_root = setup["repo_root"]
    _, atlantis_binary = resolve_build_artifacts(repo_root, args.build_dir)
    plan_path = analysis_dir(repo_root, args.name) / "plan.json"
    if not plan_path.exists():
        plan = build_plan(
            repo_root=repo_root,
            name=args.name,
            archive_commit=setup["archive_commit"],
            archive_ref=args.archive_ref,
            archive_repo=args.archive_repo,
            selected_years=setup["selected_years"],
            selected_problems=setup["selected_problems"],
            limit=args.limit,
            cases=setup["cases"],
            discovery_errors=setup["discovery_errors"],
            extra={
                "minizinc_binary": str(setup["minizinc_binary"]),
                "minizinc_version": setup["minizinc_version"],
                "minizinc_cache_key": setup["minizinc_key"],
                "atlantis_binary": str(atlantis_binary),
            },
        )
        write_json(plan_path, plan)
    time_strategy = resolve_time_strategy()
    if args.dry_run:
        log(f"Would evaluate {len(setup['cases'])} cases for Atlantis runtime.")
        return 0
    run_payloads = []
    for index, case in enumerate(setup["cases"], start=1):
        compile_json_path, _, _ = artifact_paths(repo_root, args.name, compile_run_id(setup["archive_commit"], setup["minizinc_key"], case))
        compile_payload = maybe_read_json(compile_json_path)
        log(f"[run {index}/{len(setup['cases'])}] {case.year}/{case.problem}/{case.case_id}")
        payload = run_case(
            repo_root=repo_root,
            name=args.name,
            archive_commit=setup["archive_commit"],
            archive_ref=args.archive_ref,
            case=case,
            atlantis_binary=atlantis_binary,
            minizinc_key=setup["minizinc_key"],
            compile_payload=compile_payload if isinstance(compile_payload, dict) else None,
            time_strategy=time_strategy,
            timeout_sec=args.run_timeout,
            solver_timelimit_ms=args.solver_timelimit_ms,
            force=args.force,
        )
        run_payloads.append(payload)
    log(f"Run status counts: {aggregate_status_counts(run_payloads)}")
    return 0


def cmd_analyze(args: argparse.Namespace) -> int:
    repo_root = repo_root_from_args(args)
    ensure_run_layout(repo_root, args.name)
    result = analyze_run(repo_root, args.name)
    log(f"Wrote analysis to {analysis_dir(repo_root, args.name)}")
    log(f"Compile status counts: {result['summary'].get('compile_status_counts', {})}")
    log(f"Run status counts: {result['summary'].get('run_status_counts', {})}")
    return 0


def cmd_report(args: argparse.Namespace) -> int:
    repo_root = repo_root_from_args(args)
    ensure_run_layout(repo_root, args.name)
    report_path = write_report(repo_root, args.name)
    log(f"Wrote report to {report_path}")
    return 0


def add_common_run_selection_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--name", required=True, help="Named run root under .challenge/runs/")
    parser.add_argument("--repo-root", help="Repository root. Defaults to the current working directory.")
    parser.add_argument("--archive-repo", default=DEFAULT_ARCHIVE_REPO, help="MiniZinc Challenge git repository URL.")
    parser.add_argument("--archive-ref", default=DEFAULT_ARCHIVE_REF, help="Git ref to fetch from the archive repository.")
    parser.add_argument("--years", help="Comma-separated list of challenge years to include.")
    parser.add_argument("--problems", help="Comma-separated list of challenge problem directories to include.")
    parser.add_argument("--limit", type=int, help="Limit the number of discovered cases after filtering.")
    parser.add_argument("--force", action="store_true", help="Re-run steps even when terminal JSON artifacts already exist.")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Fetch, compile, run, and summarize MiniZinc Challenge sweeps for Atlantis.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=textwrap.dedent(
            """\
            Examples:
              tools/minizinc_challenge_sweep.py fetch --name smoke --years 2025 --limit 2
              tools/minizinc_challenge_sweep.py generate --name smoke --years 2025 --limit 2
              tools/minizinc_challenge_sweep.py run --name smoke --years 2025 --limit 2
              tools/minizinc_challenge_sweep.py analyze --name smoke
              tools/minizinc_challenge_sweep.py report --name smoke
            """
        ),
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    fetch_parser = subparsers.add_parser("fetch", help="Fetch/update the upstream archive and write analysis/plan.json.")
    add_common_run_selection_args(fetch_parser)
    fetch_parser.set_defaults(func=cmd_fetch)

    generate_parser = subparsers.add_parser("generate", help="Discover cases and compile them to FlatZinc.")
    add_common_run_selection_args(generate_parser)
    generate_parser.add_argument("--minizinc", help="Path to the MiniZinc executable. Defaults to `minizinc` on PATH.")
    generate_parser.add_argument("--build-dir", default="build", help="Atlantis build directory containing atlantis.msc and fzn-atlantis.")
    generate_parser.add_argument("--compile-timeout", type=int, default=60, help="Per-case MiniZinc compile timeout in seconds.")
    generate_parser.set_defaults(func=cmd_generate)

    run_parser = subparsers.add_parser("run", help="Run Atlantis on compile-success cases.")
    add_common_run_selection_args(run_parser)
    run_parser.add_argument("--minizinc", help="Path to the MiniZinc executable. Defaults to `minizinc` on PATH.")
    run_parser.add_argument("--build-dir", default="build", help="Atlantis build directory containing fzn-atlantis.")
    run_parser.add_argument("--run-timeout", type=int, default=5, help="Per-case Atlantis subprocess timeout in seconds.")
    run_parser.add_argument("--solver-timelimit-ms", type=int, default=2000, help="Atlantis internal time limit in milliseconds.")
    run_parser.add_argument("--dry-run", action="store_true", help="Print the intended runtime case count without executing Atlantis.")
    run_parser.set_defaults(func=cmd_run)

    analyze_parser = subparsers.add_parser("analyze", help="Aggregate run artifacts into analysis/*.json.")
    analyze_parser.add_argument("--name", required=True, help="Named run root under .challenge/runs/")
    analyze_parser.add_argument("--repo-root", help="Repository root. Defaults to the current working directory.")
    analyze_parser.set_defaults(func=cmd_analyze)

    report_parser = subparsers.add_parser("report", help="Render a Markdown summary for a named run.")
    report_parser.add_argument("--name", required=True, help="Named run root under .challenge/runs/")
    report_parser.add_argument("--repo-root", help="Repository root. Defaults to the current working directory.")
    report_parser.set_defaults(func=cmd_report)
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        return int(args.func(args))
    except SweepError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2
    except subprocess.CalledProcessError as exc:
        print(f"error: command failed with exit code {exc.returncode}: {' '.join(exc.cmd)}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
