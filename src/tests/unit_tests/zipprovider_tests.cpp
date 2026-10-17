#include "../../tools/eefm/location.hpp"
#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <array>
#include <cerrno>
#include <eepp/system/base64.hpp>
#include <eepp/system/packfileindex.hpp>
#include <eepp/system/zip.hpp>
#include <eepp/vfs/providers/zipprovider.hpp>
#include <fstream>

using namespace EE;
using namespace EE::System;
using namespace EE::VFS;
using namespace vfstest;

namespace {

bool makeZip( const std::string& path,
			  std::initializer_list<std::pair<std::string, std::string>> files ) {
	Zip archive;
	if ( !archive.create( path ) )
		return false;
	for ( const auto& [name, data] : files ) {
		if ( !archive.addFile( reinterpret_cast<const Uint8*>( data.data() ), data.size(), name ) )
			return false;
	}
	return archive.close();
}

} // namespace

UTEST( ZipProvider, archiveURIsPreserveIdentityAndMemberNames ) {
	TempDir temp;
	const URI file = URI::fromFSPath( temp.path + "Mixed Case !!#%?.zip" );
	const URI root = ZipProvider::makeURI( file );
	const URI member = root.withAppendedSegment( "dir" ).withAppendedSegment( "Name #%?.txt" );
	const URI roundTrip( member.toString() );
	EXPECT_TRUE( member == roundTrip );
	auto backing = ZipProvider::archiveURI( roundTrip );
	ASSERT_TRUE( backing.hasValue() );
	EXPECT_TRUE( file == backing.value() );
	EXPECT_TRUE( file == ZipProvider::archiveURI( member.getParent().getParent() ).value() );
	EXPECT_FALSE( ZipProvider::archiveURI( root.getParent() ).hasValue() );
	EXPECT_FALSE( root == ZipProvider::makeURI( URI::fromFSPath( temp.path + "other.zip" ) ) );
	EXPECT_TRUE( member.isSameOrChildOf( root ) );
	const URI bangArchive = URI::fromFSPath( temp.path + "packages!/archive!.zip" );
	const URI bangMember = ZipProvider::makeURI( bangArchive, "/folder!/name!.txt" );
	EXPECT_TRUE( bangArchive == ZipProvider::archiveURI( URI( bangMember.toString() ) ).value() );
	EXPECT_STDSTREQ( "/folder!/name!.txt", ZipProvider::memberPath( bangMember ).value() );
	const URI clamped = ZipProvider::makeURI( file, "/../../inside" );
	EXPECT_TRUE( file == ZipProvider::archiveURI( clamped ).value() );
	EXPECT_STDSTREQ( "/inside", ZipProvider::memberPath( clamped ).value() );
	EXPECT_EQ( VFSErrorCode::InvalidURI,
			   codeOf( ZipProvider::archiveURI( URI( "zip://nothex/" ) ) ) );
	EXPECT_EQ(
		VFSErrorCode::InvalidURI,
		codeOf( ZipProvider::archiveURI( ZipProvider::makeURI( URI( "sftp://host/a.zip" ) ) ) ) );
}

UTEST( ZipProvider, listsImplicitAndEmptyDirectoriesWithMetadata ) {
	TempDir temp;
	const std::string path = temp.path + "archive.zip";
	ASSERT_TRUE( makeZip( path, { { "top.txt", "hello" },
								  { "dir/leaf.txt", "leaf" },
								  { "empty/", "" },
								  { ".hidden", "secret" },
								  { "dir/Upper #%.txt", "mixed" } } ) );
	VirtualFileSystem vfs;
	const URI root = ZipProvider::makeURI( URI::fromFSPath( path ) );
	auto listed = waitFor( vfs.listAll( root ) );
	ASSERT_TRUE( listed.hasValue() );
	EXPECT_TRUE( sortedNames( listed.value() ) ==
				 std::vector<std::string>( { ".hidden", "dir", "empty", "top.txt" } ) );
	auto folder = waitFor( vfs.stat( root.withAppendedSegment( "dir" ) ) );
	ASSERT_TRUE( folder.hasValue() );
	EXPECT_TRUE( folder.value().isDirectory() );
	auto file = waitFor( vfs.stat( root.withAppendedSegment( "top.txt" ) ) );
	ASSERT_TRUE( file.hasValue() );
	EXPECT_EQ( 5u, file.value().size().value_or( 0 ) );
	EXPECT_TRUE( file.value().modifiedTime().has_value() );
	EXPECT_FALSE( file.value().revision.empty() );
	EXPECT_FALSE( file.value().permissions().has_value() );
	EXPECT_FALSE( folder.value().permissions().has_value() );
	auto empty = waitFor( vfs.listAll( root.withAppendedSegment( "empty" ) ) );
	ASSERT_TRUE( empty.hasValue() );
	EXPECT_TRUE( empty.value().empty() );
	EXPECT_STDSTREQ( "mixed",
					 readString( vfs, root.withAppendedSegment( "dir" ).withAppendedSegment(
										  "Upper #%.txt" ) ) );
	auto opened = waitFor( vfs.openDirectory( root ) );
	ASSERT_TRUE( opened.hasValue() );
	auto handle = std::move( opened ).value();
	size_t count = 0;
	bool end = false;
	while ( !end ) {
		auto batch = waitFor( handle.read( 1 ) );
		ASSERT_TRUE( batch.hasValue() );
		count += batch.value().entries.size();
		end = batch.value().endOfDirectory;
	}
	EXPECT_EQ( 4u, count );
	EXPECT_TRUE( waitFor( handle.close() ).hasValue() );
}

UTEST( ZipProvider, streamingReadsSeekBackwardsAndCopyAcrossProviders ) {
	TempDir temp;
	std::string content( 300000, 'x' );
	Uint32 random = 1;
	for ( auto& byte : content ) {
		random = random * 1664525u + 1013904223u;
		byte = static_cast<char>( random >> 24 );
	}
	const std::string path = temp.path + "archive.zip";
	ASSERT_TRUE( makeZip(
		path, { { "dir/large.txt", content }, { "dir/second.txt", "second" }, { "zero", "" } } ) );
	VirtualFileSystem vfs;
	const URI root = ZipProvider::makeURI( URI::fromFSPath( path ) );
	const URI file = root.withAppendedSegment( "dir" ).withAppendedSegment( "large.txt" );
	auto opened = waitFor( vfs.open( file ) );
	ASSERT_TRUE( opened.hasValue() );
	auto handle = std::move( opened ).value();
	std::array<Uint8, 31> buffer;
	for ( Uint64 offset : { 0ull, 100000ull, 15ull, 299980ull, 300000ull } ) {
		auto result = waitFor( handle.read( offset, buffer ) );
		ASSERT_TRUE( result.hasValue() );
		const std::string expected = content.substr( offset, buffer.size() );
		EXPECT_EQ( expected.size(), result.value() );
		EXPECT_TRUE( expected == std::string( buffer.begin(), buffer.begin() + result.value() ) );
	}
	EXPECT_TRUE( waitFor( handle.close() ).hasValue() );
	EXPECT_EQ( VFSErrorCode::InvalidOperation, codeOf( waitFor( handle.read( 0, buffer ) ) ) );
	EXPECT_STDSTREQ( "", readString( vfs, root.withAppendedSegment( "zero" ) ) );
	EXPECT_TRUE( content == readString( vfs, file ) );
	const URI destination = URI::fromFSPath( temp.path + "extracted" );
	auto copied =
		waitFor( vfs.copy( root.withAppendedSegment( "dir" ), destination ).completion() );
	ASSERT_TRUE( copied.hasValue() );
	EXPECT_TRUE( content == readString( vfs, destination.withAppendedSegment( "large.txt" ) ) );
	EXPECT_STDSTREQ( "second", readString( vfs, destination.withAppendedSegment( "second.txt" ) ) );
	EXPECT_TRUE( waitFor( vfs.readAll( file, {}, 100 ) ).hasError() );
	// An in-place edit can break a backward seek. Keep the old stream alive when reopening
	// fails: this is an I/O failure, and must not turn every later call into "File closed".
	auto reopened = waitFor( vfs.open( file ) );
	ASSERT_TRUE( reopened.hasValue() );
	auto changing = std::move( reopened ).value();
	ASSERT_TRUE( waitFor( changing.read( 200000, buffer ) ).hasValue() );
	ASSERT_TRUE( FileSystem::fileWrite( path, std::string{} ) );
	EXPECT_TRUE( waitFor( changing.read( 0, buffer ) ).hasError() );
	auto eof = waitFor( changing.read( content.size(), buffer ) );
	ASSERT_TRUE( eof.hasValue() );
	EXPECT_EQ( 0u, eof.value() );
}

UTEST( ZipProvider, readOnlyErrorsMissingFilesAndCancellation ) {
	TempDir temp;
	const std::string path = temp.path + "archive.zip";
	ASSERT_TRUE( makeZip( path, { { "file", "contents" } } ) );
	VirtualFileSystem vfs;
	const URI root = ZipProvider::makeURI( URI::fromFSPath( path ) );
	const URI file = root.withAppendedSegment( "file" );
	EXPECT_EQ( VFSErrorCode::ReadOnly, codeOf( waitFor( vfs.open( file, OpenFlags::Write ) ) ) );
	EXPECT_EQ( VFSErrorCode::ReadOnly,
			   codeOf( waitFor( vfs.makeDir( root.withAppendedSegment( "new" ) ) ) ) );
	EXPECT_EQ( VFSErrorCode::ReadOnly, codeOf( waitFor( vfs.remove( file ) ) ) );
	EXPECT_EQ( VFSErrorCode::ReadOnly,
			   codeOf( waitFor( vfs.rename( file, root.withAppendedSegment( "renamed" ) ) ) ) );
	EXPECT_EQ( VFSErrorCode::IsDirectory, codeOf( waitFor( vfs.open( root ) ) ) );
	EXPECT_EQ( VFSErrorCode::NotDirectory, codeOf( waitFor( vfs.openDirectory( file ) ) ) );
	const URI fileWithSlash = ZipProvider::makeURI( URI::fromFSPath( path ), "/file/" );
	EXPECT_EQ( VFSErrorCode::NotDirectory, codeOf( waitFor( vfs.stat( fileWithSlash ) ) ) );
	EXPECT_EQ( VFSErrorCode::NotDirectory, codeOf( waitFor( vfs.open( fileWithSlash ) ) ) );
	EXPECT_EQ( VFSErrorCode::NotDirectory,
			   codeOf( waitFor( vfs.stat( file.withAppendedSegment( "child" ) ) ) ) );
	EXPECT_EQ( VFSErrorCode::NotFound,
			   codeOf( waitFor( vfs.stat( root.withAppendedSegment( "missing" ) ) ) ) );
	CancellationSource cancelled;
	cancelled.cancel();
	EXPECT_EQ( VFSErrorCode::Cancelled,
			   codeOf( waitFor( vfs.open( file, {}, cancelled.token() ) ) ) );
	const URI missing = ZipProvider::makeURI( URI::fromFSPath( temp.path + "missing.zip" ) );
	EXPECT_EQ( VFSErrorCode::NotFound, codeOf( waitFor( vfs.stat( missing ) ) ) );
	EXPECT_FALSE( FileSystem::fileExists( temp.path + "missing.zip" ) );
	ASSERT_TRUE( FileSystem::fileWrite( temp.path + "bad.zip", "this is not a ZIP" ) );
	EXPECT_EQ( VFSErrorCode::InvalidOperation,
			   codeOf( waitFor( vfs.stat(
				   ZipProvider::makeURI( URI::fromFSPath( temp.path + "bad.zip" ) ) ) ) ) );
	EXPECT_FALSE( vfs.capabilities( file ).has( VFSCapability::Write ) );
	auto opened = waitFor( vfs.open( file ) );
	ASSERT_TRUE( opened.hasValue() );
	auto handle = std::move( opened ).value();
	const std::array<Uint8, 1> byte{ 'x' };
	EXPECT_EQ( VFSErrorCode::ReadOnly, codeOf( waitFor( handle.write( 0, byte ) ) ) );
	EXPECT_EQ( VFSErrorCode::ReadOnly, codeOf( waitFor( handle.truncate( 0 ) ) ) );
}

UTEST( ZipProvider, movesFromZipAndResourcesLeaveDestinationsUntouched ) {
	TempDir temp;
	const std::string path = temp.path + "archive.zip";
	ASSERT_TRUE( makeZip( path, { { "vfs-readonly-move/file.txt", "source" },
								  { "vfs-readonly-move/dir/child.txt", "source" } } ) );
	// Register the archive as a resource pack too, so both read-only providers use real data.
	Zip pack;
	ASSERT_TRUE( pack.open( path ) );
	VirtualFileSystem vfs;
	CopyOptions options;
	options.overwrite = OverwritePolicy::Overwrite;
	for ( const URI& root : { ZipProvider::makeURI( URI::fromFSPath( path ), "/vfs-readonly-move" ),
							  URI( "res:///vfs-readonly-move" ) } ) {
		for ( const std::string name : { "file.txt", "dir" } ) {
			const URI source = root.withAppendedSegment( name );
			const URI destination = URI::fromFSPath( temp.path + root.getScheme() + "-" + name );
			auto moved = waitFor( vfs.move( source, destination ).completion() );
			EXPECT_EQ( VFSErrorCode::Unsupported, codeOf( moved ) );
			EXPECT_EQ( VFSErrorCode::NotFound, codeOf( waitFor( vfs.stat( destination ) ) ) );

			const bool directory = name == "dir";
			if ( directory )
				ASSERT_TRUE( waitFor( vfs.makeDirs( destination ) ).hasValue() );
			const URI destinationFile =
				directory ? destination.withAppendedSegment( "child.txt" ) : destination;
			ASSERT_TRUE( writeString( vfs, destinationFile, "existing destination" ).hasValue() );
			moved = waitFor( vfs.move( source, destination, options ).completion() );
			EXPECT_EQ( VFSErrorCode::Unsupported, codeOf( moved ) );
			EXPECT_STDSTREQ( "existing destination", readString( vfs, destinationFile ) );
			EXPECT_STDSTREQ(
				"source",
				readString( vfs, directory ? source.withAppendedSegment( "child.txt" ) : source ) );
		}
	}
}

UTEST( ZipProvider, browsingNeverRegistersResourcesAndNoticesArchiveReplacement ) {
	TempDir temp;
	const std::string path = temp.path + "archive.zip";
	ASSERT_TRUE( makeZip( path, { { "zip-provider-only.txt", "old" } } ) );
	const Uint64 generation = PackFileIndex::instance()->getGeneration();
	VirtualFileSystem vfs;
	const URI root = ZipProvider::makeURI( URI::fromFSPath( path ) );
	EXPECT_STDSTREQ( "old",
					 readString( vfs, root.withAppendedSegment( "zip-provider-only.txt" ) ) );
	EXPECT_EQ( generation, PackFileIndex::instance()->getGeneration() );
	EXPECT_FALSE( PackFileIndex::instance()->fileExists( "zip-provider-only.txt" ) );
	auto oldInfo = waitFor( vfs.stat( root.withAppendedSegment( "zip-provider-only.txt" ) ) );
	ASSERT_TRUE( oldInfo.hasValue() );
	// Replace only after handles have closed (portable to Windows, where an open file can
	// prevent replacement). A fresh operation must discard the cached directory index.
	ASSERT_TRUE( FileSystem::fileRemove( path ) );
	ASSERT_TRUE( makeZip( path, { { "new.txt", "new contents" } } ) );
	auto listing = waitFor( vfs.listAll( root ) );
	ASSERT_TRUE( listing.hasValue() );
	EXPECT_TRUE( sortedNames( listing.value() ) == std::vector<std::string>( { "new.txt" } ) );
	EXPECT_STDSTREQ( "new contents", readString( vfs, root.withAppendedSegment( "new.txt" ) ) );
	EXPECT_EQ(
		VFSErrorCode::NotFound,
		codeOf( waitFor( vfs.stat( root.withAppendedSegment( "zip-provider-only.txt" ) ) ) ) );
}

UTEST( Zip, standaloneReaderIsReadOnlyAndHasBoundedSeek ) {
	TempDir temp;
	const auto path = temp.path + "archive.zip";
	const std::string content( 200000, 'a' );
	ASSERT_TRUE( makeZip( path, { { "large", content } } ) );
	std::string original;
	ASSERT_TRUE( FileSystem::fileGet( path, original ) );
	{
		Zip reader( false );
		ASSERT_TRUE( reader.open( path, Zip::OpenMode::ReadOnly ) );
		EXPECT_TRUE( reader.isOpen() );
		const std::array<Uint8, 1> byte{ 'x' };
		EXPECT_FALSE( reader.addFile( byte.data(), byte.size(), "forbidden" ) );
		EXPECT_TRUE( reader.error() == Zip::Error::ReadOnly );
		EXPECT_FALSE( reader.eraseFile( "large" ) );
		EXPECT_EQ( 1u, reader.entryCount() );
		Zip::Entry entry;
		ASSERT_TRUE( reader.entry( 0, entry ) );
		EXPECT_EQ( content.size(), entry.size );
		auto stream = reader.openStream( 0 );
		ASSERT_TRUE( stream != nullptr );
		EXPECT_EQ( 199999, stream->seek( 199999 ) );
		char value = 0;
		EXPECT_EQ( 1, stream->read( &value, 1 ) );
		EXPECT_EQ( int( 'a' ), int( value ) );
		EXPECT_EQ( 0, stream->seek( 0 ) );
		EXPECT_EQ( 1, stream->read( &value, 1 ) );
	}
	std::string after;
	ASSERT_TRUE( FileSystem::fileGet( path, after ) );
	EXPECT_TRUE( original == after );
	Zip missing( false );
	EXPECT_FALSE( missing.open( temp.path + "missing.zip", Zip::OpenMode::ReadOnly ) );
	EXPECT_TRUE( missing.error() == Zip::Error::NotFound );
	EXPECT_FALSE( FileSystem::fileExists( temp.path + "missing.zip" ) );
	EXPECT_FALSE( missing.open( temp.path + "missing.zip" ) );
	EXPECT_FALSE( FileSystem::fileExists( temp.path + "missing.zip" ) );
	ASSERT_TRUE( FileSystem::fileWrite( temp.path + "invalid.zip", "not an archive" ) );
	errno = EACCES;
	EXPECT_FALSE( missing.open( temp.path + "invalid.zip", Zip::OpenMode::ReadOnly ) );
	EXPECT_TRUE( missing.error() == Zip::Error::InvalidArchive );
	errno = EACCES;
	EXPECT_FALSE( missing.open( temp.path + "invalid.zip" ) );
	EXPECT_TRUE( missing.error() == Zip::Error::InvalidArchive );
}

UTEST( ZipLocation, zipBasedFormatsUseTheSameProvider ) {
	TempDir temp;
	VirtualFileSystem vfs;
	for ( const char* name : { "archive.zip", "library.jar", "application.APK", "book.epub" } ) {
		const std::string path = temp.path + name;
		ASSERT_TRUE( makeZip( path, { { "entry.txt", "archive content" } } ) );
		const URI file = URI::fromFSPath( path );
		const URI root = eefm::Location::archiveRoot( file );
		EXPECT_TRUE( root == ZipProvider::makeURI( file ) );
		EXPECT_STDSTREQ( "archive content",
						 readString( vfs, root.withAppendedSegment( "entry.txt" ) ) );
	}
	EXPECT_TRUE(
		eefm::Location::archiveRoot( temp.uri().withAppendedSegment( "file.txt" ) ).empty() );
	EXPECT_TRUE( eefm::Location::archiveRoot( URI( "sftp://host/archive.zip" ) ).empty() );
}

UTEST( ZipLocation, readableAddressesRoundTripAndRootReturnsToContainingFolder ) {
	TempDir temp;
	const URI archive = URI::fromFSPath( temp.path + "Mixed # %!?.zip" );
	const URI root = ZipProvider::makeURI( archive );
	const URI member = root.withAppendedSegment( "a # %.txt" );
	EXPECT_TRUE( eefm::Location::parse( eefm::Location::text( root ).toUtf8() ) == root );
	EXPECT_TRUE( eefm::Location::parse( eefm::Location::text( member ).toUtf8() ) == member );
	EXPECT_TRUE( URI( eefm::Location::text( member ).toUtf8() ) == member );
	EXPECT_TRUE( eefm::Location::text( root ).toUtf8().starts_with( "zip:///" ) );
	EXPECT_TRUE( ZipProvider::archiveURI( URI( "zip:///tmp/archive.zip!/docs" ) ).hasValue() );
	EXPECT_TRUE( eefm::Location::parent( root ) == archive.getParent() );
	EXPECT_TRUE( eefm::Location::parent( member ) == root );
	EXPECT_STDSTREQ( archive.getLastPathSegment(), eefm::Location::entryName( root ) );
	EXPECT_STDSTREQ( archive.getLastPathSegment(), eefm::Location::displayName( root ).toUtf8() );
}

UTEST( ZipProvider, unsafeNamesAreExcludedAndCollisionsAreDeterministic ) {
	TempDir temp;
	// Stored ZIP with traversal/absolute/drive names, duplicate entries, and a file/directory
	// collision. Kept as a fixture because Zip::addFile refuses duplicate names.
	const std::string encoded = "UEsDBBQAAAAAADIIUV2PKKQfBAAAAAQAAAAIAAAAc2FmZS50eHRzYWZlUEsDBBQAAA"
								"AAADIIUV37OSuCAwAAAAMAAAAN"
								"AAAALi4vZXNjYXBlLnR4dGJhZFBLAwQUAAAAAAAyCFFd+"
								"zkrggMAAAADAAAADQAAAC9hYnNvbHV0ZS50eHRiYWRQSwME"
								"FAAAAAAAMghRXfs5K4IDAAAAAwAAABQAAABkaXIvLi4vLi4vZXNjYXBlLnR4dGJhZF"
								"BLAwQUAAAAAAAyCFFd+zkrggMA"
								"AAADAAAACwAAAEM6ZHJpdmUudHh0YmFkUEsDBBQAAAAAADIIUV37OSuCAwAAAAMAAA"
								"AOAAAAZGlyXGVzY2FwZS50eHRi"
								"YWRQSwMEFAAAAAAAMghRXfs5K4IDAAAAAwAAAA4AAAAuLiAvZXNjYXBlLnR4dGJhZF"
								"BLAwQUAAAAAAAyCFFdAAAAAAAA"
								"AAAAAAAABgAAAGVtcHR5L1BLAwQUAAAAAAAyCFFdEDafjAQAAAAEAAAACQAAAGNvbG"
								"xpc2lvbmZpbGVQSwMEFAAAAAAA"
								"MghRXSlUsyIFAAAABQAAAA8AAABjb2xsaXNpb24vY2hpbGRjaGlsZFBLAwQUAAAAAA"
								"AyCFFdV+5xkgUAAAAFAAAACQAA"
								"AGR1cGxpY2F0ZWZpcnN0UEsDBBQAAAAAADIIUV1pER+"
								"2BgAAAAYAAAAJAAAAZHVwbGljYXRlc2Vjb25kUEsBAhQDFAAA"
								"AAAAMghRXY8opB8EAAAABAAAAAgAAAAAAAAAAAAAAIABAAAAAHNhZmUudHh0UEsBAh"
								"QDFAAAAAAAMghRXfs5K4IDAAAA"
								"AwAAAA0AAAAAAAAAAAAAAIABKgAAAC4uL2VzY2FwZS50eHRQSwECFAMUAAAAAAAyCF"
								"Fd+zkrggMAAAADAAAADQAAAAAA"
								"AAAAAAAAgAFYAAAAL2Fic29sdXRlLnR4dFBLAQIUAxQAAAAAADIIUV37OSuCAwAAAA"
								"MAAAAUAAAAAAAAAAAAAACAAYYA"
								"AABkaXIvLi4vLi4vZXNjYXBlLnR4dFBLAQIUAxQAAAAAADIIUV37OSuCAwAAAAMAAA"
								"ALAAAAAAAAAAAAAACAAbsAAABD"
								"OmRyaXZlLnR4dFBLAQIUAxQAAAAAADIIUV37OSuCAwAAAAMAAAAOAAAAAAAAAAAAAA"
								"CAAecAAABkaXJcZXNjYXBlLnR4"
								"dFBLAQIUAxQAAAAAADIIUV37OSuCAwAAAAMAAAAOAAAAAAAAAAAAAACAARYBAAAuLi"
								"AvZXNjYXBlLnR4dFBLAQIUAxQA"
								"AAAAADIIUV0AAAAAAAAAAAAAAAAGAAAAAAAAAAAAEAD9QUUBAABlbXB0eS9QSwECFA"
								"MUAAAAAAAyCFFdEDafjAQAAAAE"
								"AAAACQAAAAAAAAAAAAAAgAFpAQAAY29sbGlzaW9uUEsBAhQDFAAAAAAAMghRXSlUsy"
								"IFAAAABQAAAA8AAAAAAAAAAAAA"
								"AIABlAEAAGNvbGxpc2lvbi9jaGlsZFBLAQIUAxQAAAAAADIIUV1X7nGSBQAAAAUAAA"
								"AJAAAAAAAAAAAAAACAAcYBAABk"
								"dXBsaWNhdGVQSwECFAMUAAAAAAAyCFFdaREftgYAAAAGAAAACQAAAAAAAAAAAAAAgA"
								"HyAQAAZHVwbGljYXRlUEsFBgAA"
								"AAAMAAwAtQIAAB8CAAAAAA==";
	std::string bytes;
	ASSERT_NE( size_t( -1 ), Base64::decode( encoded, bytes ) );
	ASSERT_TRUE( FileSystem::fileWrite( temp.path + "unsafe.zip", bytes ) );
	VirtualFileSystem vfs;
	const URI root = ZipProvider::makeURI( URI::fromFSPath( temp.path + "unsafe.zip" ) );
	auto listed = waitFor( vfs.listAll( root ) );
	ASSERT_TRUE( listed.hasValue() );
	EXPECT_TRUE( sortedNames( listed.value() ) ==
				 std::vector<std::string>( { "collision", "duplicate", "empty", "safe.txt" } ) );
	EXPECT_STDSTREQ( "first", readString( vfs, root.withAppendedSegment( "duplicate" ) ) );
	EXPECT_STDSTREQ(
		"child",
		readString( vfs, root.withAppendedSegment( "collision" ).withAppendedSegment( "child" ) ) );
	const auto destination = URI::fromFSPath( temp.path + "out" );
	ASSERT_TRUE( waitFor( vfs.copy( root, destination ).completion() ).hasValue() );
	EXPECT_FALSE( FileSystem::fileExists( temp.path + "escape.txt" ) );
	EXPECT_STDSTREQ( "safe", readString( vfs, destination.withAppendedSegment( "safe.txt" ) ) );
}

UTEST( ZipProvider, crcFailureIsReportedEvenForAnExactSizedRead ) {
	TempDir temp;
	const std::string encoded =
		"UEsDBBQAAAAAADIIUV0Ks4YUDwAAAA8AAAALAAAAY2hlY2tlZC50eHRjaGVja2VkIHBheWxvYWRQSwECFAMUAAAAAA"
		"Ay"
		"CFFdCrOGFA8AAAAPAAAACwAAAAAAAAAAAAAAgAEAAAAAY2hlY2tlZC50eHRQSwUGAAAAAAEAAQA5AAAAOAAAAAAA";
	std::string bytes;
	ASSERT_NE( size_t( -1 ), Base64::decode( encoded, bytes ) );
	const auto payload = bytes.find( "checked payload" );
	ASSERT_NE( std::string::npos, payload );
	bytes[payload] ^= 1;
	ASSERT_TRUE( FileSystem::fileWrite( temp.path + "corrupt.zip", bytes ) );
	VirtualFileSystem vfs;
	const URI root = ZipProvider::makeURI( URI::fromFSPath( temp.path + "corrupt.zip" ) );
	auto opened = waitFor( vfs.open( root.withAppendedSegment( "checked.txt" ) ) );
	ASSERT_TRUE( opened.hasValue() );
	auto handle = std::move( opened ).value();
	std::array<Uint8, 15> data;
	EXPECT_EQ( VFSErrorCode::IO, codeOf( waitFor( handle.read( 0, data ) ) ) );
}

UTEST( ZipProvider, openHandlesSurviveCacheEvictionAndProviderRemoval ) {
	TempDir temp;
	const auto path = temp.path + "original.zip";
	ASSERT_TRUE( makeZip( path, { { "file", "original" }, { "other", "second" } } ) );
	VirtualFileSystem vfs;
	const URI root = ZipProvider::makeURI( URI::fromFSPath( path ) );
	auto first = waitFor( vfs.open( root.withAppendedSegment( "file" ) ) );
	auto second = waitFor( vfs.open( root.withAppendedSegment( "other" ) ) );
	ASSERT_TRUE( first.hasValue() );
	ASSERT_TRUE( second.hasValue() );
	auto a = std::move( first ).value();
	auto b = std::move( second ).value();
	for ( int i = 0; i < 9; ++i ) {
		const auto other = temp.path + String::toString( i ) + ".zip";
		ASSERT_TRUE( makeZip( other, { { "x", "x" } } ) );
		ASSERT_TRUE(
			waitFor( vfs.stat( ZipProvider::makeURI( URI::fromFSPath( other ) ) ) ).hasValue() );
	}
	vfs.unregisterProvider( "zip" );
	std::array<Uint8, 8> left;
	std::array<Uint8, 6> right;
	auto readingA = a.read( 0, left );
	auto readingB = b.read( 0, right );
	ASSERT_TRUE( waitFor( std::move( readingA ) ).hasValue() );
	ASSERT_TRUE( waitFor( std::move( readingB ) ).hasValue() );
	EXPECT_TRUE( std::string( left.begin(), left.end() ) == "original" );
	EXPECT_TRUE( std::string( right.begin(), right.end() ) == "second" );
	EXPECT_TRUE( waitFor( a.close() ).hasValue() );
	EXPECT_TRUE( waitFor( b.close() ).hasValue() );
}

UTEST( ZipProvider, emptyArchivesAndUnicodeBackingFiles ) {
	TempDir temp;
	const std::string empty = std::string( "PK\x05\x06", 4 ) + std::string( 18, '\0' );
	const auto path = temp.path + "empty.zip";
	ASSERT_TRUE( FileSystem::fileWrite( path, empty ) );
	VirtualFileSystem vfs;
	auto listing = waitFor( vfs.listAll( ZipProvider::makeURI( URI::fromFSPath( path ) ) ) );
	ASSERT_TRUE( listing.hasValue() );
	EXPECT_TRUE( listing.value().empty() );
	const auto unicode = temp.path + "archivo-ñ-雪.zip";
	// Build at an ASCII path with the legacy writer, then rename through the portable VFS.
	const auto source = temp.path + "source.zip";
	ASSERT_TRUE( makeZip(
		source, { { "file", "unicode archive" }, { "directorio-ñ/雪.txt", "unicode member" } } ) );
	ASSERT_TRUE(
		waitFor( vfs.rename( URI::fromFSPath( source ), URI::fromFSPath( unicode ) ) ).hasValue() );
	EXPECT_STDSTREQ( "unicode archive",
					 readString( vfs, ZipProvider::makeURI( URI::fromFSPath( unicode ) )
										  .withAppendedSegment( "file" ) ) );
	const URI member = ZipProvider::makeURI( URI::fromFSPath( unicode ), "/directorio-ñ/雪.txt" );
	EXPECT_STDSTREQ( "unicode member", readString( vfs, URI( member.toString() ) ) );
}

UTEST( ZipProvider, pollingWatchNoticesBackingArchiveReplacement ) {
	TempDir temp;
	const auto path = temp.path + "watched.zip";
	ASSERT_TRUE( makeZip( path, { { "before", "old" } } ) );
	VirtualFileSystem vfs;
	const URI root = ZipProvider::makeURI( URI::fromFSPath( path ) );
	WatchOptions options;
	options.pollingInterval = Milliseconds( 50 );
	auto opened = waitFor( vfs.watch( root, options ) );
	ASSERT_TRUE( opened.hasValue() );
	WatchHandle watch( std::move( opened ).value() );
	EXPECT_EQ( WatchStatus::Polling, watch.status() );
	TestLoop loop;
	EventCollector collector;
	watch.onEvents( loop, collector.callback() );
	ASSERT_TRUE( FileSystem::fileRemove( path ) );
	ASSERT_TRUE( makeZip( path, { { "after", "new contents" } } ) );
	EXPECT_TRUE( loop.runUntil( [&] { return collector.has( WatchEventType::Added, "after" ); } ) );
	EXPECT_TRUE( collector.has( WatchEventType::Removed, "before" ) );
	EXPECT_STDSTREQ( "new contents", readString( vfs, root.withAppendedSegment( "after" ) ) );
}
