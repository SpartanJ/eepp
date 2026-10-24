#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <eepp/vfs/providers/archiveprovider.hpp>
#include <eepp/vfs/providers/zipprovider.hpp>

using namespace vfstest;

UTEST( VFSLocation, RenameRewritesCurrentAndDescendantLocations ) {
	VirtualFileSystem vfs;
	TempDir temp;
	const URI from = temp.uri().withAppendedSegment( "original %20!" );
	const URI to = temp.uri().withAppendedSegment( "renamed #?%" );
	URI current = from;
	ASSERT_TRUE( vfs.relocateURI( current, from, to ) );
	EXPECT_TRUE( current == to );
	URI nested = from.withAppendedSegment( "child %2F#?.txt" );
	ASSERT_TRUE( vfs.relocateURI( nested, from, to ) );
	EXPECT_TRUE( nested == to.withAppendedSegment( "child %2F#?.txt" ) );
	URI fromSlash = from;
	fromSlash.setRawPath( from.getPath() + "/" );
	URI toSlash = to;
	toSlash.setRawPath( to.getPath() + "/" );
	current = fromSlash;
	ASSERT_TRUE( vfs.relocateURI( current, fromSlash, toSlash ) );
	EXPECT_TRUE( current == toSlash );
	nested = from.withAppendedSegment( "child" );
	ASSERT_TRUE( vfs.relocateURI( nested, fromSlash, toSlash ) );
	EXPECT_TRUE( nested == to.withAppendedSegment( "child" ) );
}

UTEST( VFSLocation, RenamePreservesOtherAuthoritiesAndSiblingPrefixes ) {
	VirtualFileSystem vfs;
	vfs.registerProvider( std::make_shared<MemoryProvider>( "sftp" ) );
	const URI from( "sftp://alice@example.test:2222/original" );
	const URI to( "sftp://alice@example.test:2222/renamed" );
	for ( const auto* value : { "sftp://alice@example.test:2222/original-backup/child",
								"sftp://bob@example.test:2222/original/child",
								"sftp://alice@other.test:2222/original/child",
								"sftp://alice@example.test:22/original/child" } ) {
		URI uri( value );
		const URI before = uri;
		EXPECT_FALSE( vfs.relocateURI( uri, from, to ) );
		EXPECT_TRUE( uri == before );
	}
	URI child = from.withAppendedSegment( "nested" );
	ASSERT_TRUE( vfs.relocateURI( child, from, to ) );
	EXPECT_TRUE( child == to.withAppendedSegment( "nested" ) );
}

UTEST( VFSLocation, RenameUpdatesArchiveBackingPathsAndPreservesMembers ) {
	VirtualFileSystem vfs;
	TempDir temp;
	const URI from = temp.uri().withAppendedSegment( "original" );
	const URI to = temp.uri().withAppendedSegment( "renamed" );
	const URI file = from.withAppendedSegment( "bundle !%#.zip" );
	const URI renamed = to.withAppendedSegment( "bundle !%#.zip" );
	for ( const bool zip : { true, false } ) {
		URI uri = zip ? ZipProvider::makeURI( file, "/nested/member !%#.txt" )
					  : ArchiveProvider::makeURI( file, "/nested/member !%#.txt" );
		ASSERT_TRUE( vfs.relocateURI( uri, from, to ) );
		const URI expected = zip ? ZipProvider::makeURI( renamed, "/nested/member !%#.txt" )
								 : ArchiveProvider::makeURI( renamed, "/nested/member !%#.txt" );
		EXPECT_TRUE( uri == expected );
	}
}

UTEST( VFSLocation, RenameDispatchesToRegisteredProviderWithoutIO ) {
	// A compound scheme unknown to both eefm and the VFS facade. Its decoded path is a URI.
	class WrappedProvider : public MemoryProvider {
	  public:
		WrappedProvider() : MemoryProvider( "wrapped" ) {}

		bool relocateURI( URI& uri, const URI& from, const URI& to ) const override {
			URI backing( uri.getPath() );
			if ( !VFSProvider::relocateURI( backing, from, to ) )
				return false;
			uri.setRawPath( backing.toString() );
			return true;
		}
	};
	VirtualFileSystem::Config config;
	config.registerDefaultProviders = false;
	VirtualFileSystem vfs( config );
	auto provider = std::make_shared<WrappedProvider>();
	vfs.registerProvider( provider );
	const URI from( "mem:///original" ), to( "mem:///renamed" );
	URI uri( "wrapped:mem:///original/child" );
	ASSERT_TRUE( vfs.relocateURI( uri, from, to ) );
	EXPECT_STDSTREQ( "wrapped", uri.getScheme() );
	EXPECT_TRUE( URI( uri.getPath() ) == to.withAppendedSegment( "child" ) );
	EXPECT_EQ( 0u, provider->operationCount() );
	vfs.unregisterProvider( "wrapped" );
	const URI before = uri;
	EXPECT_FALSE( vfs.relocateURI( uri, to, from ) );
	EXPECT_TRUE( uri == before );
}

UTEST( VFSLocation, RenameLeavesInvalidAndUnrelatedArchiveLocationsUnchanged ) {
	VirtualFileSystem vfs;
	TempDir temp;
	const URI file = temp.uri().withAppendedSegment( "bundle !%#.zip" );
	const URI renamed = temp.uri().withAppendedSegment( "renamed !%#.zip" );
	for ( const bool zip : { true, false } ) {
		URI uri = zip ? ZipProvider::makeURI( file, "/nested/member !%#.txt" )
					  : ArchiveProvider::makeURI( file, "/nested/member !%#.txt" );
		const URI before = uri;
		EXPECT_FALSE( vfs.relocateURI( uri, temp.uri().withAppendedSegment( "other" ), renamed ) );
		EXPECT_TRUE( uri == before );
		URI otherHost = renamed;
		otherHost.setHost( "other.test" );
		EXPECT_FALSE( vfs.relocateURI( uri, file, otherHost ) );
		EXPECT_TRUE( uri == before );
		ASSERT_TRUE( vfs.relocateURI( uri, file, renamed ) );
		const URI expected = zip ? ZipProvider::makeURI( renamed, "/nested/member !%#.txt" )
								 : ArchiveProvider::makeURI( renamed, "/nested/member !%#.txt" );
		EXPECT_TRUE( uri == expected );
		URI invalid( zip ? "zip:///missing-boundary" : "archive:///missing-boundary" );
		const URI invalidBefore = invalid;
		EXPECT_FALSE(
			vfs.relocateURI( invalid, temp.uri(), temp.uri().withAppendedSegment( "new" ) ) );
		EXPECT_TRUE( invalid == invalidBefore );
	}
}
