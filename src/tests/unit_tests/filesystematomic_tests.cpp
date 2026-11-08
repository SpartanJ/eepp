#include "utest.hpp"
#include <eepp/system/filesystem.hpp>
#if defined( EE_PLATFORM_POSIX )
#include <sys/stat.h>
#endif
using namespace EE::System;

UTEST( FileSystem, privateTempDirectoryAndAtomicReplacement ) {
	struct Directory {
		std::string path{ FileSystem::createTempDirectory( "eepp-atomic-" ) };
		~Directory() {
			if ( !path.empty() )
				FileSystem::dirRemoveAll( path );
		}
	} directory;
	ASSERT_FALSE( directory.path.empty() );
	EXPECT_TRUE( FileSystem::isDirectory( directory.path ) );
#if defined( EE_PLATFORM_POSIX )
	struct stat info{};
	ASSERT_EQ( stat( directory.path.c_str(), &info ), 0 );
	EXPECT_EQ( info.st_mode & 0777, 0700 );
#endif
	const auto path = directory.path + "/record";
	ASSERT_TRUE( FileSystem::fileWriteAtomic( path, "old" ) );
#if defined( EE_PLATFORM_POSIX )
	ASSERT_EQ( chmod( path.c_str(), 0600 ), 0 );
#endif
	const std::string replacement( "new\0record", 10 );
	ASSERT_TRUE( FileSystem::fileWriteAtomic( path, replacement, true ) );
	std::string data;
	ASSERT_TRUE( FileSystem::fileGet( path, data ) );
	EXPECT_TRUE( data == replacement );
#if defined( EE_PLATFORM_POSIX )
	ASSERT_EQ( stat( path.c_str(), &info ), 0 );
	EXPECT_EQ( info.st_mode & 0777, 0600 );
#endif
	EXPECT_FALSE( FileSystem::fileWriteAtomic( path + "/impossible", "new" ) );
	ASSERT_TRUE( FileSystem::fileGet( path, data ) );
	EXPECT_TRUE( data == replacement );
	ASSERT_TRUE( FileSystem::fileWriteAtomic( path, "" ) );
	EXPECT_EQ( FileSystem::fileSize( path ), 0u );
	EXPECT_TRUE( FileSystem::createTempDirectory( "bad/path" ).empty() );
}

UTEST( FileSystem, absolutePathsAndDriveRelativePaths ) {
	EXPECT_TRUE( FileSystem::isRelativePath( "relative" ) );
	EXPECT_TRUE( FileSystem::isRelativePath( "" ) );
	EXPECT_FALSE( FileSystem::isRelativePath( "/absolute" ) );
#if EE_PLATFORM == EE_PLATFORM_WIN
	EXPECT_FALSE( FileSystem::isRelativePath( "C:\\absolute" ) );
	EXPECT_FALSE( FileSystem::isRelativePath( "\\\\server\\share\\file" ) );
	EXPECT_FALSE( FileSystem::isRelativePath( "//server/share/file" ) );
	EXPECT_FALSE( FileSystem::isRelativePath( "C:relative" ) );
	EXPECT_FALSE( FileSystem::isRelativePath( "C:" ) );
#endif
}
