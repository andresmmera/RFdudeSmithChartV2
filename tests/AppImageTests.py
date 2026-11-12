import importlib.util
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch


TOOLS = Path(__file__).resolve().parents[1] / "tools"
spec = importlib.util.spec_from_file_location("check_linux_package", TOOLS / "check_linux_package.py")
package_check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(package_check)
spec = importlib.util.spec_from_file_location("check_appimage", TOOLS / "check_appimage.py")
appimage_check = importlib.util.module_from_spec(spec)
with patch.dict(sys.modules, {"check_linux_package": package_check}):
    spec.loader.exec_module(appimage_check)


class AppImageTests(unittest.TestCase):
    def check_fixture(self, missing=None, escaping=False):
        def run(command, **kwargs):
            if command[-1] == "--appimage-extract":
                appdir = Path(kwargs["cwd"]) / "squashfs-root"
                for name in (
                    "AppRun", "usr/bin/RFdudeSmithChartV2",
                    "usr/lib/rfdudesmithchartv2/bin/RFdudeSmithChartV2",
                    "usr/share/applications/rfdude-smith-chart-v2.desktop",
                    "usr/share/icons/hicolor/scalable/apps/rfdude-smith-chart-v2.svg",
                ):
                    path = appdir / name
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.touch()
                    path.chmod(0o755)
                for name, target in (
                    ("rfdude-smith-chart-v2.desktop",
                     "usr/share/applications/rfdude-smith-chart-v2.desktop"),
                    ("rfdude-smith-chart-v2.svg",
                     "usr/share/icons/hicolor/scalable/apps/rfdude-smith-chart-v2.svg"),
                ):
                    if name != missing:
                        (appdir / name).symlink_to(target)
                if escaping:
                    (appdir / "rfdude-smith-chart-v2.svg").unlink()
                    (appdir / "rfdude-smith-chart-v2.svg").symlink_to(candidate)
                return subprocess.CompletedProcess(command, 0, "", "")
            private = Path(kwargs["cwd"]) / appimage_check.PRIVATE_RUNTIME
            return subprocess.CompletedProcess(
                command, 0, "Cross-platform RF Smith chart\n",
                f"123: calling init: {private}/lib/libQt6Core.so.6\n"
                f"123: calling init: {private}/plugins/platforms/libqxcb.so\n",
            )

        with tempfile.TemporaryDirectory() as directory:
            candidate = Path(directory) / "fixture.AppImage"
            candidate.touch()
            candidate.chmod(0o755)
            with (
                patch.object(appimage_check.subprocess, "run", side_effect=run) as process,
                patch.object(appimage_check, "check_qt_dependencies"),
                patch.object(appimage_check, "check_rpaths"),
            ):
                appimage_check.check_appimage(candidate)
                self.assertEqual(process.call_count, 2)

    def test_root_metadata_links_allow_launch(self):
        self.check_fixture()

    def test_usr_share_metadata_alone_is_insufficient(self):
        for name in ("rfdude-smith-chart-v2.desktop", "rfdude-smith-chart-v2.svg"):
            with self.subTest(name=name), self.assertRaisesRegex(RuntimeError, "desktop entry, or icon"):
                self.check_fixture(missing=name)

    def test_root_metadata_link_must_stay_in_payload(self):
        with self.assertRaisesRegex(RuntimeError, "symlink escapes"):
            self.check_fixture(escaping=True)


if __name__ == "__main__":
    unittest.main()
