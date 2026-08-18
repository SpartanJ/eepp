#include "utest.hpp"

#include <chrono>
#include <eepp/system/fileinfo.hpp>
#include <eepp/system/filesystem.hpp>
#include <filesystem>

using namespace EE;
using namespace EE::System;

namespace {

class FileInfoIdentity : public FileInfo {
  public:
	void setIdentity( Uint64 device, Uint64 inode ) {
		mDevice = device;
		mInode = inode;
	}
};

struct TempDirectory {
	TempDirectory() {
		path = std::filesystem::temp_directory_path() /
			   ( "eepp-fileinfo-" +
				 std::to_string( std::chrono::steady_clock::now().time_since_epoch().count() ) );
		std::filesystem::create_directories( path );
	}

	~TempDirectory() { FileSystem::dirRemoveAll( path.string() ); }

	std::filesystem::path path;
};

} // namespace

UTEST( FileSystem, dirRemoveAllHandlesTrailingSlash ) {
	TempDirectory temp;
	const std::filesystem::path nested = temp.path / "nested";
	std::filesystem::create_directories( nested );
	ASSERT_TRUE( FileSystem::fileWrite( ( nested / "file.txt" ).string(), "contents" ) );

	std::string path( temp.path.string() );
	FileSystem::dirAddSlashAtEnd( path );
	EXPECT_TRUE( FileSystem::dirRemoveAll( path ) );
	EXPECT_FALSE( std::filesystem::exists( temp.path ) );
	EXPECT_TRUE( FileSystem::dirRemoveAll( path ) );
}

UTEST( FileInfo, sameInodeUsesDeviceAndRejectsInvalidIdentity ) {
	FileInfoIdentity first;
	FileInfoIdentity same;
	FileInfoIdentity otherDevice;
	FileInfoIdentity invalid;

	first.setIdentity( 1, 42 );
	same.setIdentity( 1, 42 );
	otherDevice.setIdentity( 2, 42 );
	invalid.setIdentity( 1, 0 );

	EXPECT_TRUE( FileInfo::inodeSupported() );
	EXPECT_TRUE( first.sameInode( same ) );
	EXPECT_FALSE( first.sameInode( otherDevice ) );
	EXPECT_FALSE( invalid.sameInode( invalid ) );
}

UTEST( FileInfo, reportsHardLinkIdentityAndLinkCount ) {
	TempDirectory temp;
	const std::filesystem::path original = temp.path / "original.txt";
	const std::filesystem::path hardLink = temp.path / "hard-link.txt";

	ASSERT_TRUE( FileSystem::fileWrite( original.string(), "contents" ) );
	std::error_code error;
	std::filesystem::create_hard_link( original, hardLink, error );
	if ( error ) {
		const std::string message = "hard links are unavailable: " + error.message();
		UTEST_SKIP( message.c_str() );
	}

	FileInfo originalInfo( original.string() );
	FileInfo hardLinkInfo( hardLink.string() );

	EXPECT_TRUE( originalInfo.sameInode( hardLinkInfo ) );
	EXPECT_EQ( originalInfo.getDevice(), hardLinkInfo.getDevice() );
	EXPECT_EQ( originalInfo.getInode(), hardLinkInfo.getInode() );
	EXPECT_TRUE( originalInfo.getLinkCount() >= 2 );
	EXPECT_TRUE( hardLinkInfo.getLinkCount() >= 2 );
}
