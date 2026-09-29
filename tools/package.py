"""Package the independently built engine with exact source and dependency notices."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import zipfile

ROOT = Path(__file__).resolve().parents[1]
SOURCE_DIRS = ("src", "tests", "tools", "patches", "vendor", "docs")
SOURCE_FILES = (".clang-format", ".gitignore", "LICENSE", "README.md",
                "frontend.cmake", "engine.anyband.json.in", "full-v1.json", "upstream.json")

def package(build, source, output, *, runtime=None):
    build, source, output = build.resolve(), source.resolve(), output.resolve()
    # New destinations only: never replace installed engines or user data.
    output.mkdir(parents=True, exist_ok=False)
    game = build / "game"
    manifest = json.loads((game / "engine.anyband.json").read_text())
    for name in (manifest["executable"], "engine.anyband.json"):
        shutil.copy2(game / name, output / name)
    shutil.copytree(source / "lib", output / "lib",
                    ignore=shutil.ignore_patterns("user", "save", "*.log"))
    shutil.copy2(ROOT / "README.md", output / "README.md")
    notices = output / "licenses"
    notices.mkdir()
    for src, name in ((ROOT / "LICENSE", "adapter-GPL-2.0.txt"),
                      (ROOT / "docs/angband-copying.rst", "angband-copying.rst"),
                      (ROOT / "vendor/cjson/LICENSE", "cJSON-MIT.txt")):
        shutil.copy2(src, notices / name)
    if manifest["executable"].endswith(".exe"):
        candidates = sorted(Path("C:/Program Files/Microsoft Visual Studio").glob(
            "*/*/VC/Redist/MSVC/[0-9]*/x64/Microsoft.VC*.CRT"))
        if runtime is not None:
            candidates = [Path(runtime)]
        if not candidates or not list(candidates[-1].glob("*.dll")):
            raise RuntimeError("Cannot find the redistributable runtime for packaging")
        for dll in candidates[-1].glob("*.dll"):
            shutil.copy2(dll, output / dll.name)
    # Explicit source roots keep local backups and unrelated files out of releases.
    # Include the exact prepared engine, so rebuilding does not require a network.
    with zipfile.ZipFile(output / "source.zip", "w", zipfile.ZIP_DEFLATED) as archive:
        paths = [ROOT / name for name in SOURCE_FILES]
        for name in SOURCE_DIRS:
            paths.extend((ROOT / name).rglob("*"))
        for path in sorted(paths):
            rel = path.relative_to(ROOT)
            if "__pycache__" in rel.parts or path.suffix == ".pyc":
                continue
            if path.is_file():
                archive.write(path, "adapter/" + rel.as_posix())
        for path in sorted(source.rglob("*")):
            rel = path.relative_to(source)
            if rel.parts[:2] in (("lib", "user"), ("lib", "save")) or "__pycache__" in rel.parts:
                continue
            if path.is_file():
                archive.write(path, "angband/" + rel.as_posix())
    hashes = {p.relative_to(output).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
              for p in sorted(output.rglob("*")) if p.is_file()}
    (output / "SHA256.json").write_text(json.dumps(hashes, indent=2) + "\n")
    archive_path = output.with_name(output.name + ".zip")
    if archive_path.exists():
        raise FileExistsError(archive_path)
    with zipfile.ZipFile(archive_path, "w", zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(output.rglob("*")):
            if path.is_file():
                archive.write(path, output.name + "/" + path.relative_to(output).as_posix())
    print(archive_path)
    return output

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", type=Path, default=ROOT / "build/native")
    parser.add_argument("--source", type=Path, default=ROOT / "build/engine")
    parser.add_argument("--output", type=Path, default=ROOT / "dist/angband-4.2.6-windows-x64")
    parser.add_argument("--runtime", type=Path, help="Runtime directory from the selected build toolchain")
    args = parser.parse_args()
    package(args.build, args.source, args.output, runtime=args.runtime)
