#include "utest.hpp"

#include <cstdlib>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>

using namespace EE;
using namespace EE::System;

namespace {

struct TemporaryWorkingDirectory {
	TemporaryWorkingDirectory() : originalPath( FileSystem::getCurrentWorkingDirectory() ) {
		path = Sys::getTempPath();
		FileSystem::dirAddSlashAtEnd( path );
		path += "eepp-path-路径-" + std::to_string( Sys::getProcessID() ) + "-" +
				std::to_string( Sys::getTicks() );
		created = FileSystem::makeDir( path );
		changed = created && FileSystem::changeWorkingDirectory( path );
	}

	~TemporaryWorkingDirectory() {
		FileSystem::changeWorkingDirectory( originalPath );
		if ( created )
			FileSystem::dirRemoveAll( path );
	}

	std::string originalPath;
	std::string path;
	bool created{ false };
	bool changed{ false };
};

} // namespace

UTEST( SystemPath, processPathMatchesExecutablePath ) {
	const std::string executablePath( Sys::getProcessFilePath() );
	ASSERT_FALSE( executablePath.empty() );
	EXPECT_TRUE( FileSystem::fileExists( executablePath ) );
	EXPECT_STDSTREQ( FileSystem::fileRemoveFileName( executablePath ), Sys::getProcessPath() );
}

UTEST( SystemPath, workingDirectoryRoundTripsUnicode ) {
	TemporaryWorkingDirectory temp;
	ASSERT_TRUE( temp.created );
	ASSERT_TRUE( temp.changed );
	EXPECT_STDSTREQ( FileSystem::getRealPath( temp.path ),
					 FileSystem::getCurrentWorkingDirectory() );
}

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD

UTEST( SystemPath, cachePathFollowsXDG ) {
	auto restore = []( const char* name, const char* value ) {
		if ( value )
			::setenv( name, value, 1 );
		else
			::unsetenv( name );
	};
	const char* xdg = std::getenv( "XDG_CACHE_HOME" );
	std::string savedXdg( xdg ? xdg : "" );
	const char* home = std::getenv( "HOME" );
	std::string savedHome( home ? home : "" );

	::setenv( "XDG_CACHE_HOME", "/xdg/cache", 1 );
	::setenv( "HOME", "/home/someone", 1 );
	EXPECT_STDSTREQ( "/xdg/cache/app", Sys::getCachePath( "app" ) );
	// The specification ignores relative and empty values.
	::setenv( "XDG_CACHE_HOME", "relative", 1 );
	EXPECT_STDSTREQ( "/home/someone/.cache/app", Sys::getCachePath( "app" ) );
	::unsetenv( "XDG_CACHE_HOME" );
	EXPECT_STDSTREQ( "/home/someone/.cache/app", Sys::getCachePath( "app" ) );

	restore( "XDG_CACHE_HOME", xdg ? savedXdg.c_str() : nullptr );
	restore( "HOME", home ? savedHome.c_str() : nullptr );
}

#endif
