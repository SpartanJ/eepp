#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <condition_variable>
#include <cstring>
#include <eepp/vfs/path.hpp>
#include <eepp/vfs/watchers/earlywatchevents.hpp>
#include <eepp/vfs/watchers/pollingwatchbackend.hpp>
#include <filesystem>
#include <functional>
#include <limits>
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

UTEST( VFSSafety, equivalentLocalAuthoritiesNeverReplaceTheSource ) {
	TempDir temp;
	VirtualFileSystem vfs;
	const URI file = temp.uri().withAppendedSegment( "file" );
	const URI link = temp.uri().withAppendedSegment( "hardlink" );
	ASSERT_TRUE( waitFor( vfs.writeAll( file, "keep" ) ) );
	std::error_code error;
	const std::string nativeFile = file.getFSPath(), nativeLink = link.getFSPath();
	std::filesystem::create_hard_link(
		std::filesystem::path( std::u8string( nativeFile.begin(), nativeFile.end() ) ),
		std::filesystem::path( std::u8string( nativeLink.begin(), nativeLink.end() ) ), error );
	ASSERT_FALSE( error );
	URI alias = file;
	alias.setHost( "localhost" );
	const auto revision = waitFor( vfs.stat( file ) ).value().revision;
	CopyOptions options;
	options.overwrite = OverwritePolicy::Overwrite;
	for ( const bool reverse : { false, true } ) {
		const URI& source = reverse ? alias : file;
		const URI& destination = reverse ? file : alias;
		EXPECT_EQ( VFSErrorCode::InvalidOperation,
				   codeOf( waitFor( vfs.copy( source, destination, options ).completion() ) ) );
		EXPECT_TRUE( revision.matches( waitFor( vfs.stat( file ) ).value().revision ) );
		EXPECT_EQ( VFSErrorCode::InvalidOperation,
				   codeOf( waitFor( vfs.move( source, destination, options ).completion() ) ) );
		EXPECT_TRUE( revision.matches( waitFor( vfs.stat( file ) ).value().revision ) );
		EXPECT_TRUE( revision.matches( waitFor( vfs.stat( link ) ).value().revision ) );
		EXPECT_STDSTREQ( "keep", readString( vfs, file ) );
		EXPECT_STDSTREQ( "keep", readString( vfs, link ) );
	}
	URI nested = temp.uri().withAppendedSegment( "nested" );
	nested.setHost( "localhost" );
	EXPECT_EQ( VFSErrorCode::InvalidOperation,
			   codeOf( waitFor( vfs.copy( temp.uri(), nested, options ).completion() ) ) );
	EXPECT_EQ( VFSErrorCode::NotFound, codeOf( waitFor( vfs.stat( nested ) ) ) );
	// Distinct files through those same authorities remain transferable.
	URI destination = temp.uri().withAppendedSegment( "copy" );
	destination.setHost( "localhost" );
	ASSERT_TRUE( waitFor( vfs.copy( file, destination ).completion() ) );
	EXPECT_STDSTREQ( "keep", readString( vfs, destination ) );
}

UTEST( VFSSafety, overwriteMovesBetweenHardLinksAreRejected ) {
	TempDir temp;
	VirtualFileSystem vfs;
	const URI source = temp.uri().withAppendedSegment( "source" );
	const URI link = temp.uri().withAppendedSegment( "hardlink" );
	ASSERT_TRUE( waitFor( vfs.writeAll( source, "keep" ) ) );
	auto nativePath = []( const URI& uri ) {
		const std::string path = uri.getFSPath();
		return std::filesystem::path( std::u8string( path.begin(), path.end() ) );
	};
	std::error_code error;
	std::filesystem::create_hard_link( nativePath( source ), nativePath( link ), error );
	ASSERT_FALSE( error );
	const auto revision = waitFor( vfs.stat( source ) ).value().revision;
	CopyOptions options;
	options.overwrite = OverwritePolicy::Overwrite;
	for ( const bool authorityAlias : { false, true } ) {
		URI alias = link;
		if ( authorityAlias )
			alias.setHost( "localhost" );
		for ( const bool reverse : { false, true } ) {
			const URI& from = reverse ? alias : source;
			const URI& to = reverse ? source : alias;
			EXPECT_EQ( VFSErrorCode::InvalidOperation,
					   codeOf( waitFor( vfs.move( from, to, options ).completion() ) ) );
			EXPECT_EQ( VFSErrorCode::InvalidOperation,
					   codeOf( waitFor( vfs.rename( from, to, RenameOptions{ true } ) ) ) );
			EXPECT_TRUE( revision.matches( waitFor( vfs.stat( source ) ).value().revision ) );
			EXPECT_TRUE( revision.matches( waitFor( vfs.stat( link ) ).value().revision ) );
			EXPECT_TRUE(
				std::filesystem::equivalent( nativePath( source ), nativePath( link ), error ) );
			EXPECT_FALSE( error );
			EXPECT_STDSTREQ( "keep", readString( vfs, source ) );
			EXPECT_STDSTREQ( "keep", readString( vfs, link ) );
		}
	}
	// Equal bytes, length and timestamps on a distinct file must not be mistaken for identity.
	const URI destination = temp.uri().withAppendedSegment( "destination" );
	ASSERT_TRUE( waitFor( vfs.writeAll( destination, "keep" ) ) );
	const auto modified = std::filesystem::last_write_time( nativePath( source ), error );
	ASSERT_FALSE( error );
	std::filesystem::last_write_time( nativePath( destination ), modified, error );
	ASSERT_FALSE( error );
	auto moved = waitFor( vfs.move( source, destination, options ).completion() );
	ASSERT_TRUE( moved );
	EXPECT_TRUE( moved.value().renamed );
	EXPECT_EQ( VFSErrorCode::NotFound, codeOf( waitFor( vfs.stat( source ) ) ) );
	EXPECT_STDSTREQ( "keep", readString( vfs, destination ) );
	EXPECT_STDSTREQ( "keep", readString( vfs, link ) );
}

UTEST( VFSProvider, allProvidersExposeCancellableNamespaceIdentity ) {
	TempDir temp;
	VirtualFileSystem vfs;
	vfs.registerProvider( std::make_shared<MemoryProvider>() );
	CancellationSource cancelled;
	cancelled.cancel();
	const URI locations[]{
		temp.uri(),
		URI( "mem://test/file" ),
		URI( "res://fonts/file" ),
		ZipProvider::makeURI( temp.uri().withAppendedSegment( "file.zip" ) ),
		ArchiveProvider::makeURI( temp.uri().withAppendedSegment( "file.tar" ) ),
#ifdef EE_VFS_SFTP
		URI( "sftp://unused-host/file" ),
#endif
	};
	for ( const URI& uri : locations ) {
		auto provider = vfs.providerFor( uri );
		ASSERT_TRUE( provider );
		auto identity = waitFor( provider->sameFileSystem( uri, uri, {} ) );
		ASSERT_TRUE( identity );
		EXPECT_TRUE( identity.value() );
		EXPECT_EQ( VFSErrorCode::Cancelled,
				   codeOf( waitFor( provider->sameFileSystem( uri, uri, cancelled.token() ) ) ) );
	}
	// res:// authorities are convenience path prefixes in the global pack namespace.
	auto resources = vfs.providerFor( URI( "res:///" ) );
	EXPECT_TRUE( waitFor( resources->sameFileSystem( URI( "res://fonts/file" ),
													 URI( "res:///fonts/file" ), {} ) )
					 .value() );
}

UTEST( VFSSafety, transfersUseTheProvidersNamespaceDecision ) {
	class IdentityProvider : public MemoryProvider {
	  public:
		VFSTask<bool> sameFileSystem( const URI& from, const URI&, CancellationToken ) override {
			return readyError<bool>( VFSErrorCode::Unsupported, from, "Identity is uncertain" );
		}
	};
	VirtualFileSystem vfs;
	auto provider = std::make_shared<IdentityProvider>();
	vfs.registerProvider( provider );
	const URI source( "mem://one/source" ), destination( "mem://one/destination" );
	provider->putFile( source, "keep" );
	// The authorities match, but only the provider can decide whether paths are comparable.
	EXPECT_EQ( VFSErrorCode::Unsupported,
			   codeOf( waitFor( vfs.copy( source, destination ).completion() ) ) );
	EXPECT_EQ( VFSErrorCode::Unsupported,
			   codeOf( waitFor( vfs.move( source, destination ).completion() ) ) );
	EXPECT_EQ( VFSErrorCode::NotFound, codeOf( waitFor( vfs.stat( destination ) ) ) );
	EXPECT_STDSTREQ( "keep", readString( vfs, source ) );
}

UTEST( VFSSafety, inMemoryAuthoritiesRemainSeparateNamespaces ) {
	VirtualFileSystem vfs;
	auto provider = std::make_shared<MemoryProvider>();
	vfs.registerProvider( provider );
	provider->putFile( URI( "mem://one/file" ), "one" );
	provider->putDirectory( URI( "mem://two/" ) );
	ASSERT_TRUE(
		waitFor( vfs.move( URI( "mem://one/file" ), URI( "mem://two/file" ) ).completion() ) );
	EXPECT_STDSTREQ( "one", readString( vfs, URI( "mem://two/file" ) ) );
	EXPECT_EQ( VFSErrorCode::NotFound, codeOf( waitFor( vfs.stat( URI( "mem://one/file" ) ) ) ) );
}

UTEST( VFSOrchestration, readRequestsStayBoundedForKnownAndUnknownSizes ) {
	class GeneratedFile : public FileHandleImpl {
	  public:
		explicit GeneratedFile( Uint64 length, bool known ) : size( length ), knownSize( known ) {}

		VFSTask<size_t> read( Uint64 offset, std::span<Uint8> data, CancellationToken ) override {
			largestRead = std::max( largestRead, data.size() );
			++reads;
			// Fault oversized requests immediately; no timed/throttled SSH transfer is needed
			// to catch a capacity reservation escaping into a whole-file operation.
			if ( data.size() > 256u * 1024 )
				return readyError<size_t>( VFSErrorCode::TimedOut );
			const size_t count =
				static_cast<size_t>( std::min<Uint64>( data.size(), size - offset ) );
			std::memset( data.data(), 'x', count );
			return readyValue<size_t>( count );
		}

		VFSTask<size_t> write( Uint64, std::span<const Uint8>, CancellationToken ) override {
			return readyError<size_t>( VFSErrorCode::ReadOnly );
		}

		VFSTask<void> truncate( Uint64 ) override {
			return readyError<void>( VFSErrorCode::ReadOnly );
		}

		VFSTask<void> flush() override { return readyValue<void>(); }

		VFSTask<void> close() override { return readyValue<void>(); }

		VFSTask<VFSFileInfo> stat() override {
			VFSFileInfo info;
			if ( knownSize )
				info.setSize( size );
			return readyValue<VFSFileInfo>( std::move( info ) );
		}

		ProviderCapabilities capabilities() const override { return VFSCapability::Read; }

		Uint64 size;
		size_t largestRead{ 0 };
		size_t reads{ 0 };
		bool knownSize;
	};
	class GeneratedProvider : public MemoryProvider {
	  public:
		VFSTask<FileHandle> open( const URI& uri, const OpenOptions&, CancellationToken ) override {
			return readyValue<FileHandle>( uri, file );
		}

		std::shared_ptr<GeneratedFile> file;
	};
	VirtualFileSystem vfs;
	auto provider = std::make_shared<GeneratedProvider>();
	vfs.registerProvider( provider );
	const URI file( "mem://test/generated" );
	for ( const bool known : { false, true } ) {
		provider->file = std::make_shared<GeneratedFile>( 2 * 1024 * 1024, known );
		auto read = waitFor( vfs.readAll( file ) );
		ASSERT_TRUE( read );
		EXPECT_EQ( 2u * 1024 * 1024, read.value().size() );
		EXPECT_LE( provider->file->largestRead, 256u * 1024 );
		EXPECT_EQ( 9u, provider->file->reads ); // Eight bounded reads, followed by EOF.
		EXPECT_EQ( int( 'x' ), int( read.value().back() ) );
	}
	for ( const bool known : { false, true } ) {
		provider->file = std::make_shared<GeneratedFile>( 10, known );
		auto unlimited = waitFor( vfs.readAll( file, {}, std::numeric_limits<Uint64>::max() ) );
		ASSERT_TRUE( unlimited );
		EXPECT_EQ( 10u, unlimited.value().size() );
		EXPECT_EQ( VFSErrorCode::InvalidOperation,
				   codeOf( waitFor( vfs.readAll( file, {}, 9 ) ) ) );
		EXPECT_EQ( 10u, waitFor( vfs.readAll( file, {}, 10 ) ).value().size() );
		provider->file = std::make_shared<GeneratedFile>( 0, known );
		EXPECT_TRUE( waitFor( vfs.readAll( file, {}, 0 ) ).value().empty() );
		provider->file = std::make_shared<GeneratedFile>( 1, known );
		EXPECT_EQ( VFSErrorCode::InvalidOperation,
				   codeOf( waitFor( vfs.readAll( file, {}, 0 ) ) ) );
	}
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

UTEST( VFSWatch, earlyNativeEventsAlwaysRequireARescan ) {
	RecordingSink sink;
	EE::VFS::detail::EarlyWatchEvents clean;
	clean.replay( sink );
	EXPECT_TRUE( sink.events.empty() );
	for ( const size_t count : { size_t( 1 ), size_t( 5000 ) } ) {
		EE::VFS::detail::EarlyWatchEvents pending;
		for ( size_t i = 0; i < count; ++i )
			pending.push( "/root/", "older", efsw::Actions::Add, {} );
		pending.replay( sink );
	}
	EE::VFS::detail::EarlyWatchEvents missed;
	missed.invalidate();
	missed.replay( sink );
	ASSERT_EQ( 3u, sink.events.size() );
	for ( const auto& event : sink.events )
		EXPECT_EQ( WatchEventType::RescanRequired, event.type );
}

UTEST( VFSWatch, liveEventsDuringRegistrationCannotBeFollowedByStaleReplay ) {
	class PausedSink : public RecordingSink {
	  public:
		void rescanRequired() override {
			std::unique_lock<std::mutex> lock( mutex );
			replaying = true;
			wake.notify_all();
			wake.wait( lock, [&] { return release; } );
			RecordingSink::rescanRequired();
		}

		std::mutex mutex;
		std::condition_variable wake;
		bool replaying{ false };
		bool release{ false };
	};
	EE::VFS::detail::EarlyWatchEvents pending;
	pending.push( "/root/", "entry", efsw::Actions::Add, {} );
	PausedSink sink;
	std::thread registration( [&] { pending.replay( sink ); } );
	bool replaying;
	{
		std::unique_lock<std::mutex> lock( sink.mutex );
		replaying =
			sink.wake.wait_for( lock, std::chrono::seconds( 2 ), [&] { return sink.replaying; } );
		// The newer native event arrives after publication while registration is paused.
		sink.push( WatchEvent{ "entry", {}, WatchEventType::Removed } );
		sink.release = true;
	}
	sink.wake.notify_all();
	registration.join();
	ASSERT_TRUE( replaying );
	ASSERT_EQ( 2u, sink.events.size() );
	EXPECT_EQ( WatchEventType::Removed, sink.events[0].type );
	EXPECT_EQ( WatchEventType::RescanRequired, sink.events[1].type );
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
