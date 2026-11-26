#include "utest.hpp"
#include <eepp/system/filelock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>

using namespace EE::System;

UTEST( FileLock, reportsInvalidOrInaccessiblePathsAsErrors ) {
	const auto directory = FileSystem::createTempDirectory( "eepp-file-lock-" );
	ASSERT_FALSE( directory.empty() );
	ScopedOp cleanup( nullptr, [&directory] { FileSystem::dirRemoveAll( directory ); } );
	const auto slash = FileSystem::getOSSlash();
	const auto path = directory + slash + "lock";
	FileLock lock;
	EXPECT_TRUE( lock.tryLock( "" ) == FileLock::Result::Error );
	EXPECT_TRUE( lock.tryLock( path + '\0' + "suffix" ) == FileLock::Result::Error );
	EXPECT_FALSE( FileSystem::fileExists( path ) );
	EXPECT_TRUE( lock.tryLock( directory + slash + "missing" + slash + "lock" ) ==
				 FileLock::Result::Error );
}

#if EE_PLATFORM == EE_PLATFORM_WIN ||                                          \
	( defined( EE_PLATFORM_POSIX ) && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN && \
	  EE_PLATFORM != EE_PLATFORM_SOLARIS )
UTEST( FileLock, reportsContentionAndReleasesOwnershipOnDestruction ) {
	const auto directory = FileSystem::createTempDirectory( "eepp-file-lock-" );
	ASSERT_FALSE( directory.empty() );
	ScopedOp cleanup( nullptr, [&directory] { FileSystem::dirRemoveAll( directory ); } );
	const auto path = directory + FileSystem::getOSSlash() + "lock-\xC3\xA9";
	FileLock next;
	{
		FileLock first;
		ASSERT_TRUE( first.tryLock( path ) == FileLock::Result::Acquired );
		EXPECT_TRUE( first.tryLock( path + ".other" ) == FileLock::Result::Error );
		EXPECT_FALSE( FileSystem::fileExists( path + ".other" ) );
		EXPECT_TRUE( next.tryLock( path ) == FileLock::Result::Busy );
	}
	EXPECT_TRUE( FileSystem::fileExists( path ) );
	EXPECT_TRUE( next.tryLock( path ) == FileLock::Result::Acquired );
}
#endif
