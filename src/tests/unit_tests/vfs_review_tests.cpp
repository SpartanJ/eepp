#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <eepp/vfs/path.hpp>
#include <eepp/vfs/watchers/earlywatchevents.hpp>
#include <eepp/vfs/watchers/pollingwatchbackend.hpp>
#include <filesystem>
#include <functional>
#include <utility>

using namespace EE;
using namespace EE::VFS;
using namespace vfstest;

namespace {

/** Controlled mutation of a source after a chunk has been read or after publication. */
class ReadHookHandle : public FileHandleImpl {
  public:
	ReadHookHandle( FileHandle handle, std::function<void()> hook, bool afterClose ) :
		mHandle( std::move( handle ) ), mHook( std::move( hook ) ), mAfterClose( afterClose ) {}

	VFSTask<size_t> read( Uint64 offset, std::span<Uint8> data, CancellationToken token ) override {
		auto read = mHandle.read( offset, data, token );
		if ( mAfterClose || !mHook )
			return read;
		return std::move( read ).then(
			[hook = std::exchange( mHook, {} )]( VFSResult<size_t>&& result ) {
				if ( result && result.value() )
					hook();
				return std::move( result );
			} );
	}

	VFSTask<size_t> write( Uint64 offset, std::span<const Uint8> data,
						   CancellationToken token ) override {
		return mHandle.write( offset, data, token );
	}

	VFSTask<void> truncate( Uint64 size ) override { return mHandle.truncate( size ); }

	VFSTask<void> flush() override { return mHandle.flush(); }

	VFSTask<VFSFileInfo> stat() override { return mHandle.stat(); }

	VFSTask<void> close() override {
		auto closed = mHandle.close();
		if ( !mAfterClose || !mHook )
			return closed;
		return std::move( closed ).then(
			[hook = std::exchange( mHook, {} )]( VFSResult<void>&& result ) {
				hook();
				return std::move( result );
			} );
	}

	ProviderCapabilities capabilities() const override { return mHandle.capabilities(); }

  private:
	FileHandle mHandle;
	std::function<void()> mHook;
	bool mAfterClose;
};

class ChangingSource : public MemoryProvider {
  public:
	enum class When { Scanned, Read, Published, UnknownRevision };

	ChangingSource( URI file, When when, bool replace ) :
		MemoryProvider( "changing" ),
		mFile( std::move( file ) ),
		mWhen( when ),
		mReplace( replace ) {}

	VFSTask<VFSFileInfo> stat( const URI& uri, const StatOptions& options,
							   CancellationToken token ) override {
		return MemoryProvider::stat( uri, options, token )
			.then( [this, uri]( VFSResult<VFSFileInfo>&& result ) {
				if ( result && uri == mFile ) {
					if ( mWhen == When::UnknownRevision )
						result.value().revision = {};
					else if ( mWhen == When::Scanned && !mChanged )
						change();
				}
				return std::move( result );
			} );
	}

	VFSTask<FileHandle> open( const URI& uri, const OpenOptions& options,
							  CancellationToken token ) override {
		return MemoryProvider::open( uri, options, token )
			.then( [this, uri]( VFSResult<FileHandle>&& opened ) -> VFSResult<FileHandle> {
				if ( !opened || uri != mFile || mWhen == When::UnknownRevision ||
					 mWhen == When::Scanned )
					return std::move( opened );
				return Ok( FileHandle( uri, std::make_shared<ReadHookHandle>(
												std::move( opened ).value(), [this] { change(); },
												mWhen == When::Published ) ) );
			} );
	}

	VFSTask<void> remove( const URI& uri, const RemoveOptions& options,
						  CancellationToken token ) override {
		++removals;
		return MemoryProvider::remove( uri, options, token );
	}

	unsigned removals{ 0 };

  private:
	void change() {
		if ( mChanged )
			return;
		mChanged = true;
		if ( mReplace ) {
			putFile( mFile, std::string( 128, 'n' ) );
		} else {
			auto opened = waitFor( MemoryProvider::open( mFile, OpenFlags::Write, {} ) );
			if ( opened ) {
				const Uint8 changed = 'n';
				waitFor( opened.value().write( 0, std::span<const Uint8>( &changed, 1 ) ) );
			}
		}
	}

	URI mFile;
	When mWhen;
	bool mReplace;
	bool mChanged{ false };
};

/** Faults only the identity lookup, while normal existence checks still succeed. */
class StatFaultProvider : public MemoryProvider {
  public:
	VFSErrorCode error{ VFSErrorCode::TimedOut };
	unsigned opens{ 0 };

	VFSTask<VFSFileInfo> stat( const URI& uri, const StatOptions& options,
							   CancellationToken token ) override {
		if ( !options.followSymlinks )
			return readyError<VFSFileInfo>( error, uri, "injected identity lookup failure" );
		return MemoryProvider::stat( uri, options, token );
	}

	VFSTask<FileHandle> open( const URI& uri, const OpenOptions& options,
							  CancellationToken token ) override {
		++opens;
		return MemoryProvider::open( uri, options, token );
	}
};

/** Generates a large listing without creating 100,001 real filesystem objects. */
class GeneratedDirectory : public DirectoryHandleImpl {
  public:
	GeneratedDirectory( size_t count, std::optional<std::string> name ) :
		mName( std::move( name ) ), mCount( count ) {}

	VFSTask<DirectoryBatch> read( size_t maxEntries, CancellationToken ) override {
		DirectoryBatch batch;
		const size_t end = std::min( mCount, mPosition + maxEntries );
		batch.entries.reserve( end - mPosition );
		while ( mPosition < end ) {
			VFSFileInfo info;
			info.name =
				mName ? *mName : "entry" + String::toString( static_cast<Uint64>( mPosition ) );
			info.type = FileType::Regular;
			info.revision = FileRevision::fromValues( { 1 } );
			batch.entries.push_back( std::move( info ) );
			++mPosition;
		}
		batch.endOfDirectory = mPosition == mCount;
		return readyValue<DirectoryBatch>( std::move( batch ) );
	}

	VFSTask<void> close() override { return readyValue<void>(); }

  private:
	std::optional<std::string> mName;
	size_t mCount;
	size_t mPosition{ 0 };
};

class ListingProvider : public MemoryProvider {
  public:
	explicit ListingProvider( size_t entries, std::optional<std::string> name = {} ) :
		MemoryProvider( "listing" ), count( entries ), mName( std::move( name ) ) {
		putDirectory( URI( "listing://test/tree" ) );
		putFile( URI( "listing://test/payload" ), "payload" );
	}

	VFSTask<DirectoryHandle> openDirectory( const URI& uri, const DirectoryOpenOptions&,
											CancellationToken ) override {
		++listings;
		return readyValue<DirectoryHandle>(
			uri, std::make_shared<GeneratedDirectory>( count.load(), mName ) );
	}

	VFSTask<FileHandle> open( const URI&, const OpenOptions& options,
							  CancellationToken token ) override {
		return MemoryProvider::open( URI( "listing://test/payload" ), options, token );
	}

	std::atomic<size_t> count;
	std::atomic<unsigned> listings{ 0 };

  private:
	std::optional<std::string> mName;
};

class RecordingSink : public WatchSink {
  public:
	void push( WatchEvent&& event ) override { events.push_back( std::move( event ) ); }

	void push( std::vector<WatchEvent>&& batch ) override {
		for ( auto& event : batch )
			push( std::move( event ) );
	}

	void rescanRequired() override { push( WatchEvent{ {}, {}, WatchEventType::RescanRequired } ); }

	void setStatus( WatchStatus ) override {}

	const URI& root() const override { return mRoot; }

	std::vector<WatchEvent> events;

  private:
	URI mRoot;
};

} // namespace

UTEST( VFSPath, nulLocalURIsNeverAliasTheNativePrefix ) {
	TempDir temp;
	VirtualFileSystem vfs;
	const URI file = temp.uri().withAppendedSegment( "important.txt" );
	const URI other = temp.uri().withAppendedSegment( "other.txt" );
	ASSERT_TRUE( waitFor( vfs.writeAll( file, "valuable data" ) ) );
	ASSERT_TRUE( waitFor( vfs.writeAll( other, "other data" ) ) );
	const URI nul( file.toString() + "%00-suffix" );
	EXPECT_EQ( VFSErrorCode::InvalidURI, codeOf( LocalProvider::toNativePath( nul ) ) );
	EXPECT_EQ( VFSErrorCode::InvalidURI, codeOf( waitFor( vfs.stat( nul ) ) ) );
	EXPECT_EQ( VFSErrorCode::InvalidURI,
			   codeOf( waitFor( vfs.open( nul, OpenFlags::Write | OpenFlags::Truncate ) ) ) );
	EXPECT_STDSTREQ( "valuable data", readString( vfs, file ) );
	EXPECT_EQ( VFSErrorCode::InvalidURI, codeOf( waitFor( vfs.remove( nul ) ) ) );
	EXPECT_STDSTREQ( "valuable data", readString( vfs, file ) );
	EXPECT_EQ( VFSErrorCode::InvalidURI,
			   codeOf( waitFor( vfs.rename( nul, other, RenameOptions{ true } ) ) ) );
	EXPECT_EQ( VFSErrorCode::InvalidURI,
			   codeOf( waitFor( vfs.rename( other, nul, RenameOptions{ true } ) ) ) );
	EXPECT_STDSTREQ( "valuable data", readString( vfs, file ) );
	EXPECT_STDSTREQ( "other data", readString( vfs, other ) );
	URI host = file;
	host.setHost( std::string( "localhost\0suffix", 16 ) );
	EXPECT_EQ( VFSErrorCode::InvalidURI, codeOf( LocalProvider::toNativePath( host ) ) );
}

UTEST( VFSPath, basenameValidationUsesTheDestinationRules ) {
	const URI local = URI::fromFSPath( System::Sys::getTempPath() );
	const URI remote( "sftp://host/folder" );
	for ( const auto& invalid :
		  { std::string(), std::string( "." ), std::string( ".." ), std::string( "../outside" ),
			std::string( "/absolute" ), std::string( "a/b" ), std::string( "a\0b", 3 ) } ) {
		EXPECT_FALSE( VFSPath::validName( invalid, local ) );
		EXPECT_FALSE( VFSPath::validName( invalid, remote ) );
	}
	EXPECT_TRUE( VFSPath::validName( "日本語 #%.txt", local ) );
	EXPECT_TRUE( VFSPath::validName( "日本語 #%.txt", remote ) );
	for ( const auto name :
		  { "a\\b", "CON", "con.txt", "nul", "aux.zip", "COM1", "LPT9.txt", "COM¹", "LPT².txt",
			"a:b", "name.", "name ", "a?b", "a|b", "a\"b", "a*b", "a<b" } ) {
		EXPECT_TRUE( VFSPath::validName( name, remote ) );
#if EE_PLATFORM == EE_PLATFORM_WIN
		EXPECT_FALSE( VFSPath::validName( name, local ) );
#else
		EXPECT_TRUE( VFSPath::validName( name, local ) );
#endif
	}
	EXPECT_TRUE( VFSPath::validName( "console.txt", local ) );
	EXPECT_TRUE( VFSPath::validName( "com10.txt", local ) );
}

UTEST( VFSSafety, movePreservesSourcesChangedDuringCopy ) {
	for ( const auto when : { ChangingSource::When::Scanned, ChangingSource::When::Read,
							  ChangingSource::When::Published } ) {
		for ( const bool replace : { false, true } ) {
			TempDir temp;
			VirtualFileSystem vfs;
			const URI source( "changing://test/source" );
			auto provider = std::make_shared<ChangingSource>( source, when, replace );
			provider->putFile( source, std::string( 128, 'o' ) );
			vfs.registerProvider( provider );
			CopyOptions options;
			options.chunkSize = 16;
			const URI target = temp.uri().withAppendedSegment( "copy" );
			auto moved = waitFor( vfs.move( source, target, options ).completion() );
			ASSERT_FALSE( moved );
			EXPECT_EQ( VFSErrorCode::ChangedExternally, moved.error().code );
			EXPECT_TRUE( moved.error().uri == source );
			EXPECT_EQ( 0u, provider->removals );
			auto content = provider->fileContent( source );
			ASSERT_TRUE( content );
			EXPECT_EQ( int( 'n' ), int( content->front() ) );
			if ( when == ChangingSource::When::Scanned ) {
				EXPECT_EQ( VFSErrorCode::NotFound, codeOf( waitFor( vfs.stat( target ) ) ) );
			} else {
				EXPECT_STDSTREQ( std::string( 128, 'o' ), readString( vfs, target ) );
			}
		}
	}
}

UTEST( VFSSafety, recursiveMoveRetainsAChangedChildAndItsDirectory ) {
	TempDir temp;
	VirtualFileSystem vfs;
	const URI source( "changing://test/tree/changed" );
	auto provider = std::make_shared<ChangingSource>( source, ChangingSource::When::Read, true );
	provider->putFile( source, std::string( 128, 'o' ) );
	vfs.registerProvider( provider );
	CopyOptions options;
	options.chunkSize = 16;
	const URI target = temp.uri().withAppendedSegment( "tree" );
	auto moved = waitFor( vfs.move( source.getParent(), target, options ).completion() );
	ASSERT_FALSE( moved );
	EXPECT_EQ( VFSErrorCode::ChangedExternally, moved.error().code );
	EXPECT_TRUE( moved.error().uri == source );
	EXPECT_EQ( 0u, provider->removals );
	EXPECT_STDSTREQ( std::string( 128, 'n' ), *provider->fileContent( source ) );
	EXPECT_STDSTREQ( std::string( 128, 'o' ),
					 readString( vfs, target.withAppendedSegment( "changed" ) ) );
	EXPECT_TRUE( waitFor( vfs.stat( source.getParent() ) ).value().isDirectory() );
}

UTEST( VFSSafety, moveWithoutASourceRevisionRequiresAnExplicitCopy ) {
	TempDir temp;
	VirtualFileSystem vfs;
	const URI source( "changing://test/source" );
	auto provider =
		std::make_shared<ChangingSource>( source, ChangingSource::When::UnknownRevision, false );
	provider->putFile( source, "keep" );
	vfs.registerProvider( provider );
	const URI target = temp.uri().withAppendedSegment( "copy" );
	EXPECT_EQ( VFSErrorCode::Unsupported,
			   codeOf( waitFor( vfs.move( source, target ).completion() ) ) );
	EXPECT_EQ( VFSErrorCode::NotFound, codeOf( waitFor( vfs.stat( target ) ) ) );
	EXPECT_STDSTREQ( "keep", *provider->fileContent( source ) );
	EXPECT_EQ( 0u, provider->removals );
	ASSERT_TRUE( waitFor( vfs.copy( source, target ).completion() ) );
	EXPECT_STDSTREQ( "keep", readString( vfs, target ) );
}

UTEST( VFSSafety, identityLookupErrorsStopWritesAndCopies ) {
	VirtualFileSystem vfs;
	auto provider = std::make_shared<StatFaultProvider>();
	vfs.registerProvider( provider );
	const URI file( "mem://test/file" );
	provider->putFile( file, "valuable data" );
	TempDir temp;
	const URI source = temp.uri().withAppendedSegment( "source" );
	ASSERT_TRUE( waitFor( vfs.writeAll( source, "replacement" ) ) );
	for ( const auto code : { VFSErrorCode::PermissionDenied, VFSErrorCode::ConnectionLost,
							  VFSErrorCode::TimedOut } ) {
		provider->error = code;
		EXPECT_EQ( code, codeOf( waitFor( vfs.writeAll( file, "replacement" ) ) ) );
		CopyOptions options;
		options.overwrite = OverwritePolicy::Overwrite;
		EXPECT_EQ( code, codeOf( waitFor( vfs.copy( source, file, options ).completion() ) ) );
		EXPECT_STDSTREQ( "valuable data", *provider->fileContent( file ) );
		EXPECT_EQ( 0u, provider->opens );
	}
}

UTEST( VFSSafety, untrustedDirectoryNamesCannotEscapeTransfersOrRecursiveRemoval ) {
	TempDir temp;
	VirtualFileSystem vfs;
	const URI target = temp.uri().withAppendedSegment( "output" );
	const URI outside = temp.uri().withAppendedSegment( "outside" );
	ASSERT_TRUE( waitFor( vfs.writeAll( outside, "keep" ) ) );
	for ( const auto& name :
		  { std::string( "../outside" ), std::string( "/absolute" ),
			std::string( "nested/../../outside" ), std::string( "entry\0suffix", 12 ),
			std::string(), std::string( "." ), std::string( ".." ) } ) {
		auto provider = std::make_shared<ListingProvider>( 1, name );
		vfs.registerProvider( provider );
		CopyOptions options;
		options.overwrite = OverwritePolicy::Overwrite;
		EXPECT_EQ( VFSErrorCode::ProtocolError,
				   codeOf( waitFor(
					   vfs.copy( URI( "listing://test/tree" ), target, options ).completion() ) ) );
		EXPECT_EQ( VFSErrorCode::NotFound, codeOf( waitFor( vfs.stat( target ) ) ) );
		EXPECT_EQ( VFSErrorCode::ProtocolError,
				   codeOf( waitFor(
					   vfs.remove( URI( "listing://test/tree" ), RemoveOptions{ true } ) ) ) );
		EXPECT_STDSTREQ( "payload", *provider->fileContent( URI( "listing://test/payload" ) ) );
		EXPECT_STDSTREQ( "keep", readString( vfs, outside ) );
	}
	auto valid = std::make_shared<ListingProvider>( 1, "日本語 #%.txt" );
	vfs.registerProvider( valid );
	ASSERT_TRUE( waitFor( vfs.copy( URI( "listing://test/tree" ), target ).completion() ) );
	EXPECT_STDSTREQ( "payload", readString( vfs, target.withAppendedSegment( "日本語 #%.txt" ) ) );
}

UTEST( VFSSafety, directoryAliasesCannotBypassTheSelfCopyGuard ) {
	TempDir temp;
	VirtualFileSystem vfs;
	const URI source = temp.uri().withAppendedSegment( "tree" );
	const URI alias = temp.uri().withAppendedSegment( "alias" );
	ASSERT_TRUE( waitFor( vfs.makeDir( source ) ) );
	ASSERT_TRUE( waitFor( vfs.writeAll( source.withAppendedSegment( "file" ), "keep" ) ) );
	const std::string sourcePath = source.getFSPath(), aliasPath = alias.getFSPath();
	std::error_code error;
	std::filesystem::create_directory_symlink(
		std::filesystem::path( std::u8string( sourcePath.begin(), sourcePath.end() ) ),
		std::filesystem::path( std::u8string( aliasPath.begin(), aliasPath.end() ) ), error );
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( error )
		UTEST_SKIP( "Creating directory symlinks requires Windows symlink privileges" );
#else
	ASSERT_FALSE( error );
#endif
	for ( const URI& destination :
		  { alias.withAppendedSegment( "sub" ),
			alias.withAppendedSegment( "missing" ).withAppendedSegment( "nested" ), alias } ) {
		CopyOptions options;
		options.overwrite = OverwritePolicy::MergeDirectories;
		EXPECT_EQ( VFSErrorCode::InvalidOperation,
				   codeOf( waitFor( vfs.copy( source, destination, options ).completion() ) ) );
		EXPECT_EQ( 1u, waitFor( vfs.listAll( source ) ).value().size() );
		EXPECT_STDSTREQ( "keep", readString( vfs, source.withAppendedSegment( "file" ) ) );
	}
}

UTEST( VFSWatch, earlyNativeOverflowRequiresARescan ) {
	EE::VFS::detail::EarlyWatchEvents pending;
	for ( size_t i = 0; i < EE::VFS::detail::EarlyWatchEvents::MaxEvents + 8; ++i )
		pending.push( "/root/", "entry", efsw::Actions::Add, {} );
	RecordingSink sink;
	unsigned delivered = 0;
	pending.replay( sink, [&]( const auto& ) { ++delivered; } );
	EXPECT_EQ( 0u, delivered );
	ASSERT_EQ( 1u, sink.events.size() );
	EXPECT_EQ( WatchEventType::RescanRequired, sink.events[0].type );
	// An explicit missed-actions callback before registration has the same contract.
	EE::VFS::detail::EarlyWatchEvents missed;
	missed.invalidate();
	missed.push( "/root/", "later", efsw::Actions::Add, {} );
	missed.replay( sink, [&]( const auto& ) { ++delivered; } );
	EXPECT_EQ( 0u, delivered );
	EXPECT_EQ( 2u, sink.events.size() );
	// A new registration starts with a clean buffer and preserves the full bounded stream.
	EE::VFS::detail::EarlyWatchEvents complete;
	for ( size_t i = 0; i < EE::VFS::detail::EarlyWatchEvents::MaxEvents; ++i )
		complete.push( "/root/", "entry", efsw::Actions::Add, {} );
	complete.replay( sink, [&]( const auto& ) { ++delivered; } );
	EXPECT_EQ( EE::VFS::detail::EarlyWatchEvents::MaxEvents, delivered );
	EXPECT_EQ( 2u, sink.events.size() );
}

UTEST( VFSWatch, oversizedInitialPollingSnapshotIsRejected ) {
	VirtualFileSystem vfs;
	auto provider = std::make_shared<ListingProvider>( PollingWatchBackend::MaxEntries + 1 );
	vfs.registerProvider( provider );
	auto watch = waitFor( vfs.watch( URI( "listing://test/tree" ) ) );
	EXPECT_EQ( VFSErrorCode::Unsupported, codeOf( watch ) );
	EXPECT_EQ( 1u, provider->listings.load() );
}

UTEST( VFSWatch, pollingCoverageLossFailsOnceAndStopsPolling ) {
	VirtualFileSystem vfs;
	auto provider = std::make_shared<ListingProvider>( 1 );
	vfs.registerProvider( provider );
	WatchOptions options;
	options.pollingInterval = System::Milliseconds( 50 );
	auto opened = waitFor( vfs.watch( URI( "listing://test/tree" ), options ) );
	ASSERT_TRUE( opened );
	WatchHandle watch( std::move( opened ).value() );
	TestLoop loop;
	EventCollector collector;
	watch.onEvents( loop, collector.callback() );
	provider->count = PollingWatchBackend::MaxEntries + 1;
	ASSERT_TRUE( loop.runUntil( [&] {
		return watch.status() == WatchStatus::Failed &&
			   collector.has( WatchEventType::RescanRequired, "" );
	} ) );
	const unsigned listings = provider->listings.load();
	const int batches = collector.batches.load();
	std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
	loop.drain();
	EXPECT_EQ( listings, provider->listings.load() );
	EXPECT_EQ( batches, collector.batches.load() );
	EXPECT_FALSE( collector.has( WatchEventType::Removed, "entry0" ) );
	EXPECT_FALSE( collector.has( WatchEventType::Added, "entry100000" ) );
	// A failed large watch does not prevent new, smaller watches from being serviced.
	provider->count = 1;
	auto small = waitFor( vfs.watch( URI( "listing://test/tree" ), options ) );
	ASSERT_TRUE( small );
	EXPECT_EQ( WatchStatus::Polling, small.value().status() );
}
