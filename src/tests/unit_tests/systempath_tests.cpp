#include "utest.hpp"

#include <cstdlib>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <optional>

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

UTEST( SystemPath, dataPathsFollowXDG ) {
	auto save = []( const char* name ) -> std::optional<std::string> {
		const char* value = std::getenv( name );
		return value ? std::optional<std::string>( value ) : std::nullopt;
	};
	auto restore = []( const char* name, const std::optional<std::string>& value ) {
		if ( value )
			::setenv( name, value->c_str(), 1 );
		else
			::unsetenv( name );
	};
	auto savedData = save( "XDG_DATA_HOME" );
	auto savedDirs = save( "XDG_DATA_DIRS" );
	auto savedHome = save( "HOME" );

	::setenv( "HOME", "/home/someone", 1 );
	::setenv( "XDG_DATA_HOME", "/xdg/data/", 1 );
	EXPECT_STDSTREQ( "/xdg/data", Sys::getDataPath() );
	EXPECT_STDSTREQ( "/xdg/data/app", Sys::getDataPath( "app" ) );
	// The specification ignores relative and empty values.
	::setenv( "XDG_DATA_HOME", "relative", 1 );
	EXPECT_STDSTREQ( "/home/someone/.local/share", Sys::getDataPath() );
	::unsetenv( "XDG_DATA_HOME" );
	EXPECT_STDSTREQ( "/home/someone/.local/share/Trash", Sys::getDataPath( "Trash" ) );

	::setenv( "XDG_DATA_DIRS", "/opt/share/:relative::/usr/share", 1 );
	auto paths = Sys::getDataSearchPaths();
	ASSERT_EQ( 3u, paths.size() );
	EXPECT_STDSTREQ( "/home/someone/.local/share", paths[0] );
	EXPECT_STDSTREQ( "/opt/share", paths[1] );
	EXPECT_STDSTREQ( "/usr/share", paths[2] );
	::unsetenv( "XDG_DATA_DIRS" );
	paths = Sys::getDataSearchPaths();
	ASSERT_EQ( 3u, paths.size() );
	EXPECT_STDSTREQ( "/usr/local/share", paths[1] );

	restore( "XDG_DATA_HOME", savedData );
	restore( "XDG_DATA_DIRS", savedDirs );
	restore( "HOME", savedHome );
}

#endif

UTEST( SystemPath, fileNameSplitExtension ) {
	auto split = []( const std::string& name ) {
		auto parts = FileSystem::fileNameSplitExtension( name );
		return parts.first + "|" + parts.second;
	};
	EXPECT_STDSTREQ( "a.tar|.gz", split( "a.tar.gz" ) );
	EXPECT_STDSTREQ( "notes|.TXT", split( "notes.TXT" ) );
	EXPECT_STDSTREQ( "Makefile|", split( "Makefile" ) );
	EXPECT_STDSTREQ( ".bashrc|", split( ".bashrc" ) );
	EXPECT_STDSTREQ( ".config|.json", split( ".config.json" ) );
	EXPECT_STDSTREQ( "trailing|.", split( "trailing." ) );
}
