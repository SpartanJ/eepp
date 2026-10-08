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

If the SDL2/SDL3 runtime library was not bundled next to the binaries (see
`bin/`), install it with your system package manager (for example
`libsdl2-2.0-0` on Debian/Ubuntu).

## Linking against eepp

Add `include/` to your include path and link against the libraries in
`libs/<platform>/<arch>/`:

- Dynamic linking: link against the shared library (`eepp.dll` / `libeepp.so` /
  `libeepp.dylib` and the matching import library when applicable).
- The optional modules (`eepp-maps`, `eepp-physics`) and the `eterm` /
  `languages-syntax-highlighting` static libraries are also provided.

Start with the documentation in `docs/articles/` and the sources of the shipped
examples (available in the repository under `src/examples/`) to learn the API.

## Notes

- These are unstable nightly builds; APIs may change at any time.
- Report issues at https://github.com/SpartanJ/eepp/issues
