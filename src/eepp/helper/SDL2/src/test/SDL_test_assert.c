/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

/*

 Used by the test framework and test cases. 

*/

#include "SDL_config.h"

#include "SDL_test.h"

/* Assert check message format */
const char *SDLTest_AssertCheckFmt = "Assert '%s': %s";

/* Assert summary message format */
const char *SDLTest_AssertSummaryFmt = "Assert Summary: Total=%d Passed=%d Failed=%d";

/*! \brief counts the failed asserts */
static Uint32 SDLTest_AssertsFailed = 0;

/*! \brief counts the passed asserts */
static Uint32 SDLTest_AssertsPassed = 0;

/*
 *  Assert that logs and break execution flow on failures (i.e. for harness errors).
 */
void SDLTest_Assert(int assertCondition, char *assertDescription)
{
	SDL_assert((SDLTest_AssertCheck(assertCondition, assertDescription)));
}

/*
 * Assert that logs but does not break execution flow on failures (i.e. for test cases).
 */
int SDLTest_AssertCheck(int assertCondition, char *assertDescription)
{
	char *fmt = (char *)SDLTest_AssertCheckFmt;
	if (assertCondition == ASSERT_FAIL)
	{
		SDLTest_AssertsFailed++;
		SDLTest_LogError(fmt, assertDescription, "Failed");
	} 
	else 
	{
		SDLTest_AssertsPassed++;
		SDLTest_Log(fmt, assertDescription, "Passed");
	}

	return assertCondition;
}

/*
 * Resets the assert summary counters to zero.
 */
void SDLTest_ResetAssertSummary()
{
	SDLTest_AssertsPassed = 0;
	SDLTest_AssertsFailed = 0;
}

/*
 * Logs summary of all assertions (total, pass, fail) since last reset 
 * as INFO (failed==0) or ERROR (failed > 0).
 */
void SDLTest_LogAssertSummary()
{
	char *fmt = (char *)SDLTest_AssertSummaryFmt;
	Uint32 totalAsserts = SDLTest_AssertsPassed + SDLTest_AssertsFailed;
	if (SDLTest_AssertsFailed == 0)
	{
		SDLTest_Log(fmt, totalAsserts, SDLTest_AssertsPassed, SDLTest_AssertsFailed);
	} 
	else 
	{
		SDLTest_LogError(fmt, totalAsserts, SDLTest_AssertsPassed, SDLTest_AssertsFailed);
	}
}

/*
 * Converts the current assert state into a test result
 */
int SDLTest_AssertSummaryToTestResult()
{
	if (SDLTest_AssertsFailed > 0) {
		return TEST_RESULT_FAILED;
	} else {
		if (SDLTest_AssertsPassed > 0) {
			return TEST_RESULT_PASSED;
		} else {
			return TEST_RESULT_NO_ASSERT;
		}
	}
}
