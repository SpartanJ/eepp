# Unit Testing Requirements & Guidelines

This project relies on a comprehensive suite of unit tests to prevent regressions. You must ensure all existing tests pass after making modifications.

## Running Tests
The test binary manages its own current working directory, so you can execute it from anywhere.
CI unit-test steps set `SDL_AUDIODRIVER=dummy`, `SDL_AUDIO_DRIVER=dummy`, and
`ALSOFT_DRIVERS=null` to exercise playback without an audio device. Local runs use the normal
audio backend; set these environment variables explicitly when silent output is needed.

Native is the default runtime. Leave `EEPP_RUNTIME` unset for normal native runs; set it to
`native` only to override an inherited non-native value or to test explicit runtime selection.

The required default workflow is:

1. Build the release unit-test target (use ninja build).
2. On a usable desktop, run `bin/unit_tests/eepp-unit_tests` directly, without
   Xvfb. The unit-test build defines `EE_UNIT_TESTS`, making `WindowStyle::Default` include
   `WindowStyle::Hidden`; native test windows remain hidden and do not take desktop focus.
3. If sandbox restrictions prevent access to the host display, retry the same native command with
   elevated permissions. A sandbox display failure does not establish that the host lacks a display.
4. When no usable display is available, use `EEPP_RUNTIME=headless bin/unit_tests/eepp-unit_tests`.
   Also use it when requested or when checking the headless runtime itself.
5. Use Xvfb only if there is no usable desktop display and the required check cannot run with the
   headless runtime. The fallback commands below use the default native runtime.

The preference for headless application launches and interactive UI inspector checks does not
change this harness workflow. Keep all native test windows hidden, including secondary dialog
hosts; showing a UI widget inside a hidden host does not require showing the native window.

*   **Use the release test binary during normal development:**
    When AddressSanitizer or other debug-only diagnostics are not required, build and run `bin/unit_tests/eepp-unit_tests`. The optimized release suite is substantially faster and is the required default for iterative testing. Use `bin/unit_tests/eepp-unit_tests-debug` only when investigating memory safety, assertions, or other behavior that specifically requires the debug configuration.
*   **Default Execution on a Graphical Desktop:**
    `bin/unit_tests/eepp-unit_tests`
    Hidden windows preserve native rendering without appearing or taking desktop focus.
    Confirm that the renderer log names the real GPU rather than llvmpipe when validating rendering
    behavior or performance.
*   **Filtered Tests:**
    Use the same native runtime for focused runs on a usable desktop:
    `bin/unit_tests/eepp-unit_tests --filter="FontRendering.*Offset*"`
*   **Headless Execution:**
    `EEPP_RUNTIME=headless bin/unit_tests/eepp-unit_tests`
    Use this when no usable desktop display is available, when requested, or for headless runtime
    validation. It renders offscreen without Xvfb. It can use a software renderer, so its timings
    do not necessarily represent hardware GPU performance. Add `--filter` for focused headless runs.
*   **Last Fallback When the Headless Runtime Cannot Support the Required Check:**
    Keep `projects/scripts/xvfb-run-eepp` as the fallback when no usable desktop display is available:
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
    Keep the same runtime as the full suite for the current environment: native on a usable desktop,
    headless when no usable display is available, or the Xvfb fallback when headless cannot support
    the required check.

## Cross-Platform Compatibility (Required)

All general unit tests must work on every supported platform, including at least the desktop
platforms: Windows, Linux, and macOS. Consider portability when implementing a test, rather than
relying on success on the development host alone.

* Use platform-aware filesystem helpers for fixture paths and expected filesystem paths. Do not
  assume `/` is the native path separator or that every absolute path starts with `/`.
* Keep filesystem paths distinct from URIs. Construct explicit file URIs for navigation; a Windows
  drive letter in a bare filesystem path can be interpreted as a URI scheme.
* Avoid unguarded assumptions about shell commands, environment variables, keyboard modifiers,
  fonts, or operating-system behavior. Use the project's portable APIs where available.
* Tests for intentionally platform-specific behavior are allowed, but must be explicitly guarded
  for the relevant platforms. Do not restrict a general test to one platform to avoid fixing its
  portability.
* When changing platform-sensitive test code, validate it on the affected platforms where possible.
  Use `projects/mingw32/make.sh` to build Windows tests from Linux and run them under Wine.
  State any platforms that remain unverified.

## Writing New Tests
Writing new tests is highly encouraged, but depends on the context of your changes:
*   **Core Framework (`eepp`):** If you add new logic, math, or framework-level features, you are **expected** to write unit tests for them.
*   **Application/Tools (`ecode`):** Application-level UI changes or tool integrations are often difficult to mock/test. Tests for these are **optional** and should only be added if practical to set up.

**Testing Workflow:**
1.  All tests are located in `src/tests/unit_tests/`.
2.  Before modifying code, run the existing tests most relevant to your change to ensure a baseline.
3.  For reference on how tests are structured in this project, review `src/tests/unit_tests/fontrendering.cpp` (the most complete set of text rendering tests).
