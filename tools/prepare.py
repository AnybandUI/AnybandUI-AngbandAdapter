"""Export the pinned AnybandUI Angband commit without modifying the checkout."""
import argparse
import hashlib
import io
import json
from pathlib import Path
import subprocess
import tarfile

ROOT = Path(__file__).resolve().parents[1]

def source_digest(source):
    digest = hashlib.sha256()
    for path in sorted(source.rglob("*")):
        if path.is_file() and path.name != ".adapter-source.json":
            digest.update(path.relative_to(source).as_posix().encode() + b"\0")
            digest.update(path.read_bytes())
    return digest.hexdigest()


def prepare(repository, destination):
    repository, destination = repository.resolve(), destination.resolve()
    lock = json.loads((ROOT / "upstream.json").read_text())
    expected = {"commit": lock["commit"]}
    marker = destination / ".adapter-source.json"
    if destination.exists():
        if marker.is_file():
            recorded = json.loads(marker.read_text())
            if all(recorded.get(k) == v for k, v in expected.items()):
                if recorded.get("source_sha256") == source_digest(destination):
                    return destination
                raise SystemExit("Prepared engine source was edited; use --prepared deliberately "
                                 "or prepare a new source directory.")
        raise SystemExit(f"Source destination already exists or uses a different engine commit: {destination}. "
                         "Use a new --source directory; existing work is never deleted.")
    archive = subprocess.check_output(["git", "archive", lock["commit"]], cwd=repository)
    destination.mkdir(parents=True)
    with tarfile.open(fileobj=io.BytesIO(archive)) as source:
        source.extractall(destination, filter="data")
    expected["source_sha256"] = source_digest(destination)
    marker.write_text(json.dumps(expected, indent=2) + "\n")
    return destination

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", type=Path, default=ROOT.parent / "angband")
    parser.add_argument("--source", type=Path, default=ROOT / "build/engine")
    args = parser.parse_args()
    print(prepare(args.repository, args.source))
