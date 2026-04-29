# Unit Testing Requirements & Guidelines

This project relies on a comprehensive suite of unit tests to prevent regressions. You must ensure all existing tests pass after making modifications.

## Running Tests
The test binary manages its own current working directory, so you can execute it from anywhere.

*   **Standard Execution:**
    `bin/unit_tests/eepp-unit_tests-debug`
*   **Linux & FreeBSD Execution (Required for Desktop Environments):**
    Tests open ~50 individual windows. To prevent disrupting the desktop environment, run them in an isolated framebuffer using `xvfb-run`:
    `ASAN_OPTIONS=detect_leaks=0 xvfb-run -a -s "-screen 0 1280x1024x24" bin/unit_tests/eepp-unit_tests-debug`
*   **Filtering Tests:**
    Use the `--filter` parameter to run specific tests (supports glob patterns).
    *Example (runs all tests with "Offset" in the name):*
    `bin/unit_tests/eepp-unit_tests-debug --filter="FontRendering.*Offset*"`

## Writing New Tests
Writing new tests is highly encouraged, but depends on the context of your changes:
*   **Core Framework (`eepp`):** If you add new logic, math, or framework-level features, you are **expected** to write unit tests for them.
*   **Application/Tools (`ecode`):** Application-level UI changes or tool integrations are often difficult to mock/test. Tests for these are **optional** and should only be added if practical to set up.

**Testing Workflow:**
1.  All tests are located in `src/tests/unit_tests/`.
2.  Before modifying code, run the existing tests most relevant to your change to ensure a baseline.
3.  For reference on how tests are structured in this project, review `src/tests/unit_tests/fontrendering.cpp` (the most complete set of text rendering tests).
