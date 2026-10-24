#include "utest.hpp"
#include "vfs_test_utils.hpp"

#if EE_PLATFORM != EE_PLATFORM_WIN
#include <unistd.h>
#endif

using namespace vfstest;

UTEST( VFSPermissions, MaskedChangesPreserveEachSelectionsOtherBits ) {
	VirtualFileSystem vfs;
	auto memory = std::make_shared<MemoryProvider>();
	vfs.registerProvider( memory );
	const URI a( "mem:///a" ), b( "mem:///b" );
	memory->putFile( a, "a", 0400 );
	memory->putFile( b, "b", 0755 );
	PermissionChanges changes;
	changes.mask = 0022;
	changes.value = 0020;
	ASSERT_TRUE( waitFor( vfs.changePermissions( { a, b }, changes ) ).hasValue() );
	EXPECT_EQ( 0420u, *waitFor( vfs.stat( a ) ).value().permissions() );
	EXPECT_EQ( 0775u, *waitFor( vfs.stat( b ) ).value().permissions() );
}

UTEST( VFSPermissions, RecursiveChangesStreamMultipleBatches ) {
	VirtualFileSystem::Config config;
	// The traversal must not wait on an orchestration task submitted to its own pool.
	config.orchestrationThreads = 1;
	VirtualFileSystem vfs( config );
	auto memory = std::make_shared<MemoryProvider>();
	vfs.registerProvider( memory );
	const URI root( "mem:///tree" );
	memory->putDirectory( root.withAppendedSegment( "nested" ) );
	for ( size_t i = 0; i < 520; ++i )
		memory->putFile( root.withAppendedSegment( std::to_string( i ) ), "data", 0644 );
	const URI nested( "mem:///tree/nested/file" );
	memory->putFile( nested, "data", 0640 );
	PermissionChanges changes;
	changes.mask = 0077;
	changes.recursive = true;
	ASSERT_TRUE( waitFor( vfs.changePermissions( { root }, changes ) ).hasValue() );
	EXPECT_EQ( 0700u, *waitFor( vfs.stat( root ) ).value().permissions() );
	for ( size_t i = 0; i < 520; ++i ) {
		EXPECT_EQ( 0600u, *waitFor( vfs.stat( root.withAppendedSegment( std::to_string( i ) ) ) )
							   .value()
							   .permissions() );
	}
	EXPECT_EQ( 0600u, *waitFor( vfs.stat( nested ) ).value().permissions() );
}

UTEST( VFSPermissions, RecursiveChangesGrantReadAndSearchBeforeOpeningDirectories ) {
	class AccessCheckedProvider : public MemoryProvider {
	  public:
		VFSTask<DirectoryHandle> openDirectory( const URI& uri, const DirectoryOpenOptions& options,
												CancellationToken token ) override {
			return MemoryProvider::stat( uri, StatOptions{ false }, token )
				.then( [this, uri, options,
						token]( VFSResult<VFSFileInfo>&& result ) -> VFSTask<DirectoryHandle> {
					if ( !result )
						return readyError<DirectoryHandle>( std::move( result ).error() );
					if ( ( result.value().permissions().value_or( 0 ) & 0500 ) != 0500 ) {
						return readyError<DirectoryHandle>(
							VFSErrorCode::PermissionDenied, uri,
							"Directory requires owner read/search access" );
					}
					return MemoryProvider::openDirectory( uri, options, token );
				} );
		}
	};
	VirtualFileSystem vfs;
	auto memory = std::make_shared<AccessCheckedProvider>();
	vfs.registerProvider( memory );
	const URI root( "mem:///tree" ), sub( "mem:///tree/sub" ), file( "mem:///tree/sub/file" );
	memory->putFile( file, "data", 0640 );
	ASSERT_TRUE( waitFor( vfs.setPermissions( root, 0200 ) ).hasValue() );
	ASSERT_TRUE( waitFor( vfs.setPermissions( sub, 0000 ) ).hasValue() );
	PermissionChanges changes;
	changes.mask = 07777;
	changes.value = 0755;
	changes.recursive = true;
	ASSERT_TRUE( waitFor( vfs.changePermissions( { root }, changes ) ).hasValue() );
	for ( const auto& uri : { root, sub, file } ) {
		auto info = waitFor( vfs.stat( uri ) );
		ASSERT_TRUE( info.hasValue() );
		EXPECT_EQ( 0755u, *info.value().permissions() );
	}
}

UTEST( VFSPermissions, NonRecursiveChangesOnlyAffectTheSelectedEntry ) {
	VirtualFileSystem vfs;
	auto memory = std::make_shared<MemoryProvider>();
	vfs.registerProvider( memory );
	const URI root( "mem:///tree" ), child( "mem:///tree/file" );
	memory->putFile( child, "data", 0644 );
	PermissionChanges changes;
	changes.mask = 07777;
	changes.value = 0700;
	ASSERT_TRUE( waitFor( vfs.changePermissions( { root }, changes ) ).hasValue() );
	EXPECT_EQ( 0700u, *waitFor( vfs.stat( root ) ).value().permissions() );
	EXPECT_EQ( 0644u, *waitFor( vfs.stat( child ) ).value().permissions() );
}

UTEST( VFSPermissions, CancellationStopsBeforeChangingTheNextEntry ) {
	class CancellingProvider : public MemoryProvider {
	  public:
		CancellationSource cancel;

		VFSTask<void> setPermissions( const URI& uri, Uint32 mode,
									  CancellationToken token ) override {
			auto result = MemoryProvider::setPermissions( uri, mode, std::move( token ) );
			cancel.cancel();
			return result;
		}
	};
	VirtualFileSystem vfs;
	auto memory = std::make_shared<CancellingProvider>();
	vfs.registerProvider( memory );
	const URI a( "mem:///a" ), b( "mem:///b" );
	memory->putFile( a, "a", 0644 );
	memory->putFile( b, "b", 0644 );
	PermissionChanges changes;
	changes.mask = 07777;
	changes.value = 0600;
	auto result = waitFor( vfs.changePermissions( { a, b }, changes, memory->cancel.token() ) );
	ASSERT_FALSE( result.hasValue() );
	EXPECT_TRUE( result.error().is( VFSErrorCode::Cancelled ) );
	EXPECT_EQ( 0600u, *waitFor( vfs.stat( a ) ).value().permissions() );
	EXPECT_EQ( 0644u, *waitFor( vfs.stat( b ) ).value().permissions() );
}

#if EE_PLATFORM != EE_PLATFORM_WIN
UTEST( VFSPermissions, RecursiveChangesUnlockNativeDirectories ) {
	TempDir temp;
	VirtualFileSystem vfs;
	const URI root = temp.uri().withAppendedSegment( "tree" );
	const URI sub = root.withAppendedSegment( "sub" );
	const URI file = sub.withAppendedSegment( "file" );
	ASSERT_TRUE( waitFor( vfs.makeDirs( sub ) ).hasValue() );
	ASSERT_TRUE( waitFor( vfs.writeAll( file, "data" ) ).hasValue() );
	ASSERT_TRUE( waitFor( vfs.setPermissions( sub, 0000 ) ).hasValue() );
	ASSERT_TRUE( waitFor( vfs.setPermissions( root, 0000 ) ).hasValue() );
	PermissionChanges changes;
	changes.mask = 07777;
	changes.value = 0755;
	changes.recursive = true;
	const auto result = waitFor( vfs.changePermissions( { root }, changes ) );
	const auto rootInfo = waitFor( vfs.stat( root ) );
	// Restore access before assertions, including on failure, so the fixture stays removable.
	EXPECT_TRUE( waitFor( vfs.setPermissions( root, 0700 ) ).hasValue() );
	const auto subInfo = waitFor( vfs.stat( sub ) );
	EXPECT_TRUE( waitFor( vfs.setPermissions( sub, 0700 ) ).hasValue() );
	ASSERT_TRUE( result.hasValue() );
	ASSERT_TRUE( rootInfo.hasValue() );
	ASSERT_TRUE( subInfo.hasValue() );
	EXPECT_EQ( 0755u, *rootInfo.value().permissions() );
	EXPECT_EQ( 0755u, *subInfo.value().permissions() );
	const auto fileInfo = waitFor( vfs.stat( file ) );
	ASSERT_TRUE( fileInfo.hasValue() );
	EXPECT_EQ( 0755u, *fileInfo.value().permissions() );
}

UTEST( VFSPermissions, RecursiveChangesSkipLinksAndApplyDirectoriesLast ) {
	TempDir temp;
	VirtualFileSystem vfs;
	const URI root = temp.uri().withAppendedSegment( "tree" );
	const URI sub = root.withAppendedSegment( "sub" );
	const URI file = sub.withAppendedSegment( "file" );
	const URI outside = temp.uri().withAppendedSegment( "outside" );
	ASSERT_TRUE( waitFor( vfs.makeDirs( sub ) ).hasValue() );
	ASSERT_TRUE( waitFor( vfs.writeAll( file, "data" ) ).hasValue() );
	ASSERT_TRUE( waitFor( vfs.writeAll( outside, "keep" ) ).hasValue() );
	ASSERT_TRUE( waitFor( vfs.setPermissions( outside, 0644 ) ).hasValue() );
	ASSERT_EQ( 0, ::symlink( outside.getFSPath().c_str(),
							 root.withAppendedSegment( "link" ).getFSPath().c_str() ) );
	PermissionChanges changes;
	changes.mask = 07777;
	changes.recursive = true;
	const auto result = waitFor( vfs.changePermissions( { root }, changes ) );
	// Restore access even if the assertion below fails, so the fixture remains removable.
	EXPECT_TRUE( waitFor( vfs.setPermissions( root, 0700 ) ).hasValue() );
	EXPECT_EQ( 0u, *waitFor( vfs.stat( sub ) ).value().permissions() );
	EXPECT_TRUE( waitFor( vfs.setPermissions( sub, 0700 ) ).hasValue() );
	ASSERT_TRUE( result.hasValue() );
	EXPECT_EQ( 0u, *waitFor( vfs.stat( file ) ).value().permissions() );
	EXPECT_EQ( 0644u, *waitFor( vfs.stat( outside ) ).value().permissions() );
}
#endif
