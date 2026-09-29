"""Run engine, contract, actual UI transport, and rendering checks; save evidence."""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=ROOT / "build/reduced-native")
    parser.add_argument("--ui", type=Path, default=ROOT.parent / "AnybandUI")
    parser.add_argument("--engine-only", action="store_true")
    args = parser.parse_args()
    build, ui = args.build.resolve(), args.ui.resolve()
    game = build / "game"
    manifest = json.loads((game / "engine.anyband.json").read_text())
    backend = game / manifest["executable"]
    output = ROOT / "build" / ("verification-" + datetime.now(timezone.utc).strftime("%Y%m%d-%H%M%S"))
    output.mkdir(parents=True)
    records = []
    def run(name, command, timeout=600):
        print(name, flush=True)
        with (output / (name + ".log")).open("w", encoding="utf-8") as log:
            result = subprocess.run([str(a) for a in command], cwd=ROOT,
                stdout=log, stderr=subprocess.STDOUT, timeout=timeout)
        records.append({"check": name, "exit_code": result.returncode})
        (output / "results.json").write_text(json.dumps(records, indent=2) + "\n")
        if result.returncode:
            raise RuntimeError(f"{name} failed; see {output}")
    run("core-units", [sys.executable, "-B", ROOT / "tools/check_core.py", "--build", build])
    run("engine-integration", [sys.executable, "-B", ROOT / "tests/test_backend.py", "--backend", backend])
    run("adapter-boundaries", [sys.executable, "-B", ROOT / "tests/test_boundaries.py", "--backend", backend])
    if not args.engine_only:
        expected = json.loads((ROOT / "full-v1.json").read_text())
        actual = json.loads((ui / "protocol/full-v1.json").read_text())
        if expected != actual:
            raise RuntimeError("Adapter contract differs from the actual frontend contract")
        run("engine-contract", [sys.executable, "-B", ui / "protocol/check_engine.py", game / "engine.anyband.json"])
        run("client-transport", [sys.executable, "-B", ROOT / "tests/bench_session.py",
            "--backend", backend, "--transport", ui / "build-ui-native/game/anybandui-transport-tests.exe"])
        run("frontend-readiness", [sys.executable, "-B", ui / "anybandui/readiness.py", "--skip-build",
            "--engine", game / "engine.anyband.json"])
    print(f"All checks passed: {output}")

if __name__ == "__main__":
    main()
