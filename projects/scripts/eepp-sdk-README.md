# eepp SDK

Self-contained snapshot of the [eepp](https://github.com/SpartanJ/eepp) framework:
runnable demos and tools, the assets they need, and the compiled libraries ready to be
linked against by your own applications.

Version: @SDK_VERSION@ ("nightly" is a rolling build of the latest development
changes; eepp-%d.%d.%d versions are stable releases)

## Package layout

```
bin/                    Tools and examples/demos (release builds)
bin/assets/             Assets required by the shipped binaries
bin/                    Runtime libraries co-located with the binaries
libs/<platform>/<arch>/ Libraries for linking against eepp
include/                Public C++ headers
docs/articles/          Framework documentation articles
```

## Running the demos and tools

Every binary in `bin/` is ready to run:

- **Windows:** double-click any `.exe`.
- **Linux:** run any binary from a file manager or a terminal, for example
  `bin/ecode`. The binaries locate the shared libraries relative to their own
  location (`$ORIGIN` rpath), so the package can live anywhere on disk.
- **macOS:** the shipped binaries and libraries are ad-hoc signed and resolve
  their dependencies through `@loader_path`, so they can be launched straight
  from Finder or a terminal.

The SDK uses the SDL3 backend and bundles its runtime library in `bin/`
(`SDL3.dll`, `libSDL3.so.0`, or `libSDL3.0.dylib`). Applications linked against
this SDK need SDL3; SDL2 is not a compatible replacement. When deploying your
own application, ship the matching SDL3 runtime alongside eepp or install SDL3
through the target system's package manager.

## Linking against eepp

Add `include/` to your include path and link against the libraries in
`libs/<platform>/<arch>/`. Use C++20 and the toolchain matching the SDK package:

- Dynamic linking: link against the shared library (`eepp.dll` / `libeepp.so` /
  `libeepp.dylib` and the matching import library when applicable).
- The optional modules (`eepp-maps`, `eepp-physics`) and the `eterm` /
  `languages-syntax-highlighting` static libraries are also provided.

For MSVC applications, include `<SDL3/SDL_main.h>` in exactly one source file and link
`SDL3.lib` alongside eepp, using the matching SDL3 development headers and import library.
SDL3 supplies the console/GUI startup wrappers through that header; there is no `SDL3main.lib`.
The repository's Premake 4 and Premake 5 projects add this startup code automatically.

Start with the documentation in `docs/articles/` and the sources of the shipped
examples (available in the repository under `src/examples/`) to learn the API.

## Native file dragging

`include/eepp/window/draganddrop.hpp` provides outbound local-file dragging. Query
`DragAndDrop::getCapabilities()` before enabling an external drag gesture. Windows, macOS, X11 and
Wayland sources are supported with SDL2 and SDL3. X11 and Wayland support requires their development
headers when building eepp; their client libraries load dynamically at runtime. An unavailable
backend returns `Unsupported`. The existing SDL file and text drop callbacks continue to work.

Started drags show a bitmap preview and action feedback. Supply an RGB/RGBA widget snapshot
through `DragOffer::preview`, or use the generated file/count icon. Snapshot pixels are copied during
`startDrag()`; the session never retains a widget or the supplied Image.

Unix consumers do not need to link `X11` or `wayland-client`. Static Windows builds need `shell32`
alongside eepp's OLE dependencies (`ole32`, `uuid`); the Premake scripts include them automatically.
See `docs/articles/native_drag_and_drop.md` for usage examples, source lifetime rules and supported
behavior.

## Building an SDK snapshot

Select SDL3 explicitly when building SDK artifacts; the repository also supports
SDL2 builds for applications that have not migrated yet. Clean the affected build
configuration when switching backends because object directories are shared.

```sh
# Linux: install the pinned SDL3 release, then build release binaries.
bash projects/linux/scripts/install_sdl3.sh
premake5 --disable-static-build --with-backend=SDL3 gmake
make -C make/linux -j$(nproc) config=release_x86_64
bash projects/scripts/package_sdk.sh linux x86_64 --strip

# Windows SDK through MinGW on Linux.
bash projects/mingw32/make.sh --backend=sdl3 config=release_x86_64 -j$(nproc)
bash projects/scripts/package_sdk.sh windows x86_64 --strip

# macOS: install SDL3 with Homebrew and use the supported build script.
brew install sdl3 premake
bash projects/macos/make_no_fw.sh --backend=SDL3 config=release_arm64
bash projects/scripts/package_sdk.sh macos arm64
```

For MSVC, generate with `premake5.exe --windows-vc-build --disable-static-build
--with-backend=SDL3 vs2022`, build the release solution, then run
`projects\scripts\package_sdk.ps1 -Platform msvc`. SDK CI workflows select SDL3
and the packagers require its runtime library.

## Notes

- These are unstable nightly builds; APIs may change at any time.
- Report issues at https://github.com/SpartanJ/eepp/issues
