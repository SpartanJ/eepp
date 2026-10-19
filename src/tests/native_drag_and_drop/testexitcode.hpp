#ifndef EE_TESTS_NATIVE_DRAG_AND_DROP_TESTEXITCODE_HPP
#define EE_TESTS_NATIVE_DRAG_AND_DROP_TESTEXITCODE_HPP

/** The exit code of automated checks that cannot run here (another display backend, a missing
 * X11 or XTest library), as opposed to a failure. 77 is what Automake and Meson report as
 * skipped. */
constexpr int TestSkipped = 77;

#endif
