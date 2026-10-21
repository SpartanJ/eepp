# Project Architecture: eepp & ecode

This repository contains the core framework (`eepp`) and several applications and tools built on
top of it, including `ecode`, `eterm`, and `eeiv`.

## 1. eepp (Core Framework)

[eepp](https://github.com/SpartanJ/eepp/) is an open-source, cross-platform game and application development framework. It is heavily focused on providing robust technology for rich, hardware-accelerated Graphical User Interfaces (GUIs).

## 2. ecode (Application)

[ecode](https://github.com/SpartanJ/ecode/) is a lightweight, multi-platform code editor designed for responsiveness and performance.
*   **Relationship:** `ecode` is built *using* the `eepp` GUI framework. It acts as the primary real-world consumer of `eepp`.
*   **Goal:** Development on `ecode` is often used to test, improve, and drive new features in the underlying `eepp` library.

## 3. eterm (Terminal Emulator)

`eterm` is the repository's terminal emulator. Its name is always written in lowercase: `eterm`,
not `eTerm`. Its implementation lives under `src/modules/eterm/`.

## 4. eeiv (Image Viewer)

`eeiv` is the repository's image viewer. Its application entry point is
`src/tools/eeiv/eeiv.cpp`.

## 5. eefm (File Manager)

`eefm` is the repository's file manager, a Dolphin-like application built on the `eepp-vfs`
module (`src/modules/vfs/`). Its implementation lives under `src/tools/eefm/`. `App` (`app.cpp`)
owns the window, layout, state and shortcuts, and the parts, which reach each other through it (as
in ecode): `PaneManager` (split panes, tabs, navigation, status and filter bars),
`FileOperations` (context menus, clipboard, drag and drop, transfers, trash), `PlacesController`
with `PlacesPanel` / `Places`, `TerminalPanel` and `SettingsActions`. `FileTableView`
(`fileview.cpp`) is the drag-enabled table view; desktop integration lives in `MimeDatabase`,
`Applications` and `Trash`. Browsing and file operations must go through the VFS so local, packed and remote
(`sftp://`) locations behave the same; only desktop integration (trash, mounted devices, MIME
database, launching applications) may use local platform APIs. See `docs/articles/vfs.md`.

## AI Agent Runtime Checks

Native is the default runtime. Leave `EEPP_RUNTIME` unset for normal native runs; set it to
`native` only to override an inherited non-native value or to test explicit runtime selection.

AI agents should prefer `EEPP_RUNTIME=headless` when launching ordinary applications or examples
and when automating their UI with the inspector. This avoids opening desktop windows or interrupting
the human's keyboard input and focus. When headless rendering is unavailable (notably on macOS),
or native rendering needs verification, use `EEPP_WINDOW_HIDDEN=1` instead of
modifying application sources to add `WindowStyle::Hidden`. This also keeps secondary native windows
hidden, even if the application calls `show()` or restores them. Coordinate interactive checks that
require visible native windows with the user before driving them.

Unit tests and benchmarks are different: run their release binaries with the native runtime on a
usable desktop. The unit-test build defines `EE_UNIT_TESTS`, which adds `WindowStyle::Hidden` to
the default window style; benchmarks that create windows explicitly request `WindowStyle::Hidden`.
These hidden windows preserve native rendering without appearing or taking desktop focus. Keep
test and benchmark windows hidden when adding or changing cases. Do not select the headless runtime
solely to avoid desktop interruption for these harnesses.

Use headless tests when no usable display is available, when requested, or when testing the headless
runtime itself. Headless rendering can use a software renderer, so verify the renderer before
assessing GPU behavior or performance. See `.agents/rules/unit-tests.md` for commands and fallbacks.

## Documentation & Code References

When working on this project, rely on the following resources to understand existing implementations:
*   **C++ Headers (Primary Reference):** Rely heavily on Doxygen documentation found directly inside the class headers located at `include/eepp/`.
*   **Task-specific guides:** Use `docs/articles/` when the task touches an area below:
    *   Native application UI: `ui_authoring.md` for design patterns, `ui_layout_reference.md`
        for sizing and layout behavior, and `ui_css_for_applications.md` for styling and the
        native CSS cascade. Start with `ui_introduction.md` for a basic XML/CSS example.
    *   HTML in `UIWebView` or `UIMarkdownView`: `ui_html_compatibility.md` for the supported
        behavior and boundaries; `css_specification.md` for eepp CSS properties and values.
    *   Runtime UI debugging or interaction: `ui_inspector.md` explains how to enable the
        inspector and use `projects/scripts/eepp-inspect.py` to inspect live widget trees,
        geometry, CSS, focus, and nested WebView scenes, capture screenshots, and automate input.
        Use it when visual or interaction behavior needs runtime verification.
    *   UI direction, RTL support, or logical `start`/`end` layout properties: `ui_rtl.md`.
    *   UI data binding or charts: `ui_databinding.md` or `ui_charts.md`, respectively.
    *   `eterm` Kitty protocol work: `eterm_kitty_keyboard.md` and
        `eterm_kitty_graphics.md` for the corresponding supported behavior.
*   **Implementation Examples:** A wide variety of examples showing how to use the library are located in `src/examples/`.
*   **General Context:** The `README.md` at the root directory contains deeper project details.

## Architecture and Human Readability

Optimize code organization for the maintainer reading it. Correctness and performance are required;
they do not excuse architecture that requires a repository-wide search to understand a small change.
Minimize the concepts and file jumps needed to follow an operation, rather than optimizing only for
short files or fewer lines.

Before adding code, inspect the existing public class, its implementation, and the relevant backend.
Identify who owns the state, who calls the operation, and who cleans it up. Extend those existing
responsibilities where they fit. Do not introduce a manager, adapter, friend, callback bridge, or
virtual hook merely because another backend might exist someday; each boundary must serve a concrete
current responsibility or lifetime requirement.

- **Make file names navigable.** An implementation unit with externally used declarations must have
  a matching header basename. Put its declarations in that header and its definitions in the matching
  source. For example, `DragManager::invalidateWindow()` belongs in `dragmanager.hpp/.cpp`, rather than
  being declared in `dragcontext.hpp` and defined in `dragmanager.cpp`. Include the matching header
  first where platform setup permits. A source compiled on every platform but wrapped entirely in a
  platform guard includes only `<eepp/config.hpp>` before the guard, and its matching header first
  inside it, so other platforms do not parse its headers. Entry points and tests need no empty
  companion headers; genuinely header-only interfaces and templates need no empty source files.
- **Give subsystem operations a named container.** Group related operations in a class or struct
  instead of exposing a collection of namespace-level functions. Use static methods for a
  process-wide service such as `DragManager`, with its state in private static members. A reader
  should discover the available operations by opening the type or using `DragManager::` completion.
  Keep method names concise within that scope, such as `DragManager::poll()` and
  `DragManager::invalidateWindow()`, rather than repeating the subsystem name in every method.
  Keep genuinely file-local implementation helpers local; do not introduce singleton instances,
  heap allocations, or forwarding layers just to group functions.
- **Keep responsibilities together.** Implement a public class's methods in its corresponding source.
  A manager owns coordination and lifetime; a platform implementation owns native behavior; a context
  describes borrowed handles. Do not scatter a class's methods among unrelated helper sources or use
  a generic `context`, `common`, or `utils` header as a collection of unrelated declarations. Preserve
  established platform-specific splits when they represent a real backend boundary.
  Shared backend contracts belong in a virtual base; each existing backend implements its own
  concrete class in its backend directory. Do not combine those implementations into a shared
  class that switches backend behavior with preprocessor branches.
- **Keep dependencies explicit.** Headers must include what their declarations require. Callers
  include the header that owns the operation they use. Platform factory declarations and availability
  checks belong with that platform's implementation, not in a shared session/interface header.
  Implementation-only helpers stay local to their source file. Keep internal plumbing out of public
  installed headers unless consumers actually need it.
- **Prefer direct control flow.** Use an existing object's interface directly when access and
  lifetime permit it. Avoid forwarding functions, wrapper classes, and access machinery that add no
  responsibility. A facade delegating to the owner of shared state is a meaningful boundary; several
  layers forwarding the same arguments without adding behavior are a signal to simplify.
- **Name and order code for reading.** Names should explain the responsibility, and declarations and
  definitions should follow the same logical order where practical. Related setup, operation, and
  cleanup paths must be easy to find. Comments explain ownership, invariants, and unavoidable platform
  constraints; they must not justify unnecessary indirection.

Before finishing, follow one normal operation and one cancellation/destruction path from the public
API. Check that each declaration leads to the source a reader would expect, ownership is apparent,
and dependencies follow the intended subsystem/backend boundaries. Fix surprising organization;
adding an explanation does not repair it. Keep this review proportional to the change rather than
creating new abstractions or architecture documents for their own sake.

User documentation teaches consumers how to use the API, including examples, results, ownership,
and supported behavior. Allocation audits, test transcripts, implementation history, and remaining
engineering tasks belong in development reports, not in consumer guides.

## Localization

The locale catalogs under `bin/assets/i18n/` belong to **ecode**; the other applications are not
currently localized. Whenever an ecode feature introduces or changes a user-facing i18n key, add or
update that key in every catalog in this directory. Do not rely only on the fallback string embedded
in the source. Keep all locale files structurally valid and verify that every supported catalog
contains the new or renamed key.

## C++ Virtual Method Style

Follow the convention already used by the class being edited. In particular, when a class declares
virtual methods without the `override` specifier, do not introduce `override` on new methods in that
class. Mixing the styles can enable Clang's inconsistent-missing-override warnings for the existing
declarations. A class-wide conversion is a separate change and must update all applicable methods
together.

## Namespace Style

Follow eepp's established namespace style: prefer the appropriate `using namespace EE::...`
declarations and unqualified eepp type names, such as `UISplitter`, over repeatedly spelling fully
qualified names such as `EE::UI::UISplitter`. Keep explicit qualification only where it is required
to resolve ambiguity or avoid importing an unusually broad namespace into an unsuitable scope.

## Function Spacing

Always leave one blank line between consecutive function declarations in headers and between
consecutive function definitions in implementation files. This is a project-wide stylistic
constraint, including short inline functions.

## Control-Statement Braces

Use braces around the body of an `if`, `else`, `for`, `while`, or similar control statement whenever
that body occupies more than one physical source line. A statement remains visually multi-line even
when C++ treats it as a single statement, so a wrapped function call must be braced:

```cpp
if ( condition ) {
	object->function(
		argument,
		anotherArgument );
}
```

An unbraced body is acceptable only when the complete body fits on one physical source line. Apply
this rule when writing or modifying code; do not add unrelated braces throughout untouched code.
