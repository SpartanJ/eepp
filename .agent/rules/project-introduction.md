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
