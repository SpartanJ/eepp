# Build Instructions (Debug Mode)

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

The valid OS directory names are: `windows`, `macosx`, `linux`, `bsd`, `haiku`.

Run the following command, replacing `<os_name>` with the correct environment:
`make -C make/<os_name> -j$(nproc)`

**Examples:**
*   Linux: `make -C make/linux -j$(nproc)`
*   macOS: `make -C make/macosx -j$(sysctl -n hw.ncpu)`
*   Windows: `make -C make/windows -j%NUMBER_OF_PROCESSORS%`
