# Unit Testing Requirements & Guidelines

This project relies on a comprehensive suite of unit tests to prevent regressions. You must ensure all existing tests pass after making modifications.

## Running Tests
The test binary manages its own current working directory, so you can execute it from anywhere.

*   **Prefer the release test binary during normal development:**
    When AddressSanitizer or other debug-only diagnostics are not required, build and run `bin/unit_tests/eepp-unit_tests`. The optimized release suite is substantially faster and should be the default for iterative testing. Use `bin/unit_tests/eepp-unit_tests-debug` when investigating memory safety, assertions, or other behavior that specifically requires the debug configuration.
*   **Default Execution on a Graphical Linux Desktop:**
    Unit-test windows are created hidden, so run the release suite directly against the desktop:
    `bin/unit_tests/eepp-unit_tests`
    This keeps the windows invisible and unfocused while preserving hardware OpenGL acceleration.
    Confirm that the renderer log names the real GPU rather than llvmpipe when validating rendering
    behavior or performance.
*   **Filtered Tests on a Graphical Linux Desktop:**
    Use the same direct hardware-backed command for focused runs:
    `bin/unit_tests/eepp-unit_tests --filter="FontRendering.*Offset*"`
*   **Headless CI and Systems Without a Usable Desktop Display:**
    Keep `projects/scripts/xvfb-run-eepp` as the fallback when no desktop display is available:
    `projects/scripts/xvfb-run-eepp bin/unit_tests/eepp-unit_tests`
    The wrapper provides a race-safe isolated display at `1280x1024x24` and injects
    `ASAN_OPTIONS=detect_leaks=0`. Xvfb can use llvmpipe, so do not use wrapper timings to evaluate
    hardware-rendered performance.
*   **Wrapper Fallback:**
    If `projects/scripts/xvfb-run-eepp` itself fails before launching the test binary, report that
    failure and use:
    `xvfb-run -a -s "-screen 0 1280x1024x24" bin/unit_tests/eepp-unit_tests`
    Do not use plain `xvfb-run` as the first headless attempt.
*   **Non-window Tests:**
    Tests known not to create windows can run without selecting a video driver:
    `bin/unit_tests/eepp-unit_tests --filter="NonWindowTest.*"`
*   **Filtering Tests:**
    Use the `--filter` parameter to run specific tests (supports glob patterns).
    Keep the same harness as the full suite: direct execution on a graphical Linux desktop, or the
    wrapper in a genuinely headless environment.

## Writing New Tests
Writing new tests is highly encouraged, but depends on the context of your changes:
*   **Core Framework (`eepp`):** If you add new logic, math, or framework-level features, you are **expected** to write unit tests for them.
*   **Application/Tools (`ecode`):** Application-level UI changes or tool integrations are often difficult to mock/test. Tests for these are **optional** and should only be added if practical to set up.

**Testing Workflow:**
1.  All tests are located in `src/tests/unit_tests/`.
2.  Before modifying code, run the existing tests most relevant to your change to ensure a baseline.
3.  For reference on how tests are structured in this project, review `src/tests/unit_tests/fontrendering.cpp` (the most complete set of text rendering tests).
