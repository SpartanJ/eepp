# Packaging applications

Each platform has one application packager. Pass the Premake target name as the first argument:

```sh
projects/linux/build.app.sh eterm
projects/macos/build.app.sh eeiv --dmg
projects/mingw32/build.app.sh eproc --backend SDL3 --arch x86_64
projects/freebsd/build.app.sh eefm
projects/haiku/build.app.sh ecode
```

On Windows with MSVC:

```powershell
projects/windows/build.app.ps1 -App eterm -Backend sdl3 -Arch x64
```

The registered applications are `ecode`, `eterm`, `eeiv`, `eproc` and `eefm`. Outputs go to
`projects/<platform>/<app>/`. Linux produces an AppImage and a tar.gz, FreeBSD and Haiku produce
a tar.gz, Windows produces a zip, and macOS produces an app bundle with an optional DMG. The
MinGW platform directory is `mingw32`, while its archives use the name `windows`.
The existing `projects/<platform>/ecode/` entry points forward to these packagers.

## Assets

An application's `src/tools/<app>/assets.list` declares its packaged files, one path per line,
relative to the source `bin/` directory. Directories include their entire contents. Blank lines
and lines starting with `#` are ignored. Shared manifests are included with `@include`; include
paths are relative to the manifest containing the directive:

```text
@include ../../../projects/scripts/app-assets.list
assets/icon/myapp.png
assets/colorschemes/terminalcolorschemes.conf
```

`projects/scripts/app-assets.list` contains the base fonts, icon fonts and Breeze theme. Add
application-specific resources to the application's manifest rather than to platform scripts.

An optional `assets.<platform>.list` adds platform-specific files after the common list.
Platform names are `linux`, `macos`, `windows`, `freebsd` and `haiku`. A line can map a source to
a different destination using `source|destination`; both paths are relative to the source bin
or package resource directory respectively. For example, macOS can export an icon under the
application's bundle name:

```text
assets/icon/myapp.icns|myapp.icns
```

Use the same manifests independently of packaging:

```sh
projects/scripts/copy_app_assets.sh eterm ./bin /tmp/eterm-resources macos
```

```powershell
projects/scripts/copy_app_assets.ps1 -App eterm -SourceDir ./bin -DestDir ./eterm -Platform windows
```

These helpers merge files into the destination; the build packagers create a fresh staging
directory on every run. Missing declared assets fail packaging.

## Versions and new applications

Every distributable application has `src/tools/<app>/version.hpp`. Define three decimal integer
macros with the uppercase application name as the prefix (`-` becomes `_`):

```cpp
#define MYAPP_MAJOR_VERSION 0
#define MYAPP_MINOR_VERSION 1
#define MYAPP_PATCH_LEVEL 0
```

These fields match ecode's existing convention. Applications may add their own version API and
other metadata to the header. Packaging reads only these three macros. The new tool headers
start at `0.1.0`; ecode retains its existing version.

The default archive label is `major.minor.patch`. `--version nightly` (or PowerShell
`-Version nightly`) overrides artifact names; macOS bundle version fields remain numeric and
come from the header. This lets SDK release labels differ from individual tool versions.

### Adding an application

1. Add the executable target to `premake5.lua` and `premake4.lua`, following a neighboring tool.
   Use the same lowercase identifier for the target and `src/tools/<app>/`, for example `myapp`.
   Ensure the target is included in the platform's release build before selecting it for CI
   verification.
2. Add `src/tools/myapp/version.hpp` with the three integer macros shown above, and an
   `assets.list` containing the shared manifest and every application-specific resource:

   ```text
   @include ../../../projects/scripts/app-assets.list
   assets/icon/myapp.png
   ```

   Add platform overlays for resources that differ by OS. For example, `assets.macos.list`
   can contain `assets/icon/myapp-macos.png` and `assets/icon/myapp.icns`.
3. Add the SVG icon under `bin/assets/icon/` and register it in `APPS` in
   `projects/scripts/generate_app_icons.py`, following an existing application. Generate its
   PNG and platform icons with `--app myapp`; use `--macos-only` when generating just the macOS
   variants. Include the generated runtime icons in the manifests and use
   `Sys::getResourcePath( "myapp" )` when locating the application's resources.
4. Package the release build with the generic platform entry point and verify its output using
   the checks below. New applications use these existing entry points; no application-specific
   packaging wrapper is needed.

The SDK packagers discover tools through `src/tools/*/assets.list`. Adding a manifest registers
the tool for SDK packaging; the executable must also have been built for that platform.

## macOS bundles and DMGs

```sh
projects/macos/build.app.sh eterm --backend SDL3 --dmg
projects/macos/build.app.sh ecode --use-frameworks
# Sign/notarize the app before creating the release DMG when using Developer ID signing.
projects/macos/create.dmg.sh ecode --version nightly
```

The macOS packager uses the existing `make_no_fw.sh` / `make.sh` build workflows. SDL2 is the
default backend; `--use-frameworks` selects the official SDL2 framework. An explicit architecture
that differs from the host uses Premake 5 through the same build scripts. Cross builds need
libraries containing the requested architecture; for example, the official SDL2 framework
supports the Intel ecode cross-build entry point.

Assets live in `Contents/Resources/assets`. `Sys::getResourcePath()` resolves the resource base
for macOS bundles and Linux/BSD AppImages, and returns the executable directory for ordinary
builds. Pass the application name, such as `Sys::getResourcePath( "eterm" )`, to also resolve
Emscripten's `<app>/assets` preload layout when present, with shared `assets/` as the fallback.
Web resource paths are absolute, so they remain valid when the application changes its working
directory.
`UIApplication` uses the unnamed resource base by default; set `Settings::basePath` to the named
resource base for app-specific Web preloads. Applications can append `assets/` to this base to
locate their assets. A symlink in `Contents/MacOS` also supports applications using
executable-relative assets. Runtime libraries are discovered from Mach-O dependencies, copied
into `Contents/MacOS`, relocated and ad-hoc signed along with the bundle. System libraries remain
provided by macOS. An app-specific `Info.plist.tpl` is used when present, preserving ecode's
document associations; other applications receive a standard plist
and their manifest's ICNS icon. Templates can use `APP_NAME`, `APP_VERSION_STRING`,
`APP_MAJOR_VERSION`, `APP_MINOR_VERSION` and `APP_PATCH_LEVEL` placeholders. Generate application
icons from their SVG sources with `projects/scripts/generate_app_icons.py --app <app> --macos-only`.

If Finder or the Dock still shows an older icon, quit the application and run:

```sh
projects/scripts/reset-macos-icon-cache
```

The script moves the current user's persistent Dock and IconServices caches into a temporary
backup directory, prints its location, and restarts IconServices, Finder and the Dock. Reopen
the application afterward to load the regenerated runtime icon.

DMG creation uses macOS's built-in `hdiutil` and includes the app and an Applications shortcut.
`--dmg` creates it immediately; the separate `create.dmg.sh` supports the existing release
signing/notarization sequence.

For an existing build, skip compilation and optionally select a staged source/output directory:

```sh
projects/macos/build.app.sh eproc --skip-build --dmg --arch arm64 \
  --source-dir /path/to/sdk/bin --output-dir /tmp/packages --version nightly
```

The source directory must contain the executable, declared assets and its runtime libraries,
or the dependencies must be resolvable from their original build locations. Premake's minimal
`bin/<app>.app` executable layout is also supported. `--skip-build`, `--source-dir` and
`--output-dir` are available on all shell packagers; MSVC uses `-SkipBuild`, `-SourceDir` and
`-OutputDir`.

## SDK packages

`projects/scripts/package_sdk.sh macos <arch>` creates standalone DMGs next to the SDK archive
and includes the app bundles in the SDK's `Applications/` directory. It reuses the relocated
SDK binaries and SDL3 runtime without rebuilding them. CI uploads and publishes the DMGs with
the SDK archive. Other SDK platforms continue to include the registered tools as executables.

## Verifying a new application

Run the packaging regression suite from the repository root:

```sh
python3 projects/scripts/tests/test_app_packaging.py
```

The suite uses temporary directories and skips compilation. Unix hosts check the shell
packagers. Windows checks the PowerShell packager's manifest includes, platform overlays, DLL
selection, version labels, symbol archives and removal of stale package contents, and skips
the Unix-only checks. Set `EEPP_TEST_POWERSHELL` to explicitly enable the PowerShell checks on
another host. The generic fixture tests cover new tools that follow the same conventions
without adding a test for each tool.

After an x64 MSVC/SDL3 release build, check an application's actual Windows executable and assets:

```powershell
$env:EEPP_TEST_WINDOWS_APPS = 'myapp'
python projects/scripts/tests/test_app_packaging.py PowerShellPackaging
```

This invokes `projects/windows/build.app.ps1 -SkipBuild`, verifies ZIP integrity, compares the
packaged executable and runtime DLLs with the build outputs, and checks the app icon, Breeze
theme and base font. It expects `bin/myapp.exe`, `bin/SDL3.dll`, the declared assets, and
`eepp.dll` in either `bin/` or `libs/windows/x86_64/`. The required app icon follows the
`assets/icon/myapp.png` naming convention. Packaging declares all other app resources through
the manifests; exercise features that load those resources when checking the packaged app.

`EEPP_TEST_WINDOWS_APPS` accepts a comma-separated list. To add a built tool to the Windows CI
check, append its identifier to this variable in the **Application packaging tests** step of
`.github/workflows/eepp-windows-build-check.yml`, for example `eproc,myapp`. The step reuses the
existing build, and reports failures per application. It currently selects `eproc`.

Optional environment variables select other inputs:

| Variable | Purpose |
| --- | --- |
| `EEPP_TEST_WINDOWS_BIN` | Source directory containing Windows binaries and assets; defaults to the repository's `bin/`. |
| `EEPP_TEST_POWERSHELL` | PowerShell executable; also enables these checks on Unix hosts. On Windows the suite otherwise searches for `pwsh`, then `powershell`. |
| `EEPP_TEST_RESOURCE_BINARY` | Native unit-test executable used to check staged macOS bundle and Linux/BSD AppImage resource paths. |
| `EEPP_TEST_SDL3_DYLIB` | SDL3 dylib used by the macOS SDK/app/DMG integration fixture. |

Linux and macOS CI run the generic suite and native resource-path checks. macOS also enables
the SDK/DMG fixture. To inspect a new tool's real macOS or Linux package, use its platform
packager with `--skip-build` after compiling it, then launch the packaged copy. Keep the launch
working directory outside the checkout to verify that the package supplies its own resources.
