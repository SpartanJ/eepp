#include <eepp/system/directorypack.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>

namespace EE { namespace System {

DirectoryPack::DirectoryPack() {}

DirectoryPack::~DirectoryPack() {}

bool DirectoryPack::create( const std::string& path ) {
	if ( !FileSystem::isDirectory( path ) ) {
		FileSystem::makeDir( path );
	}

	return open( path );
}

bool DirectoryPack::open( const std::string& path ) {
	if ( FileSystem::isDirectory( path ) ) {
		mPath = path;

		#if EE_PLATFORM == EE_PLATFORM_WIN
		if ( mPath.find_first_of( '\\' ) != std::string::npos ) {
			String::replaceAll( mPath, "\\", "/" );
		}
		#endif

		if ( mPath.size() && mPath[ path.size() - 1 ] != '/' )
			mPath += "/";

		mIsOpen = true;

		onPackOpened();

		return true;
	}

	return false;
}

bool DirectoryPack::close() {
	mPath = "";
	onPackClosed();
	return true;
}

bool DirectoryPack::addFile( const std::string& path, const std::string& inpack ) {
	return FileSystem::fileCopy( path, mPath + inpack );
}

bool DirectoryPack::addFile( const Uint8 * data, const Uint32 & dataSize, const std::string& inpack ) {
	return FileSystem::fileWrite( mPath + inpack, data, dataSize );
}

bool DirectoryPack::addFile( std::vector<Uint8>& data, const std::string& inpack ) {
	return FileSystem::fileWrite( mPath + inpack, data );
}

bool DirectoryPack::addFiles( std::map<std::string, std::string> paths ) {
	for( std::map<std::string, std::string>::iterator itr = paths.begin(); itr != paths.end(); ++itr )
		if ( !addFile( itr->first, itr->second ) )
			return false;
	return true;
}

bool DirectoryPack::eraseFile( const std::string& path ) {
	return FileSystem::fileRemove( mPath + path );
}

bool DirectoryPack::eraseFiles( const std::vector<std::string>& paths ) {
	for ( auto it = paths.begin(); it != paths.end(); ++it ) {
		if ( !eraseFile( mPath + (*it) ) )
			return false;
	}
	return true;
}

bool DirectoryPack::extractFile( const std::string& path, const std::string& dest) {
	return FileSystem::fileCopy( mPath + path, dest );
}

bool DirectoryPack::extractFileToMemory( const std::string& path, std::vector<Uint8>& data ) {
	return FileSystem::fileGet( mPath + path, data );
}

bool DirectoryPack::extractFileToMemory( const std::string& path, SafeDataPointer& data ) {
	return FileSystem::fileGet( mPath + path, data );
}

Int32 DirectoryPack::exists( const std::string& path ) {
	return FileSystem::fileExists( mPath + path ) ? 1 : -1;
}

Int8 DirectoryPack::checkPack() {
	return mPath.empty() ? -1 : 0;
}

void DirectoryPack::getDirectoryFiles( std::vector<std::string>& files, std::string directory ) {
	std::vector<std::string> pathFiles = FileSystem::filesGetInPath( directory );
	std::string path( directory );

	if ( String::startsWith( path, mPath ) && mPath.length() <= path.size() ) {
		path = path.substr( mPath.length() );
	}

	for ( auto it = pathFiles.begin(); it != pathFiles.end(); ++it ) {
		if ( FileSystem::isDirectory( directory + (*it) ) ) {
			getDirectoryFiles( files, directory + (*it) + "/" );
		} else {
			files.push_back( path + (*it) );
		}
	}
}

std::vector<std::string> DirectoryPack::getFileList() {
	std::vector<std::string> files;

	if ( !mPath.empty() ) {
		getDirectoryFiles( files, mPath );
	}

	return files;
}

std::string DirectoryPack::getPackPath() {
	return mPath;
}

IOStream * DirectoryPack::getFileStream( const std::string & path ) {
	return eeNew( IOStreamFile, ( path ) );
}

}}
