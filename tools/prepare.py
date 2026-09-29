"""Export the pinned release and apply the reviewed patch series, without editing it."""
import argparse
import hashlib
import io
import os
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
    patches = [ROOT / "patches" / name for name in
               (ROOT / "patches/series").read_text().splitlines()
               if name and not name.startswith("#")]
    fingerprint = hashlib.sha256(b"".join(p.read_bytes() for p in patches)).hexdigest()
    expected = {"commit": lock["commit"], "patch_sha256": fingerprint}
    marker = destination / ".adapter-source.json"
    if destination.exists():
        if marker.is_file():
            recorded = json.loads(marker.read_text())
            if all(recorded.get(k) == v for k, v in expected.items()):
                if recorded.get("source_sha256") == source_digest(destination):
                    return destination
                raise SystemExit("Prepared engine source was edited; use --prepared deliberately "
                                 "or prepare a new source directory.")
        raise SystemExit(f"Source destination already exists or has different patches: {destination}. "
                         "Use a new --source directory; existing work is never deleted.")
    archive = subprocess.check_output(["git", "archive", lock["commit"]], cwd=repository)
    destination.mkdir(parents=True)
    with tarfile.open(fileobj=io.BytesIO(archive)) as source:
        source.extractall(destination, filter="data")
    # Do not discover the adapter's parent Git repository after it is initialized.
    env = dict(os.environ, GIT_CEILING_DIRECTORIES=str(destination.parent))
    for patch in patches:
        subprocess.run(["git", "apply", "--check", str(patch)], cwd=destination, env=env, check=True)
        subprocess.run(["git", "apply", "--whitespace=error", str(patch)], cwd=destination, env=env, check=True)
    expected["source_sha256"] = source_digest(destination)
    marker.write_text(json.dumps(expected, indent=2) + "\n")
    return destination

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", type=Path, default=ROOT.parent / "angband")
    parser.add_argument("--source", type=Path, default=ROOT / "build/engine")
    args = parser.parse_args()
    print(prepare(args.repository, args.source))
