#include "utest.hpp"

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
	EXPECT_STDSTREQ( temp.path, FileSystem::getCurrentWorkingDirectory() );
}
