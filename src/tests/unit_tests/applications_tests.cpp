#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <eepp/system/applications.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/sys.hpp>

using namespace EE;
using namespace EE::System;

UTEST( Applications, desktopExecQuoting ) {
	const auto args =
		Applications::splitExec( R"exec("/path with spaces/editor" "" "a\"b" "c\\d" %F)exec" );
	ASSERT_EQ( size_t( 5 ), args.size() );
	EXPECT_STDSTREQ( "/path with spaces/editor", args[0] );
	EXPECT_TRUE( args[1].empty() );
	EXPECT_STDSTREQ( "a\"b", args[2] );
	EXPECT_STDSTREQ( "c\\d", args[3] );
	EXPECT_STDSTREQ( "%F", args[4] );
}

UTEST( Applications, rejectsEmptyCommandsAndSelections ) {
	EXPECT_TRUE( Applications::splitExec( " \t " ).empty() );
	EXPECT_FALSE( Applications::spawn( {} ) );
	EXPECT_FALSE( Applications::launchCommand( "", {} ) );
	EXPECT_FALSE( Applications::launch( {}, {} ) );
}

#if defined( EE_PLATFORM_POSIX ) && EE_PLATFORM != EE_PLATFORM_MACOS &&        \
	EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN && EE_PLATFORM != EE_PLATFORM_IOS && \
	EE_PLATFORM != EE_PLATFORM_ANDROID
UTEST( Applications, singleFileURIHandlerOpensEveryFile ) {
	vfstest::TempDir root;
	const auto script = root.path + "capture.sh";
	const auto output = root.path + "arguments";
	ASSERT_TRUE( FileSystem::fileWrite( script, "printf '%s\\n' \"$2\" >> \"$1\"\n" ) );
	const std::vector<std::string> files{ root.path + "a b.txt", root.path + "c.txt" };
	Application application{
		"fixture", "Fixture", "/bin/sh \"" + script + "\" \"" + output + "\" %u", {}, false };
	ASSERT_TRUE( Applications::launch( application, files ) );
	const std::string expected = URI::fromFSPath( files[0] ).toString() + "\n" +
								 URI::fromFSPath( files[1] ).toString() + "\n";
	std::string actual;
	Clock clock;
	while ( clock.getElapsedTime() < Seconds( 2 ) ) {
		FileSystem::fileGet( output, actual );
		if ( actual.size() >= expected.size() )
			break;
		Sys::sleep( Milliseconds( 5 ) );
	}
	// Detached processes may finish in either order.
	const auto first = URI::fromFSPath( files[0] ).toString() + "\n";
	const auto second = URI::fromFSPath( files[1] ).toString() + "\n";
	EXPECT_TRUE( actual == first + second || actual == second + first );
}
#endif
