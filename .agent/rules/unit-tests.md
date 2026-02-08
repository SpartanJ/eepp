---
trigger: always_on
---

Project provide a good range of unit-tests that they must pass to guarantee that changes made do not break functionality.
To run the tests you must execute the binary:
`bin/unit_tests/eepp-unit_tests-debug`

This path is from the root directory, you can run it from anywhere, current working directory is managed by the binary.

If you need to run an specific test you can use the filter parameter, it supports glob patterns, for example:

`bin/unit_tests/eepp-unit_tests-debug --filter="FontRendering.*Offset*"`

Will run all tests with "Offset" in its name.
It's expected that for *any* requested new functionality you must add new tests and also tests with previously existing ones. Initially always test with the most relevant to the change that's has been made.

Tests can be found at: `src/tests/unit_tests`. Being `src/tests/unit_tests/fontrendering.cpp` the most complete set of tests related to text rendering.
