#include <eepp/core/string.hpp>
#include <eepp/system/fileinfo.hpp>
#include <eepp/system/filesystem.hpp>

#ifndef _DARWIN_FEATURE_64_BIT_INODE
#define _DARWIN_FEATURE_64_BIT_INODE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#endif

#include <sys/stat.h>

#include <limits.h>
#include <stdlib.h>

#ifdef EE_COMPILER_MSVC
#ifndef S_ISDIR
#define S_ISDIR( f ) ( (f)&_S_IFDIR )
#endif

#ifndef S_ISREG
#define S_ISREG( f ) ( (f)&_S_IFREG )
#endif

#ifndef S_ISRDBL
#define S_ISRDBL( f ) ( (f)&_S_IREAD )
#endif

#else
#include <unistd.h>

#ifndef S_ISRDBL
#define S_ISRDBL( f ) ( (f)&S_IRUSR )
#endif
#endif

#ifndef S_IEXEC
#define S_IEXEC 0100
#endif

namespace EE { namespace System {

bool FileInfo::exists( const std::string& filePath ) {
	FileInfo fi( filePath );
	return fi.exists();
}

bool FileInfo::isLink( const std::string& filePath ) {
	FileInfo fi( filePath, true );
	return fi.isLink();
}

bool FileInfo::inodeSupported() {
#if EE_PLATFORM != EE_PLATFORM_WIN
	return true;
#else
	return false;
#endif
}

FileInfo::FileInfo() :
	mModificationTime( 0 ), mOwnerId( 0 ), mGroupId( 0 ), mPermissions( 0 ), mInode( 0 ) {}

FileInfo::FileInfo( const std::string& filepath ) :
	mFilepath( filepath ),
	mFileName( FileSystem::fileNameFromPath( filepath ) ),
	mModificationTime( 0 ),
	mOwnerId( 0 ),
	mGroupId( 0 ),
	mPermissions( 0 ),
	mInode( 0 ) {
	getInfo();
}

FileInfo::FileInfo( const std::string& filepath, bool linkInfo ) :
	mFilepath( filepath ),
	mFileName( FileSystem::fileNameFromPath( filepath ) ),
	mModificationTime( 0 ),
	mOwnerId( 0 ),
	mGroupId( 0 ),
	mPermissions( 0 ),
	mInode( 0 ) {
	if ( linkInfo ) {
		getRealInfo();
	} else {
		getInfo();
	}
}

FileInfo::FileInfo( const FileInfo& other ) :
	mFilepath( other.mFilepath ),
	mFileName( other.mFileName ),
	mModificationTime( other.mModificationTime ),
	mSize( other.mSize ),
	mOwnerId( other.mOwnerId ),
	mGroupId( other.mGroupId ),
	mPermissions( other.mPermissions ),
	mInode( other.mInode ) {}

void FileInfo::getInfo() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( mFilepath.size() == 3 && mFilepath[1] == ':' &&
		 mFilepath[2] == FileSystem::getOSSlash() ) {
		mFilepath += FileSystem::getOSSlash();
	}
#endif

	/// Why i'm doing this? stat in mingw32 doesn't work for directories if the dir path ends with a
	/// path slash
	FileSystem::dirRemoveSlashAtEnd( mFilepath );

#if EE_PLATFORM != EE_PLATFORM_WIN
	struct stat st;
	int res = stat( mFilepath.c_str(), &st );
#else
	struct _stat st;
	int res = _wstat( String::fromUtf8( mFilepath ).toWideString().c_str(), &st );
#endif

	if ( 0 == res ) {
		mModificationTime = st.st_mtime;
		mSize = st.st_size;
		mOwnerId = st.st_uid;
		mGroupId = st.st_gid;
		mPermissions = st.st_mode;
		mInode = st.st_ino;
	}

	if ( isDirectory() )
		FileSystem::dirAddSlashAtEnd( mFilepath );
}

void FileInfo::getRealInfo() {
	FileSystem::dirRemoveSlashAtEnd( mFilepath );

#if EE_PLATFORM != EE_PLATFORM_WIN
	struct stat st;
	int res = lstat( mFilepath.c_str(), &st );
#else
	struct _stat st;
	int res = _wstat( String::fromUtf8( mFilepath ).toWideString().c_str(), &st );
#endif

	if ( 0 == res ) {
		mModificationTime = st.st_mtime;
		mSize = st.st_size;
		mOwnerId = st.st_uid;
		mGroupId = st.st_gid;
		mPermissions = st.st_mode;
		mInode = st.st_ino;
	}

	if ( isDirectory() )
		FileSystem::dirAddSlashAtEnd( mFilepath );
}

const std::string& FileInfo::getFilepath() const {
	return mFilepath;
}

std::string FileInfo::getFileName() const {
	return mFileName;
}

std::string FileInfo::getDirectoryPath() const {
	if ( isDirectory() )
		return mFilepath;
	return FileSystem::fileRemoveFileName( mFilepath );
}

const Uint64& FileInfo::getModificationTime() const {
	return mModificationTime;
}

const Uint64& FileInfo::getSize() const {
	return mSize;
}

const Uint32& FileInfo::getOwnerId() const {
	return mOwnerId;
}

const Uint32& FileInfo::getGroupId() const {
	return mGroupId;
}

const Uint32& FileInfo::getPermissions() const {
	return mPermissions;
}

const Uint64& FileInfo::getInode() const {
	return mInode;
}

bool FileInfo::isUninitialized() const {
	return mModificationTime == 0;
}

bool FileInfo::operator==( const FileInfo& Other ) const {
	return ( mModificationTime == Other.mModificationTime && mSize == Other.mSize &&
			 mOwnerId == Other.mOwnerId && mGroupId == Other.mGroupId &&
			 mPermissions == Other.mPermissions && mInode == Other.mInode );
}

bool FileInfo::isDirectory() const {
	return 0 != S_ISDIR( mPermissions );
}

bool FileInfo::isRegularFile() const {
	return 0 != S_ISREG( mPermissions );
}

bool FileInfo::isReadable() const {
#if EE_PLATFORM != EE_PLATFORM_WIN
	static bool isRoot = getuid() == 0;
	return isRoot || 0 != S_ISRDBL( mPermissions );
#else
	return 0 != S_ISRDBL( mPermissions );
#endif
}

bool FileInfo::isLink() const {
#if EE_PLATFORM != EE_PLATFORM_WIN
	return S_ISLNK( mPermissions );
#else
	return false;
#endif
}

bool FileInfo::isHidden() const {
	return FileSystem::fileIsHidden( mFilepath );
}

bool FileInfo::linksToDirectory() const {
	return isLink() && FileInfo( linksTo() ).isDirectory();
}

std::string FileInfo::linksTo() const {
#if EE_PLATFORM != EE_PLATFORM_WIN
	if ( isLink() ) {
		char* ch = realpath( mFilepath.c_str(), NULL );

		if ( NULL != ch ) {
			std::string tstr( ch );

			free( ch );

			return tstr;
		}
	}
#endif
	return std::string( "" );
}

bool FileInfo::exists() const {
	if ( isDirectory() )
		FileSystem::dirRemoveSlashAtEnd( mFilepath );

#if EE_PLATFORM != EE_PLATFORM_WIN
	struct stat st;
	int res = stat( mFilepath.c_str(), &st );
#else
	struct _stat st;
	int res = _wstat( String::fromUtf8( mFilepath ).toWideString().c_str(), &st );
#endif

	if ( isDirectory() )
		FileSystem::dirAddSlashAtEnd( mFilepath );

	return 0 == res;
}

FileInfo& FileInfo::operator=( const FileInfo& other ) {
	this->mFilepath = other.mFilepath;
	this->mFileName = other.mFileName;
	this->mSize = other.mSize;
	this->mModificationTime = other.mModificationTime;
	this->mGroupId = other.mGroupId;
	this->mOwnerId = other.mOwnerId;
	this->mPermissions = other.mPermissions;
	this->mInode = other.mInode;
	return *this;
}

bool FileInfo::isExecutable() const {
#if EE_PLATFORM != EE_PLATFORM_WIN
	return mPermissions & S_IEXEC;
#else
	std::string ext = FileSystem::fileExtension( mFilepath );
	return ext == "exe" || ext == "bat";
#endif
}

bool FileInfo::sameInode( const FileInfo& other ) const {
	return inodeSupported() && mInode == other.mInode;
}

bool FileInfo::operator!=( const FileInfo& other ) const {
	return !( *this == other );
}

}} // namespace EE::System
