#!/usr/bin/env python3
"""Extract and smoke-test a bundled AppImage without installing it."""

import argparse
import os
from pathlib import Path
import subprocess
import tempfile

from check_linux_package import (
    check_loaded_qt_libraries,
    check_qt_dependencies,
    check_rpaths,
)


PRIVATE_RUNTIME = Path("usr/lib/rfdudesmithchartv2")
APP_NAME = "RFdudeSmithChartV2"


def clean_environment() -> dict[str, str]:
    environment = os.environ.copy()
    for name in (
        "APPIMAGE",
        "LD_LIBRARY_PATH",
        "LD_PRELOAD",
        "QT_PLUGIN_PATH",
        "QT_QPA_PLATFORM_PLUGIN_PATH",
        "QT_QPA_PLATFORMTHEME",
        "QT_QPA_PLATFORM",
        "LD_DEBUG",
        "LD_DEBUG_OUTPUT",
    ):
        environment.pop(name, None)
    return environment


def check_appimage(appimage: Path) -> None:
    if not appimage.is_file() or not os.access(appimage, os.X_OK):
        raise RuntimeError(f"AppImage is missing or not executable: {appimage}")

    with tempfile.TemporaryDirectory(prefix="rfdude-appimage-") as directory:
        extraction_root = Path(directory)
        extracted = subprocess.run(
            [str(appimage), "--appimage-extract"],
            cwd=extraction_root,
            check=True,
            capture_output=True,
            text=True,
            timeout=120,
            env=clean_environment(),
        )
        appdir = extraction_root / "squashfs-root"
        private = appdir / PRIVATE_RUNTIME
        executable = private / "bin" / APP_NAME
        app_run = appdir / "AppRun"
        launcher = appdir / "usr/bin" / APP_NAME
        desktop_file = appdir / "usr/share/applications/rfdude-smith-chart-v2.desktop"
        icon = appdir / "usr/share/icons/hicolor/scalable/apps/rfdude-smith-chart-v2.svg"
        if not app_run.is_file() or not os.access(app_run, os.X_OK):
            raise RuntimeError("AppImage is missing its executable AppRun entry point.")
        if not all(path.is_file() for path in (
            launcher, desktop_file, icon,
            appdir / desktop_file.name, appdir / icon.name,
        )):
            raise RuntimeError("AppImage is missing its launcher, desktop entry, or icon.")
        if not executable.is_file():
            raise RuntimeError(f"AppImage is missing its private executable: {executable}")
        for path in appdir.rglob("*"):
            if path.is_symlink() and not path.resolve().is_relative_to(appdir.resolve()):
                raise RuntimeError(f"AppImage symlink escapes its payload: {path.relative_to(appdir)}")

        environment = clean_environment()
        environment["APPDIR"] = str(appdir)
        check_qt_dependencies(executable, private, environment)
        check_rpaths(executable, private)
        for tree in (private / "lib", private / "plugins"):
            for binary in tree.rglob("*.so*"):
                if binary.is_symlink() or not binary.is_file():
                    continue
                with binary.open("rb") as stream:
                    is_elf = stream.read(4) == b"\x7fELF"
                if is_elf:
                    check_rpaths(binary, private)
                    check_qt_dependencies(binary, private, environment, require_qt=False)

        environment["QT_QPA_PLATFORM"] = "xcb"
        environment["LD_DEBUG"] = "libs"
        result = subprocess.run(
            ["xvfb-run", "-a", str(app_run), "--help"],
            cwd=appdir,
            env=environment,
            capture_output=True,
            text=True,
            timeout=60,
        )
        if result.returncode or "Cross-platform RF Smith chart" not in result.stdout:
            raise RuntimeError(
                f"AppImage launcher failed (exit {result.returncode}):\n"
                f"{result.stdout}\n{result.stderr}\n{extracted.stderr}"
            )
        loader_output = result.stdout + result.stderr
        check_loaded_qt_libraries(loader_output, private)
        platform_plugin = private / "plugins/platforms/libqxcb.so"
        if not any(
            str(platform_plugin) in line and "calling init:" in line
            for line in loader_output.splitlines()
        ):
            raise RuntimeError(f"AppImage did not load its private Qt platform plugin:\n{loader_output}")
        print(f"PASS: isolated runtime and extracted-AppImage launch: {appimage.name}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("appimages", nargs="+", type=Path)
    arguments = parser.parse_args()
    for appimage in arguments.appimages:
        check_appimage(appimage.resolve())
