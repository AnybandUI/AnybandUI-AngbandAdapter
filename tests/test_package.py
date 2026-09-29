"""Packaging regressions using files only; no engine or compiler is launched."""
import contextlib
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import zipfile

spec = importlib.util.spec_from_file_location(
    "adapter_package", Path(__file__).resolve().parents[1] / "tools/package.py")
packager = importlib.util.module_from_spec(spec)
spec.loader.exec_module(packager)


class PackageTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.adapter = self.root / "adapter"
        self.build = self.root / "build"
        self.source = self.root / "engine"
        self.output = self.root / "release"
        for name in packager.SOURCE_FILES:
            self.write(self.adapter / name, "source " + name)
        for name in ("src/main.c", "tests/map.c", "tools/prepare.py",
                     "patches/series", "docs/angband-copying.rst", "vendor/cjson/LICENSE"):
            self.write(self.adapter / name, "maintained " + name)
        for name in ("recovery/private.zip", "experiments/old.c", "local-notes.txt",
                     "build/debug.pdb", "dist/old.zip", ".git/config", "src/__pycache__/old.pyc"):
            self.write(self.adapter / name, "excluded " + name)
        self.write(self.source / "src/game.c", "engine source")
        self.write(self.source / "lib/gamedata/constants.txt", "game data")
        self.write(self.source / "lib/user/save/character", "personal save")
        self.write(self.source / "lib/save/character", "legacy personal save")
        self.write(self.build / "game/engine", "test binary placeholder")
        self.write(self.build / "game/engine.anyband.json", json.dumps({"executable": "engine"}))

    @staticmethod
    def write(path, text):
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)

    def package(self, **kwargs):
        with patch.object(packager, "ROOT", self.adapter), contextlib.redirect_stdout(io.StringIO()):
            return packager.package(self.build, self.source, self.output, **kwargs)

    def test_release_contains_maintained_source_and_no_local_baggage(self):
        output = self.package()
        archive_path = output.with_name(output.name + ".zip")
        with zipfile.ZipFile(output / "source.zip") as archive:
            names = set(archive.namelist())
            self.assertIn("adapter/src/main.c", names)
            self.assertIn("adapter/patches/series", names)
            self.assertIn("adapter/vendor/cjson/LICENSE", names)
            self.assertIn("angband/src/game.c", names)
            self.assertIn("angband/lib/gamedata/constants.txt", names)
            for unwanted in ("recovery", "experiments", "local-notes.txt", ".git", "__pycache__", "save"):
                self.assertFalse(any(unwanted in Path(name).parts for name in names), unwanted)
        self.assertFalse((output / "lib/user").exists())
        self.assertFalse((output / "lib/save").exists())
        self.assertEqual((output / "engine").read_text(), "test binary placeholder")
        with zipfile.ZipFile(archive_path) as archive:
            self.assertIn("release/source.zip", archive.namelist())
        self.assertIn("source.zip", json.loads((output / "SHA256.json").read_text()))

    def test_official_engine_audio_assets_are_preserved(self):
        self.write(self.source / "lib/sounds/sample.mp3", "official sound")
        self.write(self.source / "lib/customize/sound.prf", "official sound mappings")
        output = self.package()
        self.assertEqual((output / "lib/sounds/sample.mp3").read_text(), "official sound")
        self.assertEqual((output / "lib/customize/sound.prf").read_text(), "official sound mappings")
        with zipfile.ZipFile(output / "source.zip") as archive:
            self.assertIn("angband/lib/sounds/sample.mp3", archive.namelist())
            self.assertIn("angband/lib/customize/sound.prf", archive.namelist())

    def test_existing_output_is_not_overwritten(self):
        self.write(self.output / "keep.txt", "keep me")
        with self.assertRaises(FileExistsError):
            self.package()
        self.assertEqual((self.output / "keep.txt").read_text(), "keep me")

    def test_windows_runtime_comes_from_selected_toolchain(self):
        self.write(self.build / "game/engine.exe", "Windows binary placeholder")
        self.write(self.build / "game/engine.anyband.json", json.dumps({"executable": "engine.exe"}))
        runtime = self.root / "selected-runtime"
        self.write(runtime / "vcruntime-test.dll", "selected runtime")
        output = self.package(runtime=runtime)
        self.assertEqual((output / "vcruntime-test.dll").read_text(), "selected runtime")


if __name__ == "__main__":
    unittest.main()
