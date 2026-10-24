#include "utest.hpp"
#include "vfs_test_utils.hpp"

#include <cstring>
#include <eepp/ui/doc/textdocument.hpp>

using namespace EE;
using namespace EE::VFS;
using namespace vfstest;

namespace {

class StreamFile : public FileHandleImpl {
  public:
	VFSTask<size_t> read( Uint64 offset, std::span<Uint8> target, CancellationToken token ) {
		++reads;
		if ( token.isCancellationRequested() )
			return readyError<size_t>( VFSErrorCode::Cancelled );
		if ( readError )
			return readyError<size_t>( VFSErrorCode::TimedOut );
		const size_t count =
			offset >= data.size()
				? 0
				: std::min(
					  { target.size(), data.size() - static_cast<size_t>( offset ), partial } );
		std::memcpy( target.data(), data.data() + std::min<Uint64>( offset, data.size() ), count );
		return readyValue<size_t>( count );
	}

	VFSTask<size_t> write( Uint64 offset, std::span<const Uint8> source, CancellationToken ) {
		++writes;
		const size_t count = std::min( partial, source.size() );
		if ( offset + count > data.size() )
			data.resize( offset + count );
		std::memcpy( data.data() + offset, source.data(), count );
		return readyValue<size_t>( count );
	}

	VFSTask<void> truncate( Uint64 ) { return readyError<void>( VFSErrorCode::Unsupported ); }

	VFSTask<void> flush() {
		++flushes;
		return readyValue<void>();
	}

	VFSTask<void> close() {
		++closes;
		return closeError ? readyError<void>( VFSErrorCode::IO ) : readyValue<void>();
	}

	VFSTask<VFSFileInfo> stat() {
		VFSFileInfo info;
		info.type = FileType::Regular;
		if ( knownSize )
			info.setSize( data.size() );
		return readyValue<VFSFileInfo>( std::move( info ) );
	}

	ProviderCapabilities capabilities() const {
		return VFSCapability::Read | VFSCapability::Write | VFSCapability::Seek;
	}

	std::string data{ "0123456789" };
	size_t partial{ 1024 * 1024 };
	int reads{ 0 }, writes{ 0 }, closes{ 0 }, flushes{ 0 };
	bool readError{ false }, closeError{ false }, knownSize{ true };
};

VFSResult<std::unique_ptr<IOStreamVFS>> adapt( std::shared_ptr<StreamFile> file,
											   CancellationToken token = {},
											   Uint64 limit = IOStreamVFS::Unlimited ) {
	return IOStreamVFS::create( FileHandle( URI( "test://stream/file" ), std::move( file ) ),
								OpenFlags::ReadWrite, std::move( token ), limit );
}

} // namespace

UTEST( VFSStream, LazyCacheSeeksWritesAndPartialOperations ) {
	auto file = std::make_shared<StreamFile>();
	file->partial = 7;
	auto opened = adapt( file );
	ASSERT_TRUE( opened );
	auto& stream = *opened.value();
	EXPECT_EQ( 0, file->reads );
	char bytes[16]{};
	EXPECT_EQ( 3, stream.read( bytes, 3 ) );
	EXPECT_TRUE( std::string( bytes, 3 ) == "012" );
	EXPECT_EQ( 1, file->reads );
	EXPECT_EQ( 1, stream.seek( 1 ) );
	EXPECT_EQ( 2, stream.read( bytes, 2 ) );
	EXPECT_TRUE( std::string( bytes, 2 ) == "12" );
	EXPECT_EQ( 1, file->reads ); // Rewinding a header stays in the read cache.
	EXPECT_EQ( 0, stream.seek( 0 ) );
	EXPECT_EQ( 10, stream.read( bytes, sizeof( bytes ) ) );
	EXPECT_TRUE( std::string( bytes, 10 ) == "0123456789" );
	EXPECT_EQ( 2, file->reads );
	EXPECT_EQ( 0, stream.read( bytes, 1 ) );
	EXPECT_FALSE( stream.error() );
	EXPECT_EQ( 1, stream.seek( 1 ) );
	EXPECT_EQ( 9, stream.write( "abcdefghi", 9 ) );
	EXPECT_EQ( 2, file->writes );
	EXPECT_EQ( 0, stream.seek( 0 ) );
	EXPECT_EQ( 10, stream.read( bytes, 10 ) );
	EXPECT_TRUE( std::string( bytes, 10 ) == "0abcdefghi" );
	EXPECT_EQ( 13, stream.seek( 13 ) );
	EXPECT_EQ( 1, stream.write( "z", 1 ) );
	EXPECT_EQ( 14, stream.getSize() );
	EXPECT_TRUE( stream.flush() );
	EXPECT_EQ( 1, file->flushes );
	EXPECT_TRUE( stream.close() );
	EXPECT_FALSE( stream.isOpen() );
	EXPECT_EQ( 1, file->closes );
}

UTEST( VFSStream, CancellationAlsoInvalidatesCachedReadsAndRetainsTheCause ) {
	auto file = std::make_shared<StreamFile>();
	CancellationSource cancellation;
	auto opened = adapt( file, cancellation.token() );
	ASSERT_TRUE( opened );
	char byte{};
	EXPECT_EQ( 1, opened.value()->read( &byte, 1 ) );
	EXPECT_EQ( 0, opened.value()->seek( 0 ) );
	cancellation.cancel();
	EXPECT_EQ( 0, opened.value()->read( &byte, 1 ) );
	ASSERT_TRUE( opened.value()->error() );
	EXPECT_TRUE( opened.value()->error()->is( VFSErrorCode::Cancelled ) );
	EXPECT_EQ( 1, file->reads );
	file->closeError = true;
	EXPECT_EQ( VFSErrorCode::Cancelled, codeOf( opened.value()->close() ) );
	EXPECT_EQ( 1, file->closes );
}

UTEST( VFSStream, ReadFailuresDifferFromEOFAndPropagateThroughTextLoading ) {
	auto file = std::make_shared<StreamFile>();
	file->readError = true;
	auto opened = adapt( file );
	ASSERT_TRUE( opened );
	EE::UI::Doc::TextDocument document( false );
	EXPECT_TRUE( document.loadFromStream( *opened.value() ) ==
				 EE::UI::Doc::TextDocument::LoadStatus::Failed );
	ASSERT_TRUE( opened.value()->error() );
	EXPECT_TRUE( opened.value()->error()->is( VFSErrorCode::TimedOut ) );
	EXPECT_FALSE( opened.value()->isOpen() );
	opened.value().reset();
	EXPECT_EQ( 1, file->closes );
}

UTEST( VFSStream, InvalidLengthsAndEarlyEOFReleaseTheHandle ) {
	auto file = std::make_shared<StreamFile>();
	file->knownSize = false;
	EXPECT_EQ( VFSErrorCode::Unsupported, codeOf( adapt( file ) ) );
	EXPECT_EQ( 1, file->closes );
	file->knownSize = true;
	EXPECT_EQ( VFSErrorCode::InvalidOperation, codeOf( adapt( file, {}, 9 ) ) );
	EXPECT_EQ( 2, file->closes );
	auto opened = adapt( file );
	ASSERT_TRUE( opened );
	file->data.clear();
	char bytes[10];
	EXPECT_EQ( 0, opened.value()->read( bytes, 10 ) );
	ASSERT_TRUE( opened.value()->error() );
	EXPECT_TRUE( opened.value()->error()->is( VFSErrorCode::IO ) );
	EXPECT_EQ( -1, opened.value()->seek( 0 ) );
}

UTEST( VFSStream, CleanupErrorsAreReportedAndCancellationBeforeOpenDoesNotRead ) {
	auto file = std::make_shared<StreamFile>();
	CancellationSource cancellation;
	cancellation.cancel();
	EXPECT_EQ( VFSErrorCode::Cancelled, codeOf( adapt( file, cancellation.token() ) ) );
	EXPECT_EQ( 0, file->reads );
	EXPECT_EQ( 1, file->closes );
	auto opened = adapt( file );
	ASSERT_TRUE( opened );
	file->closeError = true;
	EXPECT_EQ( VFSErrorCode::IO, codeOf( opened.value()->close() ) );
	EXPECT_EQ( 2, file->closes );
}

UTEST( VFSStream, LocalMemoryAndAsyncProvidersSupportOwnedStreams ) {
	TempDir directory;
	VirtualFileSystem vfs;
	auto pool = EE::System::ThreadPool::createShared( 1 );
	vfs.registerProvider( std::make_shared<MemoryProvider>( "mem", pool.get() ) );
	for ( const auto& uri :
		  { directory.uri().withAppendedSegment( "stream.txt" ), URI( "mem://test/file" ) } ) {
		auto opened = waitFor(
			vfs.openStream( uri, OpenFlags::ReadWrite | OpenFlags::Create | OpenFlags::Truncate ) );
		ASSERT_TRUE( opened );
		EXPECT_EQ( 8, opened.value()->write( "streamed", 8 ) );
		EXPECT_EQ( 0, opened.value()->seek( 0 ) );
		char bytes[8];
		EXPECT_EQ( 8, opened.value()->read( bytes, sizeof( bytes ) ) );
		EXPECT_TRUE( std::string( bytes, sizeof( bytes ) ) == "streamed" );
		EXPECT_TRUE( opened.value()->close() );
	}
	EXPECT_LE( sizeof( IOStreamVFS ), 2 * sizeof( URI ) + 160 );
	printf( "  IOStreamVFS: %zu bytes (read cache allocated on demand)\n", sizeof( IOStreamVFS ) );
}
