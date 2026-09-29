"""Export a reviewable patch series from a prepared engine tree against the pinned release."""
import argparse
import difflib
import io
import tarfile
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repository", type=Path, default=ROOT.parent / "angband")
    parser.add_argument("--source", type=Path, default=ROOT / "build/engine")
    args = parser.parse_args()
    lock = json.loads((ROOT / "upstream.json").read_text())
    archive = subprocess.check_output(["git", "archive", lock["commit"],
        "src", "CMakeLists.txt"], cwd=args.repository)
    with tarfile.open(fileobj=io.BytesIO(archive)) as tree:
        original = {m.name: tree.extractfile(m).read() for m in tree.getmembers() if m.isfile()}
    build_files = {"CMakeLists.txt", "src/Makefile.src", "src/win/vs2019/Angband.vcxproj"}
    names = sorted(set(original) | {p.relative_to(args.source).as_posix()
                   for p in (args.source / "src").rglob("*") if p.suffix in (".c", ".h")})
    groups = {"01-correctness.patch": [], "02-frontend-interface.patch": [], "03-external-build.patch": []}
    records = []
    for name in names:
        path = args.source / name
        if not path.is_file() or (path.suffix not in (".h", ".c") and name not in build_files):
            continue
        before = original.get(name, b"")
        after = path.read_bytes()
        if before.replace(b"\r\n", b"\n") == after.replace(b"\r\n", b"\n"):
            continue
        before_text, after_text = before.decode().replace("\r\n", "\n"), after.decode().replace("\r\n", "\n")
        corrected = before_text
        if name == "src/obj-power.c":
            corrected = after_text
        elif name == "CMakeLists.txt":
            corrected = before_text.replace("/utf8", "/utf-8")
        elif name == "src/ui-store.c":
            corrected = before_text.replace("if (!response) return false;", """if (!response) {
			object_delete(NULL, NULL, &dummy);
			return false;
		}""")
        elif name == "src/ui-context.c":
            corrected = before_text.replace("cmdq_push(CMD_WALK);", """cmdq_push(CMD_WALK);
			/* Supply the intended step; walking applies confusion itself. */
			cmd_set_arg_direction(cmdq_peek(), "direction",
					motion_dir(player->grid, loc(x, y)));""", 1)
        def difference(left, right):
            return list(difflib.unified_diff(left.splitlines(True), right.splitlines(True),
                fromfile="a/" + name if name in original else "/dev/null", tofile="b/" + name))
        groups["01-correctness.patch"].extend(difference(before_text, corrected))
        group = "03-external-build.patch" if name in build_files else "02-frontend-interface.patch"
        groups[group].extend(difference(corrected, after_text))
        diff = difference(before_text, after_text)
        records.append({"path": name, "added": sum(l.startswith("+") for l in diff[2:]),
                        "removed": sum(l.startswith("-") for l in diff[2:])})
    for name, diff in groups.items():
        (ROOT / "patches" / name).write_text("".join(diff), newline="\n")
    (ROOT / "patches/series").write_text("\n".join(groups) + "\n", newline="\n")
    result = {"base": lock["commit"], "files_changed": len(records),
              "added": sum(r["added"] for r in records), "removed": sum(r["removed"] for r in records),
              "files": records}
    (ROOT / "docs/surface.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps({k:v for k,v in result.items() if k != "files"}))

if __name__ == "__main__":
    main()
