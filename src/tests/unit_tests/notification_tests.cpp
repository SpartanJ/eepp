#include "utest.h"
#include <eepp/system/notification.hpp>

using namespace EE::System;

UTEST( Notification, embeddedNulIsRejectedBeforeSubmitting ) {
	std::string error;
	EXPECT_FALSE( Notification::show( std::string( "before\0after", 12 ), "message", &error ) );
	EXPECT_FALSE( error.empty() );
	EXPECT_FALSE( Notification::show( "title", std::string( "before\0after", 12 ), &error ) );
	EXPECT_FALSE( error.empty() );
	// Errors are optional; validation still runs without an output parameter.
	EXPECT_FALSE( Notification::show( std::string( 1, '\0' ), "message" ) );
}

#if EE_PLATFORM == EE_PLATFORM_MACOS
UTEST( Notification, standaloneExecutableNeedsAnAppBundle ) {
	if ( Notification::isSupported() )
		UTEST_SKIP( "This test checks the standalone executable path" );
	std::string error;
	EXPECT_FALSE( Notification::show( "Test", "Must not request permission without an app identity",
									  &error ) );
	EXPECT_TRUE( error.find( ".app" ) != std::string::npos );
}
#endif
