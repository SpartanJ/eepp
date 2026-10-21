#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <atomic>
#include <eepp/system/zip.hpp>

#if EE_PLATFORM != EE_PLATFORM_WIN
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace EE;
using namespace EE::VFS;
using EE::System::Zip;
using namespace vfstest;

UTEST( VFSDirectoryStats, IncludesHiddenEntriesAndExcludesDirectoryLogicalSizes ) {
	VirtualFileSystem vfs;
	auto memory = std::make_shared<MemoryProvider>();
	vfs.registerProvider( memory );
	const URI root( "mem://stats/root" );
	memory->putFile( root.withAppendedSegment( "visible" ), "abc" );
	memory->putFile( root.withAppendedSegment( ".hidden" ), "12345" );
	memory->putFile( root.withAppendedSegment( "nested/file" ), "test" );
	memory->putDirectory( root.withAppendedSegment( "empty" ) );
	auto result = waitFor( vfs.directoryStats( root ) );
	ASSERT_TRUE( result );
	EXPECT_EQ( 12u, result.value().size );
	EXPECT_EQ( 3u, result.value().files );
	EXPECT_EQ( 2u, result.value().directories );
	EXPECT_FALSE( result.value().allocatedSize.has_value() );
	EXPECT_FALSE( result.value().incomplete() );
}

UTEST( VFSDirectoryStats, OverlappingSelectionsDoNotMeasureSubtreesTwice ) {
	VirtualFileSystem vfs;
	auto memory = std::make_shared<MemoryProvider>();
	vfs.registerProvider( memory );
	const URI root( "mem://stats/root" );
	memory->putFile( root.withAppendedSegment( "sub/file" ), "abc" );
	auto result = waitFor( vfs.directoryStats( std::vector<URI>{
		root.withAppendedSegment( "sub/file" ), root.withAppendedSegment( "sub" ), root, root } ) );
	ASSERT_TRUE( result );
	EXPECT_EQ( 3u, result.value().size );
	EXPECT_EQ( 1u, result.value().files );
	EXPECT_EQ( 1u, result.value().directories );
	const URI sibling( "mem://stats/root-sibling" );
	memory->putFile( sibling, "12345" );
	result = waitFor( vfs.directoryStats(
		std::vector<URI>{ root.withAppendedSegment( "sub" ), root, sibling } ) );
	ASSERT_TRUE( result );
	EXPECT_EQ( 8u, result.value().size );
	EXPECT_EQ( 2u, result.value().files );
	result = waitFor( vfs.directoryStats(
		std::vector<URI>{ URI( "mem://stats/root/" ), root.withAppendedSegment( "sub/file" ) } ) );
	ASSERT_TRUE( result );
	EXPECT_EQ( 3u, result.value().size );
	EXPECT_EQ( 1u, result.value().files );
	EXPECT_EQ( 1u, result.value().directories );
}

UTEST( VFSDirectoryStats, KnownZeroAllocationStaysDistinctFromUnavailable ) {
	class SparseProvider : public MemoryProvider {
	  public:
		VFSTask<VFSFileInfo> stat( const URI& uri, const StatOptions& options,
								   CancellationToken token ) override {
			return MemoryProvider::stat( uri, options, std::move( token ) )
				.then( []( VFSResult<VFSFileInfo>&& result ) {
					if ( result )
						result.value().setAllocatedSize( 0 );
					return std::move( result );
				} );
		}
	};
	VirtualFileSystem vfs;
	auto memory = std::make_shared<SparseProvider>();
	vfs.registerProvider( memory );
	const URI file( "mem://stats/file" );
	memory->putFile( file, "abc" );
	auto result = waitFor( vfs.directoryStats( file ) );
	ASSERT_TRUE( result );
	EXPECT_EQ( 3u, result.value().size );
	ASSERT_TRUE( result.value().allocatedSize );
	EXPECT_EQ( 0u, *result.value().allocatedSize );
	EXPECT_FALSE( result.value().incomplete() );
}

UTEST( VFSDirectoryStats, MissingLogicalSizeProducesAnIncompleteSubtotal ) {
	class UnknownSizeProvider : public MemoryProvider {
	  public:
		VFSTask<VFSFileInfo> stat( const URI& uri, const StatOptions&,
								   CancellationToken ) override {
			VFSFileInfo info;
			info.name = uri.getLastPathSegment();
			info.type = FileType::Regular;
			return readyValue<VFSFileInfo>( std::move( info ) );
		}
	};
	VirtualFileSystem vfs;
	vfs.registerProvider( std::make_shared<UnknownSizeProvider>() );
	auto result = waitFor( vfs.directoryStats( URI( "mem://stats/file" ) ) );
	ASSERT_TRUE( result );
	EXPECT_EQ( 1u, result.value().unknownSizes );
	EXPECT_EQ( 1u, result.value().files );
	EXPECT_TRUE( result.value().incomplete() );
}

UTEST( VFSDirectoryStats, ArchiveMeasurementReportsLogicalTotalsWithoutInventingAllocation ) {
	TempDir temp;
	const std::string path = temp.path + "sizes.zip";
	Zip archive( false );
	ASSERT_TRUE( archive.create( path ) );
	ASSERT_TRUE( archive.addFile( reinterpret_cast<const Uint8*>( "abc" ), 3, "sub/file" ) );
	ASSERT_TRUE( archive.addFile( reinterpret_cast<const Uint8*>( "12345" ), 5, ".hidden" ) );
	ASSERT_TRUE( archive.close() );
	VirtualFileSystem vfs;
	auto result = waitFor( vfs.directoryStats( ZipProvider::makeURI( URI::fromFSPath( path ) ) ) );
	ASSERT_TRUE( result );
	EXPECT_EQ( 8u, result.value().size );
	EXPECT_EQ( 2u, result.value().files );
	EXPECT_EQ( 1u, result.value().directories );
	EXPECT_FALSE( result.value().allocatedSize );
	EXPECT_FALSE( result.value().incomplete() );
}

UTEST( VFSDirectoryStats, UnreadableDescendantsLeaveExplicitlyIncompleteSubtotals ) {
	class RestrictedProvider : public MemoryProvider {
	  public:
		VFSTask<DirectoryHandle> openDirectory( const URI& uri, const DirectoryOpenOptions& options,
												CancellationToken token ) override {
			if ( uri.getLastPathSegment() == "unreadable" )
				return readyError<DirectoryHandle>( VFSErrorCode::PermissionDenied, uri );
			return MemoryProvider::openDirectory( uri, options, std::move( token ) );
		}
	};
	VirtualFileSystem vfs;
	auto memory = std::make_shared<RestrictedProvider>();
	vfs.registerProvider( memory );
	const URI root( "mem://stats/root" );
	memory->putFile( root.withAppendedSegment( "file" ), "abc" );
	memory->putFile( root.withAppendedSegment( "unreadable/file" ), "not measured" );
	auto result = waitFor( vfs.directoryStats( root ) );
	ASSERT_TRUE( result );
	EXPECT_EQ( 3u, result.value().size );
	EXPECT_EQ( 1u, result.value().files );
	EXPECT_EQ( 1u, result.value().directories );
	EXPECT_EQ( 1u, result.value().unreadableEntries );
	EXPECT_TRUE( result.value().incomplete() );
	ASSERT_TRUE( result.value().firstError );
	EXPECT_EQ( VFSErrorCode::PermissionDenied, result.value().firstError->code );
	EXPECT_EQ(
		VFSErrorCode::PermissionDenied,
		codeOf( waitFor( vfs.directoryStats( root.withAppendedSegment( "unreadable" ) ) ) ) );
}

UTEST( VFSDirectoryStats, CancellationPublishesAFinalSubtotalAndFailsTheTask ) {
	VirtualFileSystem vfs;
	auto memory = std::make_shared<MemoryProvider>();
	vfs.registerProvider( memory );
	const URI root( "mem://stats/root" );
	memory->putFile( root.withAppendedSegment( "file" ), "abc" );
	CancellationSource cancel;
	cancel.cancel();
	std::atomic<Uint64> callbacks{ 0 };
	auto result = waitFor( vfs.directoryStats(
		root,
		[&]( const DirectoryStats& stats ) {
			++callbacks;
			EXPECT_EQ( 0u, stats.size );
		},
		cancel.token() ) );
	EXPECT_EQ( VFSErrorCode::Cancelled, codeOf( result ) );
	EXPECT_EQ( 1u, callbacks.load() );
}

UTEST( VFSDirectoryStats, CancellationDuringEnumerationKeepsTheLastMeasuredSubtotal ) {
	class CancellingDirectory : public DirectoryHandleImpl {
	  public:
		CancellingDirectory( DirectoryHandle handle, CancellationSource& cancel ) :
			mHandle( std::move( handle ) ), mCancel( cancel ) {}

		VFSTask<DirectoryBatch> read( size_t count, CancellationToken token ) override {
			if ( mReads++ == 1 )
				mCancel.cancel();
			return mHandle.read( count, std::move( token ) );
		}

		VFSTask<void> close() override { return mHandle.close(); }

	  private:
		DirectoryHandle mHandle;
		CancellationSource& mCancel;
		int mReads{ 0 };
	};
	class CancellingProvider : public MemoryProvider {
	  public:
		CancellationSource cancel;

		VFSTask<DirectoryHandle> openDirectory( const URI& uri, const DirectoryOpenOptions& options,
												CancellationToken token ) override {
			return MemoryProvider::openDirectory( uri, options, std::move( token ) )
				.then( [this,
						uri]( VFSResult<DirectoryHandle>&& result ) -> VFSResult<DirectoryHandle> {
					if ( !result )
						return Err( std::move( result ).error() );
					return Ok( DirectoryHandle( uri, std::make_shared<CancellingDirectory>(
														 std::move( result ).value(), cancel ) ) );
				} );
		}
	};
	VirtualFileSystem vfs;
	auto memory = std::make_shared<CancellingProvider>();
	vfs.registerProvider( memory );
	const URI root( "mem://stats/root" );
	for ( int i = 0; i < 300; ++i )
		memory->putFile( root.withAppendedSegment( std::to_string( i ) ), "x" );
	DirectoryStats last;
	auto result = waitFor( vfs.directoryStats(
		root, [&]( const DirectoryStats& progress ) { last = progress; },
		memory->cancel.token() ) );
	EXPECT_EQ( VFSErrorCode::Cancelled, codeOf( result ) );
	EXPECT_EQ( 256u, last.files );
	EXPECT_EQ( 256u, last.size );
}

#if EE_PLATFORM != EE_PLATFORM_WIN

UTEST( VFSDirectoryStats, ExplicitRootsThroughNestedSymlinksAreNotCoveredByParent ) {
	TempDir temp;
	VirtualFileSystem vfs;
	const auto parent = temp.uri().withAppendedSegment( "parent" );
	const auto outside = temp.uri().withAppendedSegment( "outside" );
	ASSERT_TRUE( waitFor( vfs.makeDirs( parent ) ) );
	ASSERT_TRUE( waitFor( vfs.makeDirs( outside.withAppendedSegment( "sub" ) ) ) );
	ASSERT_TRUE( waitFor( vfs.writeAll( outside.withAppendedSegment( "sub/file" ), "12345" ) ) );
	const auto link = parent.withAppendedSegment( "link" );
	ASSERT_EQ( 0, ::symlink( outside.getFSPath().c_str(), link.getFSPath().c_str() ) );
	for ( const auto& selected :
		  { link.withAppendedSegment( "sub/file" ), link.withAppendedSegment( "sub" ) } ) {
		auto result = waitFor( vfs.directoryStats( std::vector<URI>{ selected, parent } ) );
		ASSERT_TRUE( result );
		EXPECT_EQ( 5u, result.value().size );
		EXPECT_EQ( 2u, result.value().files ); // The symlink itself and the selected file.
		EXPECT_EQ( 0u, result.value().directories );
		EXPECT_FALSE( result.value().incomplete() );
	}
}

UTEST( VFSDirectoryStats, NativeAllocationHandlesSparseFilesHardLinksAndSymlinks ) {
	TempDir temp;
	VirtualFileSystem vfs;
	const URI root = temp.uri().withAppendedSegment( "root" );
	const URI nested = root.withAppendedSegment( "nested" );
	const URI file = root.withAppendedSegment( "file" );
	const URI sparse = root.withAppendedSegment( "sparse" );
	ASSERT_TRUE( waitFor( vfs.makeDirs( nested ) ) );
	ASSERT_TRUE( waitFor( vfs.writeAll( file, "abc" ) ) );
	ASSERT_EQ( 0, ::link( file.getFSPath().c_str(),
						  root.withAppendedSegment( "hardlink" ).getFSPath().c_str() ) );
	ASSERT_EQ( 0, ::symlink( "file", root.withAppendedSegment( "link" ).getFSPath().c_str() ) );
	ASSERT_EQ( 0, ::symlink( "..", root.withAppendedSegment( "cycle" ).getFSPath().c_str() ) );
	const int fd = ::open( sparse.getFSPath().c_str(), O_CREAT | O_WRONLY | O_EXCL, 0600 );
	ASSERT_GE( fd, 0 );
	const int truncated = ::ftruncate( fd, 1024 * 1024 );
	::close( fd );
	ASSERT_EQ( 0, truncated );
	Uint64 allocated = 0;
	for ( const auto& uri : { root, nested, file, sparse } ) {
		struct stat native;
		ASSERT_EQ( 0, ::lstat( uri.getFSPath().c_str(), &native ) );
		allocated += static_cast<Uint64>( native.st_blocks ) * 512;
	}
	auto result = waitFor( vfs.directoryStats( root ) );
	ASSERT_TRUE( result );
	EXPECT_EQ( 1024u * 1024u + 3u, result.value().size );
	ASSERT_TRUE( result.value().allocatedSize );
	EXPECT_EQ( allocated, *result.value().allocatedSize );
	EXPECT_EQ( 5u, result.value().files );
	EXPECT_EQ( 1u, result.value().directories );
	EXPECT_FALSE( result.value().incomplete() );
	auto link = waitFor( vfs.directoryStats( root.withAppendedSegment( "cycle" ) ) );
	ASSERT_TRUE( link );
	EXPECT_EQ( 0u, link.value().size );
	EXPECT_EQ( 1u, link.value().files );
	EXPECT_EQ( 0u, link.value().directories );
	auto selection = waitFor( vfs.directoryStats( std::vector<URI>{
		root.withAppendedSegment( "cycle" ), root.withAppendedSegment( "cycle/root/file" ) } ) );
	ASSERT_TRUE( selection );
	EXPECT_EQ( 3u, selection.value().size );
	EXPECT_EQ( 2u, selection.value().files );
}

#endif
