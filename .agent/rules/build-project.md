# Build Instructions

The build configurations in `.ecode/project_build.json` are the source of truth for the
developer's local build workflows. Check that file before selecting a generator, backend, or
build flags. In particular, do not use an AddressSanitizer build to evaluate runtime performance.

## Release Performance Builds (Linux)

For performance investigations, use the `eepp-linux-ninja` configuration from
`.ecode/project_build.json`. At the time of writing, its commands are:

`premake5 --disable-static-build --with-debug-symbols --with-backend=SDL3 ninja`

`ninja -C make/linux release`

This produces an optimized release build with debug symbols and without AddressSanitizer. Run the
release executable (for example, `bin/eepp-ui-html`) when measuring performance. Recheck
`.ecode/project_build.json` before use because the local configuration may change.

## Debug and Unit-Test Builds

All build commands must be executed from the **root project directory**. Follow these steps to build the project:

## Step 1: Regenerate Project Files
Always regenerate the project files before compiling or running tests after making changes. Do this even for edits to existing files, because the checked-in makefiles can be stale and may reference removed files or miss recently added targets.

*   **Tool:** Use `premake4` if installed; otherwise, fallback to `premake5` (the parameters are identical).
*   **Linker Flag (`--with-mold-linker`):** This flag is conditional. If the `mold` linker is installed on the system, you **must** include it to speed up linking. If `mold` is not installed, omit the flag.

**Command (if `mold` is installed):**
`premake4 --disable-static-build --with-mold-linker --with-debug-symbols --address-sanitizer gmake`

**Command (if `mold` is NOT installed):**
`premake4 --disable-static-build --with-debug-symbols --address-sanitizer gmake`

## Step 1a: Format Changed Files
After editing any C or C++ source file (`.c`, `.cpp`, `.h`, `.hpp`), you **must** run `clang-format` on all modified files to ensure consistent formatting with the project's style (defined in `.clang-format` at the repository root).

**Command (formats all currently modified tracked files at once):**
`git diff --name-only -- '*.c' '*.cpp' '*.h' '*.hpp' | xargs clang-format -i`

Run this **after** all edits and **before** attempting to compile.

---

## Step 2: Compile the Project
To compile the project in debug mode, execute the `make` command, ensuring you point to the correct directory for your current Operating System.

### Build Parallelism

Always use all processors reported by the platform when selecting the parallel job count. On Linux
and other systems with `nproc`, use `-j$(nproc)` exactly; do not substitute an arbitrary fixed value
such as `-j4`. Use the platform-equivalent processor-count command where `nproc` is unavailable.

The valid OS directory names are: `windows`, `macosx`, `linux`, `bsd`, `haiku`.

Run the following command, replacing `<os_name>` with the correct environment:
`make -C make/<os_name> -j$(nproc)`

**Examples:**
*   Linux: `make -C make/linux -j$(nproc)`
*   macOS: `make -C make/macosx -j$(sysctl -n hw.ncpu)`
*   Windows: `make -C make/windows -j%NUMBER_OF_PROCESSORS%`

## Running GUI Examples Under Xvfb

Xvfb does not support the multisampled OpenGL contexts requested by some eepp examples. In
particular, `src/examples/ui_html/ui_html.cpp` normally requests 4x MSAA. Launching that binary
through `xvfb-run` or `projects/scripts/xvfb-run-eepp` can therefore fail immediately with
`Could not create window, exiting`, even though the application works on a real display.

*   Do not treat this window-creation failure as evidence of a bug in the feature being tested.
*   Unit tests normally request a non-multisampled context and are unaffected.
*   For a temporary headless diagnostic of an example, make the diagnostic-only execution path
    request 0 MSAA, run it through `projects/scripts/xvfb-run-eepp`, and revert the temporary
    example instrumentation afterward.
*   Never weaken the example's normal graphics configuration merely to accommodate Xvfb.
