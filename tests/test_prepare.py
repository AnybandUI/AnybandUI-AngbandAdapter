"""Verify source exports use the pinned commit and preserve existing work."""
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location(
    "adapter_prepare", Path(__file__).resolve().parents[1] / "tools/prepare.py")
prepare = importlib.util.module_from_spec(spec)
spec.loader.exec_module(prepare)


class PrepareTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.repository = self.root / "repository"
        self.repository.mkdir()
        self.destination = self.root / "engine"
        self.git("init", "-q")
        (self.repository / "engine.c").write_text("pinned engine\n")
        self.git("add", "engine.c")
        self.git("-c", "user.name=Test", "-c", "user.email=test@example.invalid",
                 "-c", "commit.gpgsign=false", "commit", "-qm", "Pinned engine")
        self.commit = self.git("rev-parse", "HEAD").strip()
        (self.root / "upstream.json").write_text(json.dumps({"commit": self.commit}))
        root_patch = patch.object(prepare, "ROOT", self.root)
        root_patch.start()
        self.addCleanup(root_patch.stop)

    def git(self, *args):
        return subprocess.check_output(
            ["git", *args], cwd=self.repository, text=True)

    def export(self):
        return prepare.prepare(self.repository, self.destination)

    def test_exports_pin_without_patches_despite_later_commit_and_local_edits(self):
        source = self.repository / "engine.c"
        source.write_text("later commit\n")
        self.git("add", "engine.c")
        self.git("-c", "user.name=Test", "-c", "user.email=test@example.invalid",
                 "-c", "commit.gpgsign=false", "commit", "-qm", "Later engine")
        source.write_text("local changes\n")
        self.export()
        self.assertEqual((self.destination / "engine.c").read_text(), "pinned engine\n")
        self.assertEqual(source.read_text(), "local changes\n")
        self.assertEqual(self.export(), self.destination.resolve())

    def test_modified_export_is_rejected_without_overwriting(self):
        self.export()
        source = self.destination / "engine.c"
        source.write_text("preserve these edits\n")
        with self.assertRaisesRegex(SystemExit, "edited"):
            self.export()
        self.assertEqual(source.read_text(), "preserve these edits\n")

    def test_different_pin_is_rejected_without_overwriting(self):
        self.export()
        (self.root / "upstream.json").write_text(json.dumps({"commit": "0" * 40}))
        with self.assertRaisesRegex(SystemExit, "different engine commit"):
            self.export()
        self.assertEqual((self.destination / "engine.c").read_text(), "pinned engine\n")


if __name__ == "__main__":
    unittest.main()
