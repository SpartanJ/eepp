#!/usr/bin/env python3
"""Packaging integration checks using isolated inputs; no project build is required."""

import os
from pathlib import Path
import platform
import plistlib
import shutil
import subprocess
import tarfile
import tempfile
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[3]
POWERSHELL = os.environ.get("EEPP_TEST_POWERSHELL") or shutil.which("pwsh") or shutil.which("powershell")


class PackagingFixture(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="eepp packaging ")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name).resolve()
        shutil.copytree(ROOT / "projects/scripts", self.root / "projects/scripts")
        for target in ("linux", "freebsd", "haiku", "macos", "mingw32"):
            destination = self.root / "projects" / target
            destination.mkdir(parents=True)
            shutil.copy2(ROOT / "projects" / target / "build.app.sh", destination)
        shutil.copy2(ROOT / "projects/macos/create.dmg.sh", self.root / "projects/macos")
        (self.root / "projects/windows").mkdir()
        shutil.copy2(ROOT / "projects/windows/build.app.ps1", self.root / "projects/windows")
        self.app = self.root / "src/tools/sample"
        self.app.mkdir(parents=True)
        (self.app / "version.hpp").write_text(
            "#define SAMPLE_MAJOR_VERSION 1\n#define SAMPLE_MINOR_VERSION 2\n"
            "#define SAMPLE_PATCH_LEVEL 3\n"
        )
        (self.app / "assets.list").write_text(
            "# Shared resources\n@include shared.list\nassets/icon/sample.png\n"
        )
        (self.app / "shared.list").write_text("assets/ui\n")
        (self.app / "assets.macos.list").write_text(
            "assets/icon/shared.icns|assets/icon/sample.icns\n"
        )
        (self.app / "assets.windows.list").write_text("assets/ui/theme.css|platform/windows.css\n")
        self.source = self.root / "bin"
        for asset in ("ui/theme.css", "icon/sample.png", "icon/shared.icns"):
            path = self.source / "assets" / asset
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(asset)
        self.output = self.root / "output"
        self.env = dict(os.environ)


@unittest.skipIf(os.name == "nt", "Unix packagers are checked on Linux and macOS")
class AppPackaging(PackagingFixture):
    def setUp(self):
        super().setUp()
        self.commands = self.root / "commands"
        self.commands.mkdir()
        self.env = dict(os.environ, PATH=str(self.commands) + os.pathsep + os.environ["PATH"])
        self.command("strip", "exit 0")
        self.command("nproc", "echo 2")
        self.command("appimagetool", '[ -f "$1/sample.desktop" ] && touch "$2"')
        # ldd's paths are whitespace-delimited; use an external no-space runtime fixture.
        self.runtime = tempfile.TemporaryDirectory(prefix="eepp-sdl-")
        self.addCleanup(self.runtime.cleanup)
        runtime = Path(self.runtime.name) / "libSDL3.so.0"
        runtime.write_text("SDL3 runtime")
        self.command("ldd", f'echo "libSDL3.so.0 => {runtime} (0x1)"')
        (self.source / "libeepp.so").write_text("eepp runtime")
        (self.source / "sample").write_text('#!/bin/sh\nprintf "%s\\n" "$@"\n')
        (self.source / "sample").chmod(0o755)

    def command(self, name, body):
        path = self.commands / name
        path.write_text("#!/bin/sh\nset -e\n" + body + "\n")
        path.chmod(0o755)

    def run_script(self, script, *args, check=True):
        result = subprocess.run(
            ["bash", str(self.root / "projects" / script), *map(str, args)],
            env=self.env, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            cwd=self.root,
        )
        if check:
            self.assertEqual(result.returncode, 0, result.stdout)
        return result

    def package(self, target, *args, check=True):
        return self.run_script(
            f"{target}/build.app.sh", "sample", "--skip-build", "--arch", "x86_64",
            "--source-dir", self.source, "--output-dir", self.output, *args, check=check,
        )

    def test_manifest_includes_and_platform_mapping(self):
        self.run_script("scripts/copy_app_assets.sh", "sample", self.source, self.output, "macos")
        self.assertEqual((self.output / "assets/ui/theme.css").read_text(), "ui/theme.css")
        self.assertEqual((self.output / "assets/icon/sample.icns").read_text(), "icon/shared.icns")
        # Repeated directory copies merge without introducing ui/ui nesting.
        self.run_script("scripts/copy_app_assets.sh", "sample", self.source, self.output, "macos")
        self.assertFalse((self.output / "assets/ui/ui").exists())

    def test_common_assets_exclude_other_platforms(self):
        self.run_script("scripts/copy_app_assets.sh", "sample", self.source, self.output, "linux")
        self.assertFalse((self.output / "assets/icon/sample.icns").exists())

    def test_missing_asset_fails(self):
        (self.source / "assets/ui").rename(self.source / "assets/renamed")
        result = self.run_script("scripts/copy_app_assets.sh", "sample", self.source, self.output, check=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Missing asset", result.stdout)

    def test_linux_archives_and_launcher(self):
        self.package("linux", "--backend", "SDL3", "--version", "nightly")
        self.assertTrue((self.output / "sample-linux-nightly-x86_64.AppImage").is_file())
        archive = self.output / "sample-linux-nightly-x86_64.tar.gz"
        with tarfile.open(archive) as package:
            names = package.getnames()
            self.assertIn("sample/libs/libeepp.so", names)
            self.assertIn("sample/libs/libSDL3.so.0", names)
            self.assertNotIn("sample/AppRun", names)
            for name in ("sample/sample", "sample/sample.bin"):
                destination = self.output / "extracted" / name
                destination.parent.mkdir(parents=True, exist_ok=True)
                destination.write_bytes(package.extractfile(name).read())
                destination.chmod(0o755)
        result = subprocess.run(
            [str(self.output / "extracted/sample/sample"), "argument with spaces", "-f"],
            text=True, capture_output=True, check=True,
        )
        self.assertEqual(result.stdout.splitlines(), ["argument with spaces", "-f"])

    def test_freebsd_version_and_fresh_staging(self):
        self.package("freebsd", "--backend", "SDL3")
        (self.output / "sample.app/stale").touch()
        self.package("freebsd", "--backend", "SDL3")
        with tarfile.open(self.output / "sample-freebsd-1.2.3-x86_64.tar.gz") as package:
            self.assertNotIn("sample/stale", package.getnames())
            self.assertIn("sample/sample", package.getnames())

    def test_mingw_zip(self):
        for name in ("sample.exe", "eepp.dll", "SDL3.dll"):
            (self.source / name).write_text(name)
        self.package("mingw32", "--backend=SDL3")
        with zipfile.ZipFile(self.output / "sample-windows-1.2.3-x86_64.zip") as package:
            self.assertIn("sample/sample.exe", package.namelist())
            self.assertIn("sample/SDL3.dll", package.namelist())
            self.assertNotIn("sample/SDL2.dll", package.namelist())

    def test_invalid_cli_and_version(self):
        for args in (("--version",), ("--arch", "invalid"), ("--unknown",), ("--version", "../bad")):
            self.assertNotEqual(self.package("linux", *args, check=False).returncode, 0)
        (self.app / "version.hpp").write_text("#define SAMPLE_MAJOR_VERSION invalid\n")
        self.assertNotEqual(self.package("linux", check=False).returncode, 0)

    @unittest.skipUnless(
        platform.system() in ("Darwin", "Linux", "FreeBSD")
        and os.environ.get("EEPP_TEST_RESOURCE_BINARY"),
        "Set EEPP_TEST_RESOURCE_BINARY to the native unit-test executable",
    )
    def test_native_packaged_resource_paths(self):
        binary = Path(os.environ["EEPP_TEST_RESOURCE_BINARY"]).resolve()
        caller = self.root / "caller"
        (caller / "assets").mkdir(parents=True)
        env = dict(self.env)
        env["EEPP_TEST_RESOURCE_CWD"] = str(caller)
        library_path = "DYLD_LIBRARY_PATH" if platform.system() == "Darwin" else "LD_LIBRARY_PATH"
        env[library_path] = os.pathsep.join(
            filter(None, (str(binary.parent), str(binary.parent.parent), env.get(library_path)))
        )

        def check(executable, expected, appdir=None):
            env.pop("APPDIR", None)
            if appdir:
                env["APPDIR"] = str(appdir)
            env["EEPP_TEST_RESOURCE_PATH"] = str(expected) + "/"
            result = subprocess.run(
                [str(executable), "--filter=SystemPath.resourcePathUsesExpectedDirectory"],
                env=env, cwd=caller, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
            )
            self.assertEqual(result.returncode, 0, result.stdout)
            self.assertIn("[  PASSED  ] 1 tests.", result.stdout)

        ordinary = self.root / "ordinary"
        (ordinary / "assets").mkdir(parents=True)
        shutil.copy2(binary, ordinary / "resource-tests")
        # Ordinary executables ignore both APPDIR and unrelated assets in the caller's cwd.
        check(ordinary / "resource-tests", ordinary, caller)
        (ordinary / "assets").rmdir()
        check(ordinary / "resource-tests", ordinary, caller)

        if platform.system() == "Darwin":
            bundle = self.root / "renamed-tool.app/Contents"
            executable_dir = bundle / "MacOS"
            resources = bundle / "Resources"
            executable_dir.mkdir(parents=True)
            (resources / "assets").mkdir(parents=True)
            shutil.copy2(binary, executable_dir / "resource-tests")
            check(executable_dir / "resource-tests", resources)
            (executable_dir / "assets").mkdir()
            check(executable_dir / "resource-tests", resources)
            (resources / "assets").rmdir()
            check(executable_dir / "resource-tests", executable_dir)
        else:
            appdir = self.root / ".mount_resource-test"
            executable_dir = appdir / "usr/bin"
            (executable_dir / "assets").mkdir(parents=True)
            (appdir / "assets").mkdir()
            shutil.copy2(binary, executable_dir / "resource-tests")
            check(executable_dir / "resource-tests", executable_dir, appdir)
            (executable_dir / "assets").rmdir()
            check(executable_dir / "resource-tests", appdir, appdir)
            check(executable_dir / "resource-tests", caller)
            (caller / "assets").rmdir()
            check(executable_dir / "resource-tests", executable_dir)

    @unittest.skipUnless(
        platform.system() == "Darwin" and os.environ.get("EEPP_TEST_SDL3_DYLIB")
        and shutil.which("premake5"),
        "Set EEPP_TEST_SDL3_DYLIB to run the native macOS SDK/DMG check",
    )
    def test_macos_sdk_apps_and_dmg(self):
        # Premake is a small native executable with no non-system dependencies.
        # Apple's /usr/bin tools use arm64e, which differs from app builds' arm64.
        minimal_bundle = self.source / "sample.app/Contents/MacOS"
        minimal_bundle.mkdir(parents=True)
        (self.source / "sample").unlink()
        shutil.copyfile(shutil.which("premake5"), minimal_bundle / "sample")
        (minimal_bundle / "sample").chmod(0o755)
        vendor = self.root / "sdl3"
        (vendor / "lib").mkdir(parents=True)
        shutil.copy2(os.environ["EEPP_TEST_SDL3_DYLIB"], vendor / "lib/libSDL3.0.dylib")
        self.command("brew", f'echo "{vendor}"')
        (self.root / "include").mkdir()
        (self.root / "docs/articles").mkdir(parents=True)
        (self.root / "libs/macosx").mkdir(parents=True)
        arch = platform.machine()
        self.run_script("scripts/package_sdk.sh", "macos", arch, "--version", "eepp-9.8.7")
        sdk = self.root / "projects/macos/sdk"
        self.assertTrue((sdk / f"sample-macos-9.8.7-{arch}.dmg").is_file())
        bundle = sdk / "eepp-sdk/Applications/sample.app"
        plist = plistlib.loads((bundle / "Contents/Info.plist").read_bytes())
        self.assertEqual(plist["CFBundleShortVersionString"], "1.2.3")
        self.assertTrue((bundle / "Contents/MacOS/assets/ui/theme.css").is_file())
        subprocess.run(["codesign", "--verify", "--deep", "--strict", str(bundle)], check=True)
        with tarfile.open(sdk / f"eepp-sdk-macos-{arch}-9.8.7.tar.gz") as package:
            self.assertIn("eepp-sdk/Applications/sample.app/Contents/MacOS/sample", package.getnames())


@unittest.skipUnless(
    POWERSHELL and (os.name == "nt" or os.environ.get("EEPP_TEST_POWERSHELL")),
    "Windows PowerShell checks require Windows or an explicit EEPP_TEST_POWERSHELL executable",
)
class PowerShellPackaging(PackagingFixture):
    def setUp(self):
        super().setUp()
        for name in ("sample.exe", "sample.pdb", "SDL2.dll", "SDL3.dll"):
            (self.source / name).write_text(name)
        for arch in ("x86_64", "arm64"):
            directory = self.root / "libs/windows" / arch
            directory.mkdir(parents=True)
            (directory / "eepp.dll").write_text(f"eepp-{arch}")

    def package_windows(self, *, app="sample", source=None, script=None, arch="x64",
                        backend="sdl3", version=None, check=True):
        command = [
            POWERSHELL, "-NoProfile", "-NonInteractive", "-File",
            str(script or self.root / "projects/windows/build.app.ps1"),
            "-App", app, "-SkipBuild", "-Arch", arch, "-Backend", backend,
            "-SourceDir", str(source or self.source), "-OutputDir", str(self.output),
        ]
        if version is not None:
            command += ["-Version", version]
        result = subprocess.run(
            command, env=self.env, cwd=self.root, encoding="utf-8", errors="replace",
            stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=60,
        )
        if check:
            self.assertEqual(result.returncode, 0, result.stdout)
        return result

    def test_windows_zip_assets_and_fresh_staging(self):
        self.package_windows()
        archive = self.output / "sample-windows-1.2.3-msvc-x86_64.zip"
        with zipfile.ZipFile(archive) as package:
            self.assertIsNone(package.testzip())
            self.assertTrue({
                "sample/sample.exe", "sample/eepp.dll", "sample/SDL3.dll",
                "sample/assets/ui/theme.css", "sample/assets/icon/sample.png",
                "sample/platform/windows.css",
            } <= set(package.namelist()))
            self.assertEqual(package.read("sample/eepp.dll"), b"eepp-x86_64")
            self.assertEqual(package.read("sample/platform/windows.css"), b"ui/theme.css")
            self.assertNotIn("sample/SDL2.dll", package.namelist())
            self.assertNotIn("sample/assets/icon/sample.icns", package.namelist())
            self.assertNotIn("sample/sample.pdb", package.namelist())
        with zipfile.ZipFile(self.output / "sample-windows-1.2.3-msvc-x86_64-pdb.zip") as symbols:
            self.assertEqual(symbols.read("sample.pdb"), b"sample.pdb")

        (self.output / "sample/stale.txt").touch()
        with zipfile.ZipFile(archive, "a") as package:
            package.writestr("sample/stale-archive.txt", "old package")
        (self.source / "assets/ui/theme.css").write_text("updated theme")
        (self.source / "eepp.dll").write_text("source runtime")
        self.package_windows()
        with zipfile.ZipFile(archive) as package:
            self.assertNotIn("sample/stale.txt", package.namelist())
            self.assertNotIn("sample/stale-archive.txt", package.namelist())
            self.assertEqual(package.read("sample/assets/ui/theme.css"), b"updated theme")
            self.assertEqual(package.read("sample/eepp.dll"), b"source runtime")

    def test_windows_version_label_architecture_and_backend(self):
        self.package_windows(arch="arm64", backend="sdl2", version="nightly")
        with zipfile.ZipFile(self.output / "sample-windows-nightly-msvc-arm64.zip") as package:
            self.assertEqual(package.read("sample/eepp.dll"), b"eepp-arm64")
            self.assertIn("sample/SDL2.dll", package.namelist())
            self.assertNotIn("sample/SDL3.dll", package.namelist())

    def test_windows_missing_asset_and_invalid_versions(self):
        result = self.package_windows(version="../bad", check=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Invalid version label", result.stdout)
        (self.source / "assets/ui").rename(self.source / "assets/renamed")
        self.assertNotEqual(self.package_windows(check=False).returncode, 0)
        self.assertFalse(list(self.output.glob("*.zip")))
        (self.app / "version.hpp").write_text("#define SAMPLE_MAJOR_VERSION invalid\n")
        result = self.package_windows(check=False)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Missing integer version field", result.stdout)

    @unittest.skipUnless(
        os.environ.get("EEPP_TEST_WINDOWS_APPS"),
        "Set EEPP_TEST_WINDOWS_APPS to package existing Windows application builds",
    )
    def test_windows_built_applications(self):
        apps = [app.strip() for app in os.environ["EEPP_TEST_WINDOWS_APPS"].split(",") if app.strip()]
        self.assertTrue(apps, "EEPP_TEST_WINDOWS_APPS must contain an application name")
        source = Path(os.environ.get("EEPP_TEST_WINDOWS_BIN", ROOT / "bin")).resolve()
        for app in apps:
            with self.subTest(app=app):
                self.package_windows(app=app, source=source,
                                     script=ROOT / "projects/windows/build.app.ps1", version="ci-check")
                with zipfile.ZipFile(self.output / f"{app}-windows-ci-check-msvc-x86_64.zip") as package:
                    self.assertIsNone(package.testzip())
                    for name in (f"{app}.exe", "eepp.dll", "SDL3.dll"):
                        data = package.read(f"{app}/{name}")
                        self.assertTrue(data.startswith(b"MZ"), f"Not a Windows binary: {name}")
                        original = source / name
                        if name == "eepp.dll" and not original.is_file():
                            original = ROOT / "libs/windows/x86_64/eepp.dll"
                        self.assertEqual(data, original.read_bytes())
                    for name in ("assets/ui/breeze.css", "assets/fonts/DejaVuSansMono.ttf",
                                 f"assets/icon/{app}.png"):
                        self.assertEqual(package.read(f"{app}/{name}"), (source / name).read_bytes())


if __name__ == "__main__":
    unittest.main()
