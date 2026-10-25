#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <eepp/system/zip.hpp>
#include <initializer_list>

using namespace EE;
using namespace EE::System;

namespace {

void put( std::string& bytes, size_t offset, Uint64 value, size_t width ) {
	for ( size_t i = 0; i < width; ++i )
		bytes[offset + i] = static_cast<char>( value >> ( i * 8 ) );
}

// A ZIP64 end record at offset 0 followed by its locator and a classic end record whose
// fields defer to it. Only the declared directory range and entry count vary.
std::string zip64EndRecords( Uint64 offset, Uint64 size, Uint64 count ) {
	std::string bytes( 56 + 20 + 22, '\0' );
	put( bytes, 0, 0x06064b50, 4 );
	put( bytes, 4, 44, 8 );
	put( bytes, 12, 45, 2 );
	put( bytes, 14, 45, 2 );
	put( bytes, 24, count, 8 );
	put( bytes, 32, count, 8 );
	put( bytes, 40, size, 8 );
	put( bytes, 48, offset, 8 );
	put( bytes, 56, 0x07064b50, 4 );
	put( bytes, 64, 0, 8 );
	put( bytes, 72, 1, 4 );
	put( bytes, 76, 0x06054b50, 4 );
	put( bytes, 84, 0xffff, 2 );
	put( bytes, 86, 0xffff, 2 );
	put( bytes, 88, 0xffffffff, 4 );
	put( bytes, 92, 0xffffffff, 4 );
	return bytes;
}

} // namespace

UTEST( ZipSecurity, WrappedDirectoryRangesAreRejectedInEveryOpenMode ) {
	vfstest::TempDir temp;
	const std::string path = temp.path + "wrapped.zip";
	// High-bit offsets and sizes also cover sign handling of 32-bit fields.
	for ( auto range : { std::pair<Uint32, Uint32>{ 0xfffffff0u, 0x30u },
						 { 0x80000000u, 0x80000030u },
						 { 23u, 46u },
						 { 0u, 45u } } ) {
		std::string bytes( 22, '\0' );
		put( bytes, 0, 0x06054b50, 4 );
		put( bytes, 8, 1, 2 );
		put( bytes, 10, 1, 2 );
		put( bytes, 12, range.second, 4 );
		put( bytes, 16, range.first, 4 );
		ASSERT_TRUE( FileSystem::fileWrite( path, std::string_view( bytes ) ) );
		Zip archive( false );
		for ( auto mode : { Zip::OpenMode::ReadOnly, Zip::OpenMode::ReadWrite } ) {
			EXPECT_FALSE( archive.open( path, mode ) );
			EXPECT_EQ( archive.error(), Zip::Error::InvalidArchive );
		}
	}
}

UTEST( ZipSecurity, Zip64RangesAndCountsCannotWrapOrOverAllocate ) {
	vfstest::TempDir temp;
	const std::string path = temp.path + "zip64.zip";
	struct Case {
		Uint64 offset;
		Uint64 size;
		Uint64 count;
	};
	// 64-bit offset + size wraps, offsets past the signed seek range, entry counts that wrap
	// a 32-bit allocation size, and a directory that cannot fit before the end record.
	for ( const Case& value :
		  { Case{ 0xffffffffffffff00ull, 0x200, 1 }, Case{ 0x8000000000000000ull, 0x10, 0 },
			Case{ 0, 46, 0x100000000ull }, Case{ 0, 46, 0xffffffffull },
			Case{ 0, 0xffffffffffffffffull, 1 } } ) {
		const std::string bytes = zip64EndRecords( value.offset, value.size, value.count );
		ASSERT_TRUE( FileSystem::fileWrite( path, std::string_view( bytes ) ) );
		Zip archive( false );
		for ( auto mode : { Zip::OpenMode::ReadOnly, Zip::OpenMode::ReadWrite } ) {
			EXPECT_FALSE( archive.open( path, mode ) );
			EXPECT_NE( archive.error(), Zip::Error::None );
			EXPECT_EQ( archive.entryCount(), 0u );
		}
	}
}
