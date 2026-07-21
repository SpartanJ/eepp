# Unit Testing Requirements & Guidelines

This project relies on a comprehensive suite of unit tests to prevent regressions. You must ensure all existing tests pass after making modifications.

## Running Tests
The test binary manages its own current working directory, so you can execute it from anywhere.

*   **Default Execution for Agents on Linux & FreeBSD:**
    Always run unit tests through the project wrapper unless the user explicitly asks for a different harness:
    `projects/scripts/xvfb-run-eepp bin/unit_tests/eepp-unit_tests-debug`
*   **Why the wrapper is required:**
    Tests open ~400 individual windows. The wrapper runs them in an isolated framebuffer, enables race-safe automatic display selection for concurrent agent test runs, sets the default screen to `1280x1024x24`, and injects `ASAN_OPTIONS=detect_leaks=0` automatically.
*   **Do not skip the wrapper for filtered tests:**
    A focused test still needs the same wrapper:
    `projects/scripts/xvfb-run-eepp bin/unit_tests/eepp-unit_tests-debug --filter="FontRendering.*Offset*"`
*   **Fallback only when the wrapper itself fails:**
    If `projects/scripts/xvfb-run-eepp` fails before launching the test binary, report that wrapper failure and then use this fallback to keep verification moving:
    `ASAN_OPTIONS=detect_leaks=0 xvfb-run -a -s "-screen 0 1280x1024x24" bin/unit_tests/eepp-unit_tests-debug`
    Do not use plain `xvfb-run` as the first attempt for GUI/unit tests.
*   **Direct Execution (Only for non-window tests or explicit user requests):**
    `bin/unit_tests/eepp-unit_tests-debug`
*   **Filtering Tests:**
    Use the `--filter` parameter to run specific tests (supports glob patterns).
    Keep the wrapper in front of the binary unless the test is known not to create windows.

## Writing New Tests
Writing new tests is highly encouraged, but depends on the context of your changes:
*   **Core Framework (`eepp`):** If you add new logic, math, or framework-level features, you are **expected** to write unit tests for them.
*   **Application/Tools (`ecode`):** Application-level UI changes or tool integrations are often difficult to mock/test. Tests for these are **optional** and should only be added if practical to set up.

**Testing Workflow:**
1.  All tests are located in `src/tests/unit_tests/`.
2.  Before modifying code, run the existing tests most relevant to your change to ensure a baseline.
3.  For reference on how tests are structured in this project, review `src/tests/unit_tests/fontrendering.cpp` (the most complete set of text rendering tests).
