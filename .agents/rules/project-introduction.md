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
module (`src/modules/vfs/`). Its implementation lives under `src/tools/eefm/`: `app.cpp` (window,
layout, state, shortcuts), `panes.cpp` (split panes, tabs, navigation, status and filter bars),
`fileops.cpp` (context menus, clipboard, drag and drop, transfers), `fileview.cpp` (drag-enabled
table view), `placespanel.cpp` / `places.cpp`, and desktop integration in `mime.cpp`, `apps.cpp`
and `trash.cpp`. Browsing and file operations must go through the VFS so local, packed and remote
(`sftp://`) locations behave the same; only desktop integration (trash, mounted devices, MIME
database, launching applications) may use local platform APIs. See `docs/articles/vfs.md`.

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
