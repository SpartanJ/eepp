#include <eepp/config.hpp>
#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winioctl.h>
#endif

#include "../../tools/eefm/location.hpp"
#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <array>
#include <cstdio>
#include <eepp/system/base64.hpp>
#include <eepp/system/compression.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/packfileindex.hpp>
#include <eepp/vfs/providers/archiveprovider.hpp>
#include <filesystem>
#include <fstream>

using namespace EE;
using namespace EE::System;
using namespace EE::VFS;
using namespace vfstest;

namespace {

// Independent USTAR fixture writer. GNU/PAX extensions are explicit entries so the tests do
// not depend on a system tar command or on the same reader being used to produce fixtures.
struct TarFixture {
	std::string bytes;

	void entry( std::string_view name, std::string_view data, char type = '0',
				std::string_view link = {}, std::optional<Uint64> headerSize = {} ) {
		std::array<char, 512> header{};
		std::copy_n( name.data(), std::min<size_t>( name.size(), 100 ), header.data() );
		std::snprintf( header.data() + 100, 8, "%07o", type == '5' ? 0755 : 0644 );
		std::snprintf( header.data() + 108, 8, "%07o", 1000 );
		std::snprintf( header.data() + 116, 8, "%07o", 1000 );
		std::snprintf( header.data() + 124, 12, "%011llo",
					   static_cast<unsigned long long>( headerSize.value_or( data.size() ) &
														077777777777ULL ) );
		std::snprintf( header.data() + 136, 12, "%011o", 1700000000 );
		std::fill_n( header.data() + 148, 8, ' ' );
		header[156] = type;
		std::copy_n( link.data(), std::min<size_t>( link.size(), 100 ), header.data() + 157 );
		std::copy_n( "ustar", 5, header.data() + 257 );
		header[263] = header[264] = '0';
		unsigned sum = 0;
		for ( unsigned char byte : header )
			sum += byte;
		std::snprintf( header.data() + 148, 7, "%06o", sum );
		header[155] = ' ';
		bytes.append( header.data(), header.size() );
		bytes.append( data );
		bytes.append( ( 512 - data.size() % 512 ) % 512, '\0' );
	}

	bool write( const std::string& path, bool gzip = false ) const {
		std::string data = bytes + std::string( 1024, '\0' );
		if ( !gzip )
			return FileSystem::fileWrite( path, data );
		IOStreamMemory input( static_cast<const char*>( data.data() ), data.size() );
		IOStreamFile output( path, "wb" );
		return output.isOpen() &&
			   Compression::compress( output, input, Compression::MODE_GZIP ) == Compression::OK;
	}

	static std::string pax( std::string value ) {
		size_t size = value.size() + 3;
		while ( std::to_string( size ).size() + value.size() + 2 != size )
			size = std::to_string( size ).size() + value.size() + 2;
		return std::to_string( size ) + " " + value + "\n";
	}
};

bool available( int* utest_result, VirtualFileSystem& vfs ) {
	auto provider = std::dynamic_pointer_cast<ArchiveProvider>( vfs.provider( "archive" ) );
	if ( !provider ) {
		*utest_result = UTEST_TEST_FAILURE;
		return false;
	}
	auto support = waitFor( provider->support() );
	if ( !support ) {
		if ( !support.error().is( VFSErrorCode::Unsupported ) )
			*utest_result = UTEST_TEST_FAILURE;
		UTEST_PRINT_INFO(
			"libarchive runtime unavailable; integration requires an installed library" );
		if ( *utest_result != UTEST_TEST_FAILURE )
			*utest_result = UTEST_TEST_SKIPPED;
		return false;
	}
	if ( support.value().version.empty() ||
		 std::find( support.value().formats.begin(), support.value().formats.end(), "tar" ) ==
			 support.value().formats.end() ) {
		*utest_result = UTEST_TEST_FAILURE;
		return false;
	}
	return true;
}

} // namespace

UTEST( ArchiveProvider, locationsAndEefmExtensions ) {
	TempDir temp;
	const URI backing = URI::fromFSPath( temp.path + "Archive! #%.tar.gz" );
	const URI root = ArchiveProvider::makeURI( backing );
	const URI member = root.withAppendedSegment( "dir" ).withAppendedSegment( "entry! #%" );
	EXPECT_TRUE( ArchiveProvider::archiveURI( URI( member.toString() ) ).value() == backing );
	EXPECT_STDSTREQ( "/dir/entry! #%", ArchiveProvider::memberPath( member ).value() );
	EXPECT_TRUE( eefm::Location::parse( eefm::Location::text( member ).toUtf8() ) == member );
	EXPECT_TRUE( eefm::Location::parent( root ) == backing.getParent() );
	EXPECT_STDSTREQ( backing.getLastPathSegment(), eefm::Location::entryName( root ) );
	EXPECT_STDSTREQ( backing.getLastPathSegment(), eefm::Location::displayName( root ).toUtf8() );
	EXPECT_TRUE( eefm::Location::parent( member ) == root.withAppendedSegment( "dir" ) );
	for ( const char* name : { "archive.tar", "archive.TAR.GZ", "archive.tgz", "archive.tar.xz",
							   "archive.tar.bz2", "archive.7z", "archive.rar", "package.deb" } ) {
		const URI file = temp.uri().withAppendedSegment( name );
		EXPECT_TRUE( eefm::Location::archiveRoot( file ) == ArchiveProvider::makeURI( file ) );
	}
	EXPECT_TRUE( ArchiveProvider::makeURI( URI( "sftp://host/a.tar" ) ).empty() );
	EXPECT_EQ( VFSErrorCode::InvalidURI,
			   codeOf( ArchiveProvider::archiveURI( URI( "archive:///no-boundary" ) ) ) );
}

UTEST( ArchiveProvider, largeSparseTarUses64BitInputOffsets ) {
	TempDir temp;
	VirtualFileSystem vfs;
	if ( !available( utest_result, vfs ) )
		return;
	const Uint64 largeSize = Uint64( 3 ) * 1024 * 1024 * 1024;
	TarFixture header;
	header.entry( "large", "", '0', {}, largeSize );
	const std::string path = temp.path + "large.tar";
	ASSERT_TRUE( FileSystem::fileWrite( path, header.bytes ) );
#if EE_PLATFORM == EE_PLATFORM_WIN
	// Mark sparse before extending: the test must not allocate 3 GiB on disk.
	HANDLE file = CreateFileW( String::fromUtf8( path ).toWideString().c_str(), GENERIC_WRITE,
							   FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
							   OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr );
	ASSERT_TRUE( file != INVALID_HANDLE_VALUE );
	DWORD returned = 0;
	bool sparse =
		DeviceIoControl( file, FSCTL_SET_SPARSE, nullptr, 0, nullptr, 0, &returned, nullptr ) != 0;
	CloseHandle( file );
	if ( !sparse )
		UTEST_SKIP( "Temporary filesystem does not support sparse files" );
#endif
	TarFixture tail;
	tail.entry( "tail", "payload" );
	{
		const std::filesystem::path nativePath( std::u8string( path.begin(), path.end() ) );
		std::fstream stream( nativePath, std::ios::binary | std::ios::in | std::ios::out );
		stream.seekp( static_cast<std::streamoff>( 512 + largeSize ) );
		const std::string end = tail.bytes + std::string( 1024, '\0' );
		stream.write( end.data(), end.size() );
		ASSERT_TRUE( stream.good() );
	}
	const URI root = ArchiveProvider::makeURI( URI::fromFSPath( path ) );
	auto listed = waitFor( vfs.listAll( root ) );
	ASSERT_TRUE( listed.hasValue() );
	EXPECT_EQ( 2u, listed.value().size() );
	EXPECT_STDSTREQ( "payload", readString( vfs, root.withAppendedSegment( "tail" ) ) );
	EXPECT_EQ(
		largeSize,
		waitFor( vfs.stat( root.withAppendedSegment( "large" ) ) ).value().size().value_or( 0 ) );
}

UTEST( ArchiveProvider, missingLibraryIsOptionalAndReadOnly ) {
	auto pool = ThreadPool::createShared( 1 );
	auto scheduler = std::make_shared<LocalIOScheduler>( pool, Seconds( 1 ) );
	TempDir temp;
	ArchiveProvider provider( scheduler, temp.path + "missing-libarchive" );
	EXPECT_EQ( VFSErrorCode::Unsupported, codeOf( waitFor( provider.support() ) ) );
	const URI file =
		ArchiveProvider::makeURI( URI::fromFSPath( temp.path + "archive.tar" ), "/file" );
	EXPECT_EQ( VFSErrorCode::Unsupported, codeOf( waitFor( provider.stat( file, {}, {} ) ) ) );
	EXPECT_EQ( VFSErrorCode::ReadOnly,
			   codeOf( waitFor( provider.open( file, OpenFlags::Write, {} ) ) ) );
	EXPECT_EQ( VFSErrorCode::ReadOnly, codeOf( waitFor( provider.remove( file, {}, {} ) ) ) );
	CancellationSource cancelled;
	cancelled.cancel();
	EXPECT_EQ( VFSErrorCode::Cancelled,
			   codeOf( waitFor( provider.support( cancelled.token() ) ) ) );
	EXPECT_EQ( VFSErrorCode::Cancelled,
			   codeOf( waitFor( provider.stat( file, {}, cancelled.token() ) ) ) );
}

UTEST( ArchiveProvider, tarAndGzipListStreamSeekAndCopy ) {
	TempDir temp;
	VirtualFileSystem vfs;
	if ( !available( utest_result, vfs ) )
		return;
	TarFixture fixture;
	std::string content( 200000, 'x' );
	for ( size_t i = 0; i < content.size(); ++i )
		content[i] = static_cast<char>( 'a' + i % 26 );
	fixture.entry( "./dir/file.txt", content );
	fixture.entry( "empty/", "", '5' );
	fixture.entry( "zero", "" );
	const auto generation = PackFileIndex::instance()->getGeneration();
	for ( bool gzip : { false, true } ) {
		const std::string path = temp.path + ( gzip ? "archivo-ñ-雪.tar.gz" : "archive.tar" );
		ASSERT_TRUE( fixture.write( path, gzip ) );
		const URI root = ArchiveProvider::makeURI( URI::fromFSPath( path ) );
		auto list = waitFor( vfs.listAll( root ) );
		ASSERT_TRUE( list.hasValue() );
		EXPECT_TRUE( sortedNames( list.value() ) ==
					 std::vector<std::string>( { "dir", "empty", "zero" } ) );
		EXPECT_TRUE(
			waitFor( vfs.listAll( root.withAppendedSegment( "empty" ) ) ).value().empty() );
		const URI file = root.withAppendedSegment( "dir" ).withAppendedSegment( "file.txt" );
		auto opened = waitFor( vfs.open( file ) );
		ASSERT_TRUE( opened.hasValue() );
		auto handle = std::move( opened ).value();
		std::array<Uint8, 32> bytes{};
		for ( Uint64 offset : { 150000u, 0u, 199990u, 5u } ) {
			auto got = waitFor( handle.read( offset, bytes ) );
			ASSERT_TRUE( got.hasValue() );
			EXPECT_EQ( std::min<size_t>( bytes.size(), content.size() - offset ), got.value() );
			EXPECT_TRUE( std::string( bytes.begin(), bytes.begin() + got.value() ) ==
						 content.substr( offset, got.value() ) );
		}
		EXPECT_FALSE( handle.capabilities().has( VFSCapability::RandomAccess ) );
		EXPECT_TRUE( waitFor( handle.close() ).hasValue() );
		EXPECT_STDSTREQ( "", readString( vfs, root.withAppendedSegment( "zero" ) ) );
		const URI destination = URI::fromFSPath( temp.path + ( gzip ? "gzip-copy" : "tar-copy" ) );
		EXPECT_TRUE(
			waitFor( vfs.copy( root.withAppendedSegment( "dir" ), destination ).completion() )
				.hasValue() );
		EXPECT_TRUE( content == readString( vfs, destination.withAppendedSegment( "file.txt" ) ) );
		const URI refused = destination.withAppendedSegment( "must-not-exist" );
		EXPECT_EQ( VFSErrorCode::Unsupported,
				   codeOf( waitFor( vfs.move( file, refused ).completion() ) ) );
		EXPECT_EQ( VFSErrorCode::NotFound, codeOf( waitFor( vfs.stat( refused ) ) ) );
	}
	EXPECT_EQ( generation, PackFileIndex::instance()->getGeneration() );
}

UTEST( ArchiveProvider, extractRootIntoExistingFolder ) {
	TempDir temp;
	VirtualFileSystem vfs;
	if ( !available( utest_result, vfs ) )
		return;
	TarFixture tar;
	tar.entry( ".hidden", "hidden" );
	tar.entry( "sub/file.txt", "contents" );
	tar.entry( "empty/", "", '5' );
	ASSERT_TRUE( tar.write( temp.path + "archive.tar.gz", true ) );
	const URI root =
		eefm::Location::archiveRoot( temp.uri().withAppendedSegment( "archive.tar.gz" ) );
	CopyOptions options;
	options.overwrite = OverwritePolicy::MergeDirectories;
	auto copied = waitFor( vfs.copy( root, temp.uri(), options ).completion() );
	ASSERT_TRUE( copied );
	EXPECT_FALSE( copied.value().partial() );
	EXPECT_EQ( 2u, copied.value().filesTransferred );
	EXPECT_STDSTREQ( "hidden", readString( vfs, temp.uri().withAppendedSegment( ".hidden" ) ) );
	EXPECT_STDSTREQ( "contents",
					 readString( vfs, temp.uri().withAppendedSegment( "sub" ).withAppendedSegment(
										  "file.txt" ) ) );
	auto empty = waitFor( vfs.stat( temp.uri().withAppendedSegment( "empty" ) ) );
	ASSERT_TRUE( empty );
	EXPECT_TRUE( empty.value().isDirectory() );
	EXPECT_STDSTREQ( "hidden", readString( vfs, root.withAppendedSegment( ".hidden" ) ) );
}

UTEST( ArchiveProvider, concurrentReadersAndSequentialDirectoryCopy ) {
	TempDir temp;
	VirtualFileSystem vfs;
	if ( !available( utest_result, vfs ) )
		return;
	TarFixture fixture;
	for ( int i = 0; i < 64; ++i )
		fixture.entry( "dir/file-" + std::to_string( i ), "contents " + std::to_string( i ) );
	const std::string path = temp.path + "sequential.tar.gz";
	ASSERT_TRUE( fixture.write( path, true ) );
	const URI root = ArchiveProvider::makeURI( URI::fromFSPath( path ) );
	const URI directory = root.withAppendedSegment( "dir" );
	auto first = waitFor( vfs.open( directory.withAppendedSegment( "file-0" ) ) );
	auto second = waitFor( vfs.open( directory.withAppendedSegment( "file-1" ) ) );
	ASSERT_TRUE( first.hasValue() );
	ASSERT_TRUE( second.hasValue() );
	auto a = std::move( first ).value();
	auto b = std::move( second ).value();
	std::array<Uint8, 4> bytes{};
	EXPECT_EQ( 4u, waitFor( a.read( 2, bytes ) ).value() );
	EXPECT_TRUE( waitFor( b.close() ).hasValue() );
	// Closing one reader must not change another's position or its backward-seek behavior.
	EXPECT_EQ( 4u, waitFor( a.read( 0, bytes ) ).value() );
	EXPECT_TRUE( std::string( bytes.begin(), bytes.end() ) == "cont" );
	EXPECT_TRUE( waitFor( a.close() ).hasValue() );
	const URI output = temp.uri().withAppendedSegment( "copied" );
	auto copied = waitFor( vfs.copy( directory, output ).completion() );
	ASSERT_TRUE( copied.hasValue() );
	EXPECT_EQ( 64u, copied.value().filesTransferred );
	for ( int i = 0; i < 64; ++i ) {
		EXPECT_STDSTREQ(
			"contents " + std::to_string( i ),
			readString( vfs, output.withAppendedSegment( "file-" + std::to_string( i ) ) ) );
	}
}

UTEST( ArchiveProvider, gnuAndPaxNamesAndSizeOverrides ) {
	TempDir temp;
	VirtualFileSystem vfs;
	if ( !available( utest_result, vfs ) )
		return;
	TarFixture fixture;
	const std::string longName = "gnu/" + std::string( 180, 'n' );
	fixture.entry( "././@LongLink", longName + '\0', 'L' );
	fixture.entry( "placeholder", "gnu content" );
	const std::string paxName = "pax/" + std::string( 180, 'p' );
	fixture.entry( "PaxHeader", TarFixture::pax( "path=" + paxName ) + TarFixture::pax( "size=11" ),
				   'x' );
	fixture.entry( "placeholder", "pax content", '0', {}, 0 );
	fixture.entry( "tail", "tail content" );
	const auto path = temp.path + "extensions.tar.gz";
	ASSERT_TRUE( fixture.write( path, true ) );
	const URI root = ArchiveProvider::makeURI( URI::fromFSPath( path ) );
	EXPECT_STDSTREQ( "gnu content", readString( vfs, ArchiveProvider::makeURI(
														 URI::fromFSPath( path ), longName ) ) );
	EXPECT_STDSTREQ( "pax content", readString( vfs, ArchiveProvider::makeURI(
														 URI::fromFSPath( path ), paxName ) ) );
	EXPECT_STDSTREQ( "tail content", readString( vfs, root.withAppendedSegment( "tail" ) ) );
}

UTEST( ArchiveProvider, runtimeFiltersAndSevenZip ) {
	TempDir temp;
	VirtualFileSystem vfs;
	if ( !available( utest_result, vfs ) )
		return;
	auto provider = std::dynamic_pointer_cast<ArchiveProvider>( vfs.provider( "archive" ) );
	auto support = waitFor( provider->support() );
	ASSERT_TRUE( support.hasValue() );
	struct Fixture {
		const char* name;
		const char* filter;
		const char* format;
		const char* data;
	};
	// Generated independently using Python tarfile/bz2/lzma and libarchive's 7zip writer.
	for ( const auto& fixture :
		  { Fixture{ "archive.tar.bz2", "bzip2", "tar",
					 "QlpoOTFBWSZTWRGkBHYAAG97gMmAAAhAAEWAAARnJN4gCAggAFQ0jUxDATR6maCSKaGmg0GTQfTiO"
					 "hBOCEIlVsTWUcECGBjRxOwjZ0UMctBnNo9/TCY+TXCvVVOZEQGxdyRThQkBGkBHYA==" },
			Fixture{ "archive.tar.xz", "xz", "tar",
					 "/Td6WFoAAATm1rRGAgAhARYAAAB0L+Wj4Cf/AF1dADMaSda2OCGvAe5uBXM7wPyAzzewSHXXbqSo/"
					 "hdgBa5T3GsxIACRvOjdMLsxnrOfIgBYUuJzInp0sdefWq+cGCAurmuD6EUsI9irRPY0Dg3L73/"
					 "zP4rTIqzcpMEqAAAAAAB5/rNswT5g4QABeYBQAAAA5DeTlbHEZ/sCAAAAAARZWg==" },
			Fixture{ "archive.7z", "none", "7zip",
					 "N3q8ryccAAP0BzRLBwAAAAAAAAA0AAAAAAAAAF5/"
					 "nFVwYXlsb2FkAQQGAAEJBwAHCwEAAQEADAcACAoBFWosQgAABQERCwBmAGkAbABlAAAAFQYBACGAA"
					 "IAAAA==" } } ) {
		if ( std::find( support.value().filters.begin(), support.value().filters.end(),
						fixture.filter ) == support.value().filters.end() ||
			 std::find( support.value().formats.begin(), support.value().formats.end(),
						fixture.format ) == support.value().formats.end() )
			continue;
		const std::string path = temp.path + fixture.name;
		std::string bytes;
		Base64::decode( fixture.data, bytes );
		ASSERT_TRUE( FileSystem::fileWrite( path, bytes ) );
		EXPECT_STDSTREQ( "payload", readString( vfs, ArchiveProvider::makeURI(
														 URI::fromFSPath( path ), "/file" ) ) );
	}
}

UTEST( ArchiveProvider, recognizedFormatCanHaveAnUnavailableCodec ) {
	TempDir temp;
	VirtualFileSystem vfs;
	if ( !available( utest_result, vfs ) )
		return;
	auto provider = std::dynamic_pointer_cast<ArchiveProvider>( vfs.provider( "archive" ) );
	auto support = waitFor( provider->support() );
	ASSERT_TRUE( support.hasValue() );
	if ( std::find( support.value().formats.begin(), support.value().formats.end(), "7zip" ) ==
		 support.value().formats.end() )
		UTEST_SKIP( "7zip reader not available" );
	std::string bytes;
	Base64::decode(
		"N3q8ryccAANoCeHQEQAAAAAAAAA8AAAAAAAAAMgYqcgAOBhLmXUPRgxO4E///"
		"d8QAAEEBgABCREABwsBAAEjAwEBBV0AAIAADAcACAoBFWosQgAABQERCwBmAGkAbABlAAAAFQYBACGAAIAAAA==",
		bytes );
	const std::string path = temp.path + "lzma.7z";
	ASSERT_TRUE( FileSystem::fileWrite( path, bytes ) );
	auto result = readFile( vfs, ArchiveProvider::makeURI( URI::fromFSPath( path ), "/file" ) );
	if ( std::find( support.value().filters.begin(), support.value().filters.end(), "lzma" ) ==
		 support.value().filters.end() ) {
		EXPECT_TRUE( result.hasError() );
	} else {
		ASSERT_TRUE( result.hasValue() );
		EXPECT_STDSTREQ( "payload", std::string( result.value().begin(), result.value().end() ) );
	}
}

UTEST( ArchiveProvider, aggregateMetadataIsBounded ) {
	TempDir temp;
	VirtualFileSystem vfs;
	if ( !available( utest_result, vfs ) )
		return;
	TarFixture fixture;
	std::string nested;
	for ( int i = 0; i < 100; ++i )
		nested += "directory-name-long-enough-for-budget/";
	for ( int i = 0; i < 512; ++i ) {
		fixture.entry( "././@LongLink", std::to_string( i ) + "/" + nested + "file" + '\0', 'L' );
		fixture.entry( "file", "" );
	}
	const std::string path = temp.path + "metadata.tar";
	ASSERT_TRUE( fixture.write( path ) );
	EXPECT_EQ(
		VFSErrorCode::InvalidOperation,
		codeOf( waitFor( vfs.listAll( ArchiveProvider::makeURI( URI::fromFSPath( path ) ) ) ) ) );
}

UTEST( ArchiveProvider, unsafePathsLinksAndCollisionsStayInsideArchive ) {
	TempDir temp;
	VirtualFileSystem vfs;
	if ( !available( utest_result, vfs ) )
		return;
	TarFixture fixture;
	fixture.entry( "dir/file", "inside" );
	fixture.entry( "dir/file", "duplicate" );
	fixture.entry( "collision", "hidden" );
	fixture.entry( "collision/child", "child" );
	fixture.entry( "../evil", "evil" );
	fixture.entry( "/absolute", "evil" );
	fixture.entry( "C:drive", "evil" );
	fixture.entry( "back\\slash", "evil" );
	fixture.entry( "file-link", "", '2', "dir/file" );
	fixture.entry( ".hidden-link", "", '2', "dir/file" );
	fixture.entry( "dir-link", "", '2', "dir" );
	fixture.entry( "hard-link", "", '1', "dir/file" );
	fixture.entry( "cycle", "", '2', "cycle" );
	fixture.entry( "outside", "", '2', "../../secret" );
	fixture.entry( "pipe", "", '6' );
	const auto path = temp.path + "links.tar";
	ASSERT_TRUE( fixture.write( path ) );
	const URI root = ArchiveProvider::makeURI( URI::fromFSPath( path ) );
	auto listed = waitFor( vfs.listAll( root ) );
	ASSERT_TRUE( listed.hasValue() );
	EXPECT_EQ( 9u, listed.value().size() );
	const URI hidden = root.withAppendedSegment( ".hidden-link" );
	auto hiddenInfo = waitFor( vfs.stat( hidden ) );
	ASSERT_TRUE( hiddenInfo.hasValue() );
	EXPECT_TRUE( hiddenInfo.value().hidden );
	EXPECT_STDSTREQ( ".hidden-link", hiddenInfo.value().name );
	EXPECT_TRUE( waitFor( vfs.canonicalize( hidden ) ).value() ==
				 root.withAppendedSegment( "dir" ).withAppendedSegment( "file" ) );
	EXPECT_STDSTREQ( "inside", readString( vfs, root.withAppendedSegment( "file-link" ) ) );
	EXPECT_STDSTREQ( "inside", readString( vfs, root.withAppendedSegment( "hard-link" ) ) );
	EXPECT_STDSTREQ(
		"inside",
		readString( vfs, root.withAppendedSegment( "dir-link" ).withAppendedSegment( "file" ) ) );
	EXPECT_STDSTREQ(
		"child",
		readString( vfs, root.withAppendedSegment( "collision" ).withAppendedSegment( "child" ) ) );
	EXPECT_EQ( VFSErrorCode::TooManySymlinks,
			   codeOf( waitFor( vfs.open( root.withAppendedSegment( "cycle" ) ) ) ) );
	EXPECT_EQ( VFSErrorCode::NotFound,
			   codeOf( waitFor( vfs.open( root.withAppendedSegment( "outside" ) ) ) ) );
	EXPECT_EQ( VFSErrorCode::Unsupported,
			   codeOf( waitFor( vfs.open( root.withAppendedSegment( "pipe" ) ) ) ) );
	EXPECT_TRUE(
		waitFor( vfs.stat( root.withAppendedSegment( "file-link" ), StatOptions{ false } ) )
			.value()
			.isSymlink() );
}

UTEST( ArchiveProvider, missingInvalidCorruptAndTruncatedArchivesReportErrors ) {
	TempDir temp;
	VirtualFileSystem vfs;
	if ( !available( utest_result, vfs ) )
		return;
	EXPECT_EQ( VFSErrorCode::NotFound, codeOf( waitFor( vfs.stat( ArchiveProvider::makeURI(
										   URI::fromFSPath( temp.path + "missing.tar" ) ) ) ) ) );
	ASSERT_TRUE( FileSystem::fileWrite( temp.path + "invalid.tar", "not an archive" ) );
	EXPECT_TRUE( waitFor( vfs.listAll( ArchiveProvider::makeURI(
							  URI::fromFSPath( temp.path + "invalid.tar" ) ) ) )
					 .hasError() );
	TarFixture fixture;
	fixture.entry( "file", "contents" );
	fixture.bytes[10] ^= 1;
	ASSERT_TRUE( fixture.write( temp.path + "checksum.tar" ) );
	EXPECT_TRUE( waitFor( vfs.listAll( ArchiveProvider::makeURI(
							  URI::fromFSPath( temp.path + "checksum.tar" ) ) ) )
					 .hasError() );
	TarFixture truncated;
	truncated.entry( "file", "short", '0', {}, 65536 );
	ASSERT_TRUE( truncated.write( temp.path + "truncated.tar" ) );
	EXPECT_TRUE( waitFor( vfs.listAll( ArchiveProvider::makeURI(
							  URI::fromFSPath( temp.path + "truncated.tar" ) ) ) )
					 .hasError() );
}

UTEST( ArchiveProvider, truncatedGzipIsReported ) {
	TempDir temp;
	VirtualFileSystem vfs;
	if ( !available( utest_result, vfs ) )
		return;
	TarFixture fixture;
	fixture.entry( "file", "contents" );
	const std::string path = temp.path + "corrupt.tar.gz";
	ASSERT_TRUE( fixture.write( path, true ) );
	std::string bytes;
	ASSERT_TRUE( FileSystem::fileGet( path, bytes ) );
	ASSERT_GT( bytes.size(), 8u );
	const URI file = ArchiveProvider::makeURI( URI::fromFSPath( path ), "/file" );
	bytes.resize( bytes.size() - 6 );
	ASSERT_TRUE( FileSystem::fileWrite( path, bytes ) );
	EXPECT_TRUE( readFile( vfs, file ).hasError() );
}

UTEST( ArchiveProvider, replacementCacheEvictionAndReaderLifetime ) {
	TempDir temp;
	VirtualFileSystem vfs;
	if ( !available( utest_result, vfs ) )
		return;
	TarFixture fixture;
	fixture.entry( "file", "original" );
	const auto path = temp.path + "original.tar";
	ASSERT_TRUE( fixture.write( path ) );
	const URI root = ArchiveProvider::makeURI( URI::fromFSPath( path ) );
	auto opened = waitFor( vfs.open( root.withAppendedSegment( "file" ) ) );
	ASSERT_TRUE( opened.hasValue() );
	auto handle = std::move( opened ).value();
	for ( int i = 0; i < 10; ++i ) {
		const auto other = temp.path + std::to_string( i ) + ".tar";
		ASSERT_TRUE( fixture.write( other ) );
		ASSERT_TRUE( waitFor( vfs.listAll( ArchiveProvider::makeURI( URI::fromFSPath( other ) ) ) )
						 .hasValue() );
	}
	vfs.unregisterProvider( "archive" );
	std::array<Uint8, 8> bytes{};
	auto read = waitFor( handle.read( 0, bytes ) );
	ASSERT_TRUE( read.hasValue() );
	EXPECT_TRUE( std::string( bytes.begin(), bytes.end() ) == "original" );
	EXPECT_TRUE( waitFor( handle.close() ).hasValue() );

	// Invalidate an index after replacement, including same-length changes.
	VirtualFileSystem next;
	EXPECT_STDSTREQ( "original", readString( next, root.withAppendedSegment( "file" ) ) );
	ASSERT_TRUE( FileSystem::fileRemove( path ) );
	TarFixture replacement;
	replacement.entry( "file", "replaced" );
	ASSERT_TRUE( replacement.write( path ) );
	EXPECT_STDSTREQ( "replaced", readString( next, root.withAppendedSegment( "file" ) ) );
}
