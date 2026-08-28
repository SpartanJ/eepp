# Project Architecture: eepp & ecode

This repository contains two primary components: a core framework (`eepp`) and an application built on top of it (`ecode`).

## 1. eepp (Core Framework)
[eepp](https://github.com/SpartanJ/eepp/) is an open-source, cross-platform game and application development framework. It is heavily focused on providing robust technology for rich, hardware-accelerated Graphical User Interfaces (GUIs).

## 2. ecode (Application)
[ecode](https://github.com/SpartanJ/ecode/) is a lightweight, multi-platform code editor designed for responsiveness and performance.
*   **Relationship:** `ecode` is built *using* the `eepp` GUI framework. It acts as the primary real-world consumer of `eepp`.
*   **Goal:** Development on `ecode` is often used to test, improve, and drive new features in the underlying `eepp` library.

## Documentation & Code References
When working on this project, rely on the following resources to understand existing implementations:
*   **C++ Headers (Primary Reference):** Rely heavily on Doxygen documentation found directly inside the class headers located at `include/eepp/`.
*   **Basic Documentation:** Found in `docs/articles/`.
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
