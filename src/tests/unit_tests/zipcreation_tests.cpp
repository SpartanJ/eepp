#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <algorithm>
#include <cstring>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/zip.hpp>
#include <eepp/vfs/providers/zipprovider.hpp>
#include <filesystem>
#include <limits>

using namespace EE;
using namespace EE::System;
using namespace EE::VFS;
using namespace vfstest;

namespace {

/** Generates bytes instead of allocating a buffer, including lengths beyond 32 bits. */
class GeneratedZipInput final : public IOStream {
  public:
	GeneratedZipInput( ios_size size, size_t& largestRead, int& live,
					   CancellationSource* cancel = nullptr ) :
		mLargestRead( largestRead ), mLive( live ), mCancel( cancel ), mSize( size ) {
		++mLive;
	}

	~GeneratedZipInput() { --mLive; }

	ios_size read( char* data, ios_size size ) {
		auto count = std::min( size, mSize - mPosition );
		mLargestRead = std::max<size_t>( mLargestRead, count );
		std::memset( data, 'z', count );
		mPosition += count;
		if ( mCancel && count )
			mCancel->cancel();
		return count;
	}

	ios_size write( const char*, ios_size ) { return -1; }

	ios_size seek( ios_size position ) {
		if ( position < 0 || position > mSize )
			return -1;
		return mPosition = position;
	}

	ios_size tell() { return mPosition; }

	ios_size getSize() { return mSize; }

	bool isOpen() { return true; }

  private:
	size_t& mLargestRead;
	int& mLive;
	CancellationSource* mCancel;
	ios_size mSize;
	ios_size mPosition{ 0 };
};

std::filesystem::path nativePath( const URI& uri ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return std::filesystem::path( String( uri.getFSPath() ).toWideString() );
#else
	return std::filesystem::path( uri.getFSPath() );
#endif
}

class UnreadableZipHandle final : public FileHandleImpl {
  public:
	explicit UnreadableZipHandle( FileHandle source ) : mSource( std::move( source ) ) {}

	VFSTask<size_t> read( Uint64, std::span<Uint8>, CancellationToken ) override {
		return readyError<size_t>( VFSErrorCode::TimedOut, mSource.uri(), "Injected read timeout" );
	}

	VFSTask<size_t> write( Uint64, std::span<const Uint8>, CancellationToken ) override {
		return readyError<size_t>( VFSErrorCode::ReadOnly, mSource.uri() );
	}

	VFSTask<void> truncate( Uint64 ) override {
		return readyError<void>( VFSErrorCode::ReadOnly, mSource.uri() );
	}

	VFSTask<void> flush() override { return mSource.flush(); }

	VFSTask<VFSFileInfo> stat() override { return mSource.stat(); }

	VFSTask<void> close() override { return mSource.close(); }

	ProviderCapabilities capabilities() const override { return mSource.capabilities(); }

  private:
	FileHandle mSource;
};

class ZipInputProvider : public MemoryProvider {
  public:
	std::function<VFSTask<FileHandle>( const URI&, const OpenOptions&, CancellationToken )> onOpen;

	VFSTask<FileHandle> open( const URI& uri, const OpenOptions& options,
							  CancellationToken token ) override {
		return onOpen ? onOpen( uri, options, std::move( token ) )
					  : MemoryProvider::open( uri, options, std::move( token ) );
	}
};

} // namespace

UTEST( ZipCreation, DeferredSourcesOwnMemoryAndOpenOneStreamAtATime ) {
	TempDir temp;
	Zip archive( false );
	ASSERT_TRUE( archive.create( temp.path + "deferred.zip" ) );
	{
		std::vector<Uint8> temporary{ 'a', 'b', 'c' };
		ASSERT_TRUE( archive.addFile( temporary, "memory.txt" ) );
		temporary.assign( 3, 'x' );
	}
	size_t largestRead = 0;
	int live = 0, opened = 0;
	for ( int i = 0; i < 12; ++i ) {
		ASSERT_TRUE( archive.addStream(
			[&]() -> std::unique_ptr<IOStream> {
				if ( live != 0 )
					return nullptr;
				++opened;
				return std::make_unique<GeneratedZipInput>( 1024 * 1024, largestRead, live );
			},
			1024 * 1024, String::toString( i ) ) );
	}
	EXPECT_EQ( 0, opened );
	EXPECT_EQ( 0, live );
	ASSERT_TRUE( archive.close() );
	EXPECT_EQ( 12, opened );
	EXPECT_EQ( 0, live );
	EXPECT_LE( largestRead, 256u * 1024 );
	ASSERT_TRUE( archive.open( temp.path + "deferred.zip", Zip::OpenMode::ReadOnly ) );
	std::vector<Uint8> bytes;
	ASSERT_TRUE( archive.extractFileToMemory( "memory.txt", bytes ) );
	EXPECT_STDSTREQ( "abc", std::string( bytes.begin(), bytes.end() ) );
	ASSERT_TRUE( archive.extractFileToMemory( "11", bytes ) );
	EXPECT_EQ( 1024u * 1024, bytes.size() );
	EXPECT_TRUE(
		std::all_of( bytes.begin(), bytes.end(), []( Uint8 byte ) { return byte == 'z'; } ) );
}

UTEST( ZipCreation, BorrowedOutputUsesMaximumDeflateAndPreservesMetadata ) {
	TempDir temp;
	const std::string path = temp.path + "stream.zip";
	{
		IOStreamFile output( path, "w+b" );
		Zip archive( false );
		ASSERT_TRUE( archive.create( output ) );
		Zip::EntryOptions options;
		options.modified = 1700000000;
		options.permissions = 0751;
		std::string text( 65536, 'a' );
		ASSERT_TRUE( archive.addData(
			{ reinterpret_cast<const Uint8*>( text.data() ), text.size() }, "script", options ) );
		ASSERT_TRUE( archive.addDirectory( "empty" ) );
		ASSERT_TRUE( archive.close() );
		EXPECT_TRUE( output.isOpen() );
	}
	std::string raw;
	ASSERT_TRUE( FileSystem::fileGet( path, raw ) );
	ASSERT_GT( raw.size(), 30u );
	EXPECT_EQ( 8u, static_cast<unsigned char>( raw[8] ) );		// DEFLATE
	EXPECT_EQ( 2u, static_cast<unsigned char>( raw[6] ) & 6u ); // maximum compression flag
	VirtualFileSystem vfs;
	auto file = URI::fromFSPath( path );
	auto info = waitFor( vfs.stat( ZipProvider::makeURI( file, "/script" ) ) );
	ASSERT_TRUE( info );
	EXPECT_EQ( 1700000000, info.value().modifiedTime().value_or( 0 ) );
	EXPECT_EQ( 0751u, info.value().permissions().value_or( 0 ) );
	auto empty = waitFor( vfs.stat( ZipProvider::makeURI( file, "/empty" ) ) );
	ASSERT_TRUE( empty );
	EXPECT_TRUE( empty.value().isDirectory() );
}

UTEST( ZipCreation, LargeSourcesRemain64BitAndCanCancelInsideOneEntry ) {
	TempDir temp;
	const std::string path = temp.path + "cancelled.zip";
	Zip archive( false );
	ASSERT_TRUE( archive.create( path ) );
	std::string text = "original";
	ASSERT_TRUE( archive.addData( { reinterpret_cast<const Uint8*>( text.data() ), text.size() },
								  "original" ) );
	ASSERT_TRUE( archive.close() );
	ASSERT_TRUE( archive.open( path ) );
	CancellationSource cancel;
	ASSERT_TRUE( archive.setCancellationToken( cancel.token() ) );
	const Uint64 size = ( Uint64( 1 ) << 32 ) + 123;
	size_t largestRead = 0;
	int live = 0;
	ASSERT_TRUE( archive.addStream(
		[&] { return std::make_unique<GeneratedZipInput>( size, largestRead, live, &cancel ); },
		size, "large" ) );
	Zip::Entry entry;
	ASSERT_TRUE( archive.entry( 1, entry ) );
	EXPECT_EQ( size, entry.size );
	EXPECT_FALSE( archive.close() );
	EXPECT_TRUE( archive.error() == Zip::Error::Cancelled );
	EXPECT_EQ( 0, live );
	EXPECT_GT( largestRead, 0u );
	EXPECT_LE( largestRead, 256u * 1024 );
	ASSERT_TRUE( archive.open( path, Zip::OpenMode::ReadOnly ) );
	EXPECT_EQ( 1u, archive.entryCount() );
	EXPECT_GE( archive.exists( "original" ), 0 );
}

UTEST( ZipCreation, VFSSelectionStreamsUnicodeFoldersAndEmptyEntries ) {
	TempDir temp;
	VirtualFileSystem vfs;
	auto memory = std::make_shared<MemoryProvider>();
	vfs.registerProvider( memory );
	URI folder( "mem://compress/資料" );
	memory->putFile( folder.withAppendedSegment( "sub/mañana.txt" ), "contents", 0750 );
	memory->putFile( folder.withAppendedSegment( ".hidden" ), "hidden" );
	memory->putFile( folder.withAppendedSegment( "zero" ), "" );
	memory->putDirectory( folder.withAppendedSegment( "empty" ) );
	URI other( "mem://compress/other.txt" );
	memory->putFile( other, "other" );
	auto destination = temp.uri().withAppendedSegment( "選択.zip" );
	auto transfer = vfs.createZip( { folder, other }, destination );
	auto result = waitFor( transfer.completion() );
	ASSERT_TRUE( result );
	EXPECT_EQ( 4u, result.value().filesTransferred );
	EXPECT_EQ( 3u, result.value().directoriesCreated );
	EXPECT_EQ( 19u, result.value().bytesTransferred );
	auto root = ZipProvider::makeURI( destination );
	EXPECT_STDSTREQ( "contents",
					 readString( vfs, root.withAppendedSegment( "資料/sub/mañana.txt" ) ) );
	EXPECT_STDSTREQ( "hidden", readString( vfs, root.withAppendedSegment( "資料/.hidden" ) ) );
	EXPECT_STDSTREQ( "other", readString( vfs, root.withAppendedSegment( "other.txt" ) ) );
	auto empty = waitFor( vfs.stat( root.withAppendedSegment( "資料/empty" ) ) );
	ASSERT_TRUE( empty );
	EXPECT_TRUE( empty.value().isDirectory() );
	auto info = waitFor( vfs.stat( root.withAppendedSegment( "資料/sub/mañana.txt" ) ) );
	ASSERT_TRUE( info );
	EXPECT_EQ( 0750u, info.value().permissions().value_or( 0 ) );
	const auto repacked = temp.uri().withAppendedSegment( "repacked.zip" );
	auto copied =
		waitFor( vfs.createZip( { root.withAppendedSegment( "資料" ) }, repacked ).completion() );
	ASSERT_TRUE( copied );
	EXPECT_STDSTREQ( "contents",
					 readString( vfs, ZipProvider::makeURI( repacked, "/資料/sub/mañana.txt" ) ) );
}

UTEST( ZipCreation, ExistingDestinationAndFailedInputNeverPublishPartialOutput ) {
	TempDir temp;
	VirtualFileSystem vfs;
	auto memory = std::make_shared<MemoryProvider>();
	vfs.registerProvider( memory );
	URI source( "mem://compress/file" );
	memory->putFile( source, "new" );
	auto destination = temp.uri().withAppendedSegment( "existing.zip" );
	ASSERT_TRUE( waitFor( vfs.writeAll( destination, std::string_view( "existing" ) ) ) );
	auto result = waitFor( vfs.createZip( { source }, destination ).completion() );
	EXPECT_TRUE( codeOf( result ) == VFSErrorCode::AlreadyExists );
	EXPECT_STDSTREQ( "existing", readString( vfs, destination ) );
	destination = temp.uri().withAppendedSegment( "missing.zip" );
	result =
		waitFor( vfs.createZip( { URI( "mem://compress/absent" ) }, destination ).completion() );
	EXPECT_TRUE( codeOf( result ) == VFSErrorCode::NotFound );
	EXPECT_TRUE( codeOf( waitFor( vfs.stat( destination ) ) ) == VFSErrorCode::NotFound );
	result = waitFor(
		vfs.createZip( { source, URI( "mem://elsewhere/file" ) }, destination ).completion() );
	EXPECT_TRUE( codeOf( result ) == VFSErrorCode::AlreadyExists );
	result =
		waitFor( vfs.createZip( { source }, URI( "mem://compress/result.zip" ) ).completion() );
	EXPECT_TRUE( codeOf( result ) == VFSErrorCode::Unsupported );
	auto entries = waitFor( vfs.listAll( temp.uri() ) );
	ASSERT_TRUE( entries );
	EXPECT_EQ( 1u, entries.value().size() );
}

UTEST( ZipCreation, CancelledSelectionDoesNotLeaveOutputOrTemporaryFiles ) {
	TempDir temp;
	VirtualFileSystem vfs;
	auto memory = std::make_shared<MemoryProvider>();
	vfs.registerProvider( memory );
	URI source( "mem://compress/root" );
	for ( int i = 0; i < 1000; ++i )
		memory->putFile( source.withAppendedSegment( String::toString( i ) ), "data" );
	auto transfer = vfs.createZip( { source }, temp.uri().withAppendedSegment( "cancelled.zip" ) );
	transfer.cancel();
	auto result = waitFor( transfer.completion() );
	EXPECT_TRUE( codeOf( result ) == VFSErrorCode::Cancelled );
	auto entries = waitFor( vfs.listAll( temp.uri() ) );
	ASSERT_TRUE( entries );
	EXPECT_TRUE( entries.value().empty() );
}

UTEST( ZipCreation, FailedReadAndChangedSourceRollBackTheTransaction ) {
	TempDir temp;
	VirtualFileSystem vfs;
	auto memory = std::make_shared<ZipInputProvider>();
	vfs.registerProvider( memory );
	const URI source( "mem://compress/file" );
	memory->putFile( source, "old" );
	const auto destination = temp.uri().withAppendedSegment( "output.zip" );
	memory->onOpen = []( const URI& uri, const OpenOptions&, CancellationToken ) {
		return readyError<FileHandle>( VFSErrorCode::PermissionDenied, uri,
									   "Injected read failure" );
	};
	auto result = waitFor( vfs.createZip( { source }, destination ).completion() );
	EXPECT_TRUE( codeOf( result ) == VFSErrorCode::PermissionDenied );
	auto files = waitFor( vfs.listAll( temp.uri() ) );
	ASSERT_TRUE( files );
	EXPECT_TRUE( files.value().empty() );
	memory->onOpen = [&]( const URI& uri, const OpenOptions& options, CancellationToken token ) {
		return memory->MemoryProvider::open( uri, options, std::move( token ) )
			.then( [uri]( VFSResult<FileHandle>&& opened ) -> VFSResult<FileHandle> {
				if ( !opened )
					return Err( std::move( opened ).error() );
				return Ok( FileHandle(
					uri, std::make_shared<UnreadableZipHandle>( std::move( opened ).value() ) ) );
			} );
	};
	result = waitFor( vfs.createZip( { source }, destination ).completion() );
	EXPECT_TRUE( codeOf( result ) == VFSErrorCode::TimedOut );
	files = waitFor( vfs.listAll( temp.uri() ) );
	ASSERT_TRUE( files );
	EXPECT_TRUE( files.value().empty() );
	memory->onOpen = [&]( const URI& uri, const OpenOptions& options, CancellationToken token ) {
		// Replace with the same length after the writer checked the source revision.
		memory->putFile( uri, "new" );
		return memory->MemoryProvider::open( uri, options, std::move( token ) );
	};
	result = waitFor( vfs.createZip( { source }, destination ).completion() );
	EXPECT_TRUE( codeOf( result ) == VFSErrorCode::ChangedExternally );
	files = waitFor( vfs.listAll( temp.uri() ) );
	ASSERT_TRUE( files );
	EXPECT_TRUE( files.value().empty() );
}

UTEST( ZipCreation, DestinationCreatedDuringCompressionIsNeverOverwritten ) {
	TempDir temp;
	VirtualFileSystem vfs;
	auto memory = std::make_shared<ZipInputProvider>();
	vfs.registerProvider( memory );
	const URI source( "mem://compress/file" );
	memory->putFile( source, "data" );
	const auto destination = temp.uri().withAppendedSegment( "output.zip" );
	memory->onOpen = [&]( const URI& uri, const OpenOptions& options, CancellationToken token ) {
		FileSystem::fileWrite( destination.getFSPath(), std::string_view( "another creator" ) );
		return memory->MemoryProvider::open( uri, options, std::move( token ) );
	};
	auto result = waitFor( vfs.createZip( { source }, destination ).completion() );
	EXPECT_TRUE( codeOf( result ) == VFSErrorCode::AlreadyExists );
	EXPECT_STDSTREQ( "another creator", readString( vfs, destination ) );
	auto files = waitFor( vfs.listAll( temp.uri() ) );
	ASSERT_TRUE( files );
	EXPECT_EQ( 1u, files.value().size() );
}

UTEST( ZipCreation, SymbolicLinksAreStoredWithoutFollowingThem ) {
	TempDir temp;
	VirtualFileSystem vfs;
	const auto root = temp.uri().withAppendedSegment( "links" );
	ASSERT_TRUE( waitFor( vfs.makeDirs( root ) ) );
	std::error_code error;
	std::filesystem::create_directory_symlink(
		"..", nativePath( root.withAppendedSegment( "loop" ) ), error );
	if ( error )
		UTEST_SKIP( "Creating symbolic links is unavailable to this user" );
	std::filesystem::create_symlink( "missing-target",
									 nativePath( root.withAppendedSegment( "broken" ) ), error );
	ASSERT_FALSE( bool( error ) );
	for ( const auto& name : { "loop", "broken" } ) {
		auto link = waitFor( vfs.stat( root.withAppendedSegment( name ), StatOptions{ false } ) );
		ASSERT_TRUE_MSG( link, link ? "" : link.error().toString().c_str() );
		EXPECT_EQ( FileType::Symlink, link.value().type );
		EXPECT_STDSTREQ( std::string( name ) == "loop" ? ".." : "missing-target",
						 link.value().symlinkTarget );
	}
	const auto destination = temp.uri().withAppendedSegment( "links.zip" );
	auto result = waitFor( vfs.createZip( { root }, destination ).completion() );
	ASSERT_TRUE_MSG( result, result ? "" : result.error().toString().c_str() );
	EXPECT_EQ( 2u, result.value().filesTransferred );
	EXPECT_EQ( 1u, result.value().directoriesCreated );
	Zip archive( false );
	ASSERT_TRUE( archive.open( destination.getFSPath(), Zip::OpenMode::ReadOnly ) );
	EXPECT_EQ( 3u, archive.entryCount() );
	for ( const auto& name : { "loop", "broken" } ) {
		Zip::Entry entry;
		ASSERT_TRUE( archive.entry( archive.exists( std::string( "links/" ) + name ), entry ) );
		EXPECT_EQ( 0120000u, entry.unixMode & 0170000 );
		std::vector<Uint8> bytes;
		ASSERT_TRUE( archive.extractFileToMemory( entry.name, bytes ) );
		EXPECT_STDSTREQ( std::string( name ) == "loop" ? ".." : "missing-target",
						 std::string( bytes.begin(), bytes.end() ) );
	}
}

UTEST( ZipCreation, ArchiveInsideSelectionExcludesItsOwnTemporaryOutput ) {
	TempDir temp;
	VirtualFileSystem vfs;
	const auto folder = temp.uri().withAppendedSegment( "project" );
	ASSERT_TRUE( waitFor( vfs.makeDirs( folder ) ) );
	ASSERT_TRUE( waitFor(
		vfs.writeAll( folder.withAppendedSegment( "file" ), std::string_view( "data" ) ) ) );
	const auto destination = folder.withAppendedSegment( "inside.zip" );
	auto result = waitFor( vfs.createZip( { folder }, destination ).completion() );
	ASSERT_TRUE( result );
	Zip archive( false );
	ASSERT_TRUE( archive.open( destination.getFSPath(), Zip::OpenMode::ReadOnly ) );
	EXPECT_EQ( 2u, archive.entryCount() );
	EXPECT_GE( archive.exists( "project/" ), 0 );
	EXPECT_GE( archive.exists( "project/file" ), 0 );
	auto entries = waitFor( vfs.listAll( folder ) );
	ASSERT_TRUE( entries );
	EXPECT_EQ( 2u, entries.value().size() );
}
