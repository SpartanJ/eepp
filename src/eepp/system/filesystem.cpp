#include <algorithm>
#include <climits>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/sys.hpp>
#include <sys/stat.h>
#include <filesystem>

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

#ifndef EE_COMPILER_MSVC
#include <dirent.h>
#else
#include <direct.h>
#ifndef S_ISDIR
#define S_ISDIR( mode ) ( ( ( mode ) & S_IFMT ) == S_IFDIR )
#endif
#endif

#if defined( EE_PLATFORM_POSIX )
#if EE_PLATFORM != EE_PLATFORM_ANDROID
#include <sys/statvfs.h>
#else
#include <sys/vfs.h>
#define statvfs statfs
#define fstatvfs fstatfs
#endif
#endif

namespace EE { namespace System {

std::string FileSystem::getOSSlash() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return std::string( "\\" );
#else
	return std::string( "/" );
#endif
}

bool FileSystem::fileGet( const std::string& path, ScopedBuffer& data ) {
	if ( fileExists( path ) ) {
		IOStreamFile fs( path );

		data.reset( fs.getSize() );

		fs.read( reinterpret_cast<char*>( data.get() ), data.length() );

		return true;
	}

	return false;
}

bool FileSystem::fileGet( const std::string& path, std::vector<Uint8>& data ) {
	if ( fileExists( path ) ) {
		IOStreamFile fs( path );
		ios_size fsize = fs.getSize();

		data.clear();
		data.resize( fsize );

		fs.read( reinterpret_cast<char*>( &data[0] ), fsize );

		return true;
	}

	return false;
}

bool FileSystem::fileGet( const std::string& path, std::string& data ) {
	if ( fileExists( path ) ) {
		IOStreamFile fs( path );
		ios_size fsize = fs.getSize();

		data.clear();
		data.resize( fsize );

		fs.read( reinterpret_cast<char*>( &data[0] ), fsize );

		return true;
	}

	return false;
}

bool FileSystem::fileCopy( const std::string& src, const std::string& dst ) {
	try {
#if EE_PLATFORM == EE_PLATFORM_WIN
		std::filesystem::path source = String( src ).toWideString();
		std::filesystem::path destination = String( dst ).toWideString();
#else
		std::filesystem::path source = src;
		std::filesystem::path destination = dst;
#endif

		if ( !std::filesystem::exists( source ) )
			return false;

		if ( std::filesystem::is_directory( source ) ) {
			std::filesystem::copy( source, destination,
								   std::filesystem::copy_options::recursive |
									   std::filesystem::copy_options::overwrite_existing );
		} else {
			std::filesystem::copy( source, destination,
								   std::filesystem::copy_options::overwrite_existing );
		}
		return true;
	} catch ( const std::filesystem::filesystem_error& ) {
		return false;
	} catch ( const std::exception& ) {
		return false;
	}
}

std::string FileSystem::fileExtension( const std::string& filepath, const bool& lowerExt ) {
	std::string filename( fileNameFromPath( filepath ) );

	size_t dotPos = filename.find_last_of( "." );
	std::string tstr( dotPos != std::string::npos ? filename.substr( dotPos + 1 ) : "" );

	if ( lowerExt )
		String::toLowerInPlace( tstr );

	return tstr;
}

std::string FileSystem::fileRemoveExtension( const std::string& filepath ) {
	return filepath.substr( 0, filepath.find_last_of( "." ) );
}

std::string FileSystem::fileNameFromPath( const std::string& filepath ) {
	if ( !filepath.empty() ) {
		if ( filepath[filepath.size() - 1] == '/' || filepath[filepath.size() - 1] == '\\' ) {
			auto fp = filepath.substr( 0, filepath.size() - 1 );
			return fp.substr( fp.find_last_of( "/\\" ) + 1 );
		}
		return filepath.substr( filepath.find_last_of( "/\\" ) + 1 );
	}
	return "";
}

std::string FileSystem::fileRemoveFileName( const std::string& filepath ) {
	return filepath.substr( 0, filepath.find_last_of( "/\\" ) + 1 );
}

void FileSystem::filePathRemoveBasePath( const std::string& basePath, std::string& path ) {
	if ( String::startsWith( path, basePath ) && basePath.length() < path.size() ) {
		path = path.substr( basePath.length() );
	}
}

void FileSystem::filePathRemoveProcessPath( std::string& path ) {
	return filePathRemoveBasePath( Sys::getProcessPath(), path );
}

void FileSystem::filePathRemoveCurrentWorkingDirectory( std::string& path ) {
	return filePathRemoveBasePath( getCurrentWorkingDirectory(), path );
}

bool FileSystem::fileWrite( const std::string& filepath, const Uint8* data,
							const Uint32& dataSize ) {
	IOStreamFile fs( filepath, "wb" );

	if ( fs.isOpen() ) {
		if ( dataSize ) {
			fs.write( reinterpret_cast<const char*>( data ), dataSize );
		}

		return true;
	}

	return false;
}

bool FileSystem::fileWrite( const std::string& filepath, const std::vector<Uint8>& data ) {
	return fileWrite( filepath, reinterpret_cast<const Uint8*>( &data[0] ), (Uint32)data.size() );
}

bool FileSystem::fileWrite( const std::string& filepath, std::string_view data ) {
	return fileWrite( filepath, (const Uint8*)data.data(), (Uint32)data.size() );
}

bool FileSystem::fileRemove( const std::string& filepath ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return DeleteFileW( String( filepath ).toWideString().c_str() );
#else
	return 0 == remove( filepath.c_str() );
#endif
}

bool FileSystem::fileHide( const std::string& filepath ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return SetFileAttributesW( (LPCWSTR)String( filepath ).toWideString().c_str(),
							   FILE_ATTRIBUTE_HIDDEN );
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	return 0 == chflags( filepath.c_str(), UF_HIDDEN );
#else
	return false;
#endif
}

Uint32 FileSystem::fileGetModificationDate( const std::string& filepath ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	struct _stat st;
	int res = _wstat( String( filepath ).toWideString().c_str(), &st );
#else
	struct stat st;
	int res = stat( filepath.c_str(), &st );
#endif

	if ( 0 == res )
		return (Uint32)st.st_mtime;

	return 0;
}

bool FileSystem::fileCanWrite( const std::string& filepath ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	auto attrs = GetFileAttributesW( String::fromUtf8( filepath ).toWideString().c_str() );
	return attrs != INVALID_FILE_ATTRIBUTES && 0 == ( attrs & FILE_ATTRIBUTE_READONLY );
#else
	struct stat st;
	if ( stat( filepath.c_str(), &st ) == 0 ) {
		if ( 0 == geteuid() )
			return true;
		else if ( st.st_uid == geteuid() )
			return ( st.st_mode & S_IWUSR ) != 0;
		else if ( st.st_gid == getegid() )
			return ( st.st_mode & S_IWGRP ) != 0;
		else
			return ( st.st_mode & S_IWOTH ) != 0 || geteuid() == 0;
	}
	return false;
#endif
}

bool FileSystem::fileIsHidden( const std::string& filepath ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	auto attrs = GetFileAttributesW( String::fromUtf8( filepath ).toWideString().c_str() );
	return attrs != INVALID_FILE_ATTRIBUTES && 0 != ( attrs & FILE_ATTRIBUTE_HIDDEN );
#else
	std::string filename( fileNameFromPath( filepath ) );
	return filename.empty() ? false : filename[0] == '.';
#endif
}

void FileSystem::dirAddSlashAtEnd( std::string& path ) {
	if ( path.size() && path[path.size() - 1] != '/' && path[path.size() - 1] != '\\' )
		path += getOSSlash();
}

void FileSystem::dirRemoveSlashAtEnd( std::string& dir ) {
	if ( dir.size() > 1 && ( dir[dir.size() - 1] == '/' || dir[dir.size() - 1] == '\\' ) ) {
		dir.erase( dir.size() - 1 );
	}
}

std::string FileSystem::removeLastFolderFromPath( std::string path ) {
	if ( path.size() > 1 && ( path[path.size() - 1] == '/' || path[path.size() - 1] == '\\' ) ) {
		path.resize( path.size() - 1 );
	}

	std::size_t pos = path.find_last_of( getOSSlash() );

	if ( std::string::npos != pos ) {
		std::string sstr;
		std::size_t pos2 = path.find_first_of( getOSSlash() );

		if ( pos2 != pos ) {
			sstr = path.substr( 0, pos ) + getOSSlash();
		} else {
			sstr = path.substr( 0, pos2 + 1 );
		}

		if ( sstr.size() ) {
			dirAddSlashAtEnd( sstr );

			return sstr;
		}
	}

	dirAddSlashAtEnd( path );

	return path;
}

bool FileSystem::isDirectory( const String& path ) {
	return isDirectory( path.toUtf8() );
}

bool FileSystem::isDirectory( const std::string& path ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	auto attrs = GetFileAttributesW( String::fromUtf8( path ).toWideString().c_str() );
	return attrs != INVALID_FILE_ATTRIBUTES && 0 != ( attrs & FILE_ATTRIBUTE_DIRECTORY );
#else
	struct stat st;
	return ( stat( path.c_str(), &st ) == 0 ) && S_ISDIR( st.st_mode );
#endif
}

static inline bool eepp_mkdir( const std::string& path, const Uint16& mode ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return 0 != CreateDirectoryW( String( path ).toWideString().c_str(), NULL );
#else
	int v;
	v = mkdir( path.c_str(), mode );
	return v == 0;
#endif
}

bool FileSystem::makeDir( const std::string& path, bool recursive, const Uint16& mode ) {
	if ( recursive && path.size() > 1 ) {
		size_t i = path.size() - 1;
		for ( ; i >= 0; --i ) {
			if ( path[i] == '/' || path[i] == '\\' ) {
				if ( FileSystem::isDirectory( path.substr( 0, i ) ) ) {
					break;
				}
			}
		}

		if ( i == 0 )
			return false;

		i++;
		for ( ; i < path.size(); ++i ) {
			if ( path[i] == '/' || path[i] == '\\' ) {
				if ( !eepp_mkdir( path.substr( 0, i ), mode ) )
					return false;
			}
		}

		if ( path[path.size() - 1] == '/' || path[path.size() - 1] == '\\' )
			return true;
	}

	return eepp_mkdir( path, mode );
}

std::string FileSystem::getRealPath( const std::string& path ) {
	std::string realPath;
#ifdef EE_PLATFORM_POSIX
	char dir[PATH_MAX];
	realpath( path.c_str(), &dir[0] );
	realPath = std::string( dir );
#elif EE_PLATFORM == EE_PLATFORM_WIN
	wchar_t dir[_MAX_PATH + 1];
	GetFullPathNameW( String::fromUtf8( path ).toWideString().c_str(), _MAX_PATH, &dir[0],
					  nullptr );
	realPath = String( dir ).toUtf8();
#else
#warning FileSystem::getRealPath() not implemented on this platform.
#endif
	return realPath;
}

std::vector<String> FileSystem::filesGetInPath( const String& path, const bool& sortByName,
												const bool& foldersFirst, const bool& ignoreHidden,
												const std::function<bool()> shouldAbort ) {
	std::vector<String> files;

#if EE_PLATFORM == EE_PLATFORM_WIN
	String widePath( path );

	if ( widePath[widePath.size() - 1] == '/' || widePath[widePath.size() - 1] == '\\' ) {
		widePath += "*";
	} else {
		widePath += "\\*";
	}

	WIN32_FIND_DATAW findFileData;
	HANDLE hFind = FindFirstFileW( widePath.toWideString().c_str(), &findFileData );

	if ( hFind != INVALID_HANDLE_VALUE ) {
		String tmpstr( findFileData.cFileName );

		if ( tmpstr != "." && tmpstr != ".." )
			files.push_back( tmpstr );

		while ( FindNextFileW( hFind, &findFileData ) && ( !shouldAbort || !shouldAbort() ) ) {
			tmpstr = String( findFileData.cFileName );

			if ( tmpstr != "." && tmpstr != ".." )
				files.push_back( tmpstr );
		}

		FindClose( hFind );
	}
#else
	DIR* dp;
	struct dirent* dirp;

	if ( ( dp = opendir( path.toUtf8().c_str() ) ) == NULL )
		return files;

	while ( ( dirp = readdir( dp ) ) != NULL && ( !shouldAbort || !shouldAbort() ) ) {
#if EE_PLATFORM != EE_PLATFORM_HAIKU
		if ( strncmp( dirp->d_name, "..", sizeof( dirp->d_name ) ) != 0 &&
			 strncmp( dirp->d_name, ".", sizeof( dirp->d_name ) ) != 0 ) {
#else
		if ( strcmp( dirp->d_name, ".." ) != 0 && strcmp( dirp->d_name, "." ) != 0 ) {
#endif
			files.push_back(
				String::fromUtf8( std::string_view{ dirp->d_name, strlen( dirp->d_name ) } ) );
		}
	}

	closedir( dp );
#endif

	if ( sortByName ) {
		std::sort( files.begin(), files.end() );
	}

	if ( foldersFirst ) {
		String fpath( path );
		if ( fpath[fpath.size() - 1] != '/' && fpath[fpath.size() - 1] != '\\' )
			fpath += getOSSlash();
		std::stable_partition( files.begin(), files.end(), [&fpath]( const String& file ) {
			return FileSystem::isDirectory( fpath + file );
		} );
	}

	if ( ignoreHidden ) {
		String fpath( path );
		if ( fpath[fpath.size() - 1] != '/' && fpath[fpath.size() - 1] != '\\' )
			fpath += getOSSlash();
		files.erase( std::remove_if( files.begin(), files.end(),
									 [&fpath]( const String& file ) {
										 return FileSystem::fileIsHidden( fpath + file );
									 } ),
					 files.end() );
	}

	return files;
}

std::vector<FileInfo> FileSystem::filesInfoGetInPath( std::string path, bool linkInfo,
													  const bool& sortByName,
													  const bool& foldersFirst,
													  const bool& ignoreHidden,
													  const std::function<bool()> shouldAbort ) {
	dirAddSlashAtEnd( path );
	std::vector<FileInfo> fileInfo;
	auto files = filesGetInPath( path, sortByName, foldersFirst, ignoreHidden, shouldAbort );
	fileInfo.reserve( files.size() );
	for ( const auto& file : files )
		fileInfo.emplace_back( FileInfo( path + file, linkInfo ) );
	return fileInfo;
}

std::vector<std::string> FileSystem::filesGetInPath( const std::string& path,
													 const bool& sortByName,
													 const bool& foldersFirst,
													 const bool& ignoreHidden,
													 const std::function<bool()> shouldAbort ) {
	std::vector<std::string> files;

#if EE_PLATFORM == EE_PLATFORM_WIN
	String widePath( path );

	if ( widePath[widePath.size() - 1] == '/' || widePath[widePath.size() - 1] == '\\' ) {
		widePath += "*";
	} else {
		widePath += "\\*";
	}

	WIN32_FIND_DATAW findFileData;
	HANDLE hFind = FindFirstFileW( widePath.toWideString().c_str(), &findFileData );

	if ( hFind != INVALID_HANDLE_VALUE ) {
		String tmpstr( findFileData.cFileName );

		if ( tmpstr != "." && tmpstr != ".." )
			files.push_back( tmpstr.toUtf8() );

		while ( FindNextFileW( hFind, &findFileData ) && ( !shouldAbort || !shouldAbort() ) ) {
			tmpstr = String( findFileData.cFileName );

			if ( tmpstr != "." && tmpstr != ".." )
				files.push_back( String( findFileData.cFileName ).toUtf8() );
		}

		FindClose( hFind );
	}
#else
	DIR* dp;
	struct dirent* dirp;

	if ( ( dp = opendir( path.c_str() ) ) == NULL )
		return files;

	while ( ( dirp = readdir( dp ) ) != NULL && ( !shouldAbort || !shouldAbort() ) ) {
#if EE_PLATFORM != EE_PLATFORM_HAIKU
		if ( strncmp( dirp->d_name, "..", sizeof( dirp->d_name ) ) != 0 &&
			 strncmp( dirp->d_name, ".", sizeof( dirp->d_name ) ) != 0 )
#else
		if ( strcmp( dirp->d_name, ".." ) != 0 && strcmp( dirp->d_name, "." ) != 0 )
#endif
			files.push_back( std::string( dirp->d_name ) );
	}

	closedir( dp );
#endif

	if ( sortByName ) {
		std::sort( files.begin(), files.end() );
	}

	if ( foldersFirst ) {
		std::string fpath( path );
		dirAddSlashAtEnd( fpath );
		std::stable_partition( files.begin(), files.end(), [&fpath]( const std::string& file ) {
			return FileSystem::isDirectory( fpath + file );
		} );
	}

	if ( ignoreHidden ) {
		std::string fpath( path );
		dirAddSlashAtEnd( fpath );
		files.erase( std::remove_if( files.begin(), files.end(),
									 [&fpath]( const std::string& file ) {
										 return FileSystem::fileIsHidden( fpath + file );
									 } ),
					 files.end() );
	}

	return files;
}

Uint64 FileSystem::fileSize( const std::string& Filepath ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	struct _stat st;
	int res = _wstat( String( Filepath ).toWideString().c_str(), &st );
#else
	struct stat st;
	int res = stat( Filepath.c_str(), &st );
#endif

	if ( 0 == res )
		return st.st_size;

	return 0;
}

bool FileSystem::fileExists( const std::string& Filepath ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	struct _stat st;
	return ( _wstat( String( Filepath ).toWideString().c_str(), &st ) == 0 );
#else
	struct stat st;
	return ( stat( Filepath.c_str(), &st ) == 0 );
#endif
}

std::string FileSystem::sizeToString( const Int64& Size ) {
	double mem = static_cast<double>( Size );
	std::string size;
	Uint8 c = 0;

	while ( mem > 1024 ) {
		c++;
		mem = mem / 1024;
	}

	switch ( c ) {
		case 0:
			size = "B";
			break;
		case 1:
			size = "KiB";
			break;
		case 2:
			size = "MiB";
			break;
		case 3:
			size = "GiB";
			break;
		case 4:
			size = "TiB";
			break;
		default:
			size = "WTF";
	}

	if ( mem == (Int64)mem )
		return String::format( "%lld %s", (Int64)mem, size.c_str() );
	return String::format( "%4.2f %s", mem, size.c_str() );
}

bool FileSystem::changeWorkingDirectory( const std::string& path ) {
	int res;
#ifdef EE_COMPILER_MSVC
#ifdef UNICODE
	res = _wchdir( String::fromUtf8( path ).toWideString().c_str() );
#else
	res = _chdir( String::fromUtf8( path ).toAnsiString().c_str() );
#endif
#else
	res = chdir( path.c_str() );
#endif
	return -1 != res;
}

std::string FileSystem::getCurrentWorkingDirectory() {
#ifdef EE_COMPILER_MSVC
#if defined( UNICODE ) && !defined( EE_NO_WIDECHAR )
	wchar_t dir[_MAX_PATH];
	return ( 0 != GetCurrentDirectoryW( _MAX_PATH, dir ) ) ? String( dir ).toUtf8() : std::string();
#else
	char dir[_MAX_PATH];
	return ( 0 != GetCurrentDirectory( _MAX_PATH, dir ) ) ? String( dir, std::locale() ).toUtf8()
														  : std::string();
#endif
#else
	char dir[PATH_MAX + 1];
	getcwd( dir, PATH_MAX + 1 );
	return std::string( dir );
#endif
}

Int64 FileSystem::getDiskFreeSpace( const std::string& path ) {
#if defined( EE_PLATFORM_POSIX )
	struct statvfs data;
	statvfs( path.c_str(), &data );
#if EE_PLATFORM != EE_PLATFORM_MACOS
	return (Int64)data.f_bsize * (Int64)data.f_bfree;
#else
	return (Int64)data.f_frsize * (Int64)data.f_bfree;
#endif
#elif EE_PLATFORM == EE_PLATFORM_WIN
	Int64 AvailableBytes;
	Int64 TotalBytes;
	Int64 FreeBytes;
#ifdef UNICODE
	GetDiskFreeSpaceEx( String::fromUtf8( path ).toWideString().c_str(),
						(PULARGE_INTEGER)&AvailableBytes,
#else
	GetDiskFreeSpaceEx( path.c_str(), (PULARGE_INTEGER)&AvailableBytes,
#endif
						(PULARGE_INTEGER)&TotalBytes, (PULARGE_INTEGER)&FreeBytes );

	return FreeBytes;
#else
	return -1;
#endif
}

std::string FileSystem::fileGetNumberedFileNameFromPath( std::string directoryPath,
														 const std::string& fileName,
														 const std::string& separator,
														 const std::string& fileExtension ) {
	Uint32 fileNum = 1;
	std::string fileNumName;

	if ( FileSystem::isDirectory( directoryPath ) ) {
		dirAddSlashAtEnd( directoryPath );

		while ( fileNum < 10000 ) {
			fileNumName = String::format(
				std::string( "%s" + separator + "%d%s" ).c_str(), fileName.c_str(), fileNum,
				fileExtension.empty() ? "" : std::string( "." + fileExtension ).c_str() );

			if ( !FileSystem::fileExists( directoryPath + fileNumName ) )
				return fileNumName;

			fileNum++;
		}
	}

	return "";
}

bool FileSystem::isRelativePath( const std::string& path ) {
	if ( !path.empty() ) {
		if ( path[0] == '/' )
			return false;
#if EE_PLATFORM == EE_PLATFORM_WIN
		if ( path.size() >= 2 && String::isLetter( path[0] ) && path[1] == ':' )
			return false;
#endif
	}
	return true;
}

FILE* FileSystem::fopenUtf8( const char* path, const char* mode ) {
#if defined( _WIN32 )
	return _wfopen( String( path ).toWideString().c_str(), String( mode ).toWideString().c_str() );
#else
	return std::fopen( path, mode );
#endif
}

FILE* FileSystem::fopenUtf8( const std::string& path, const std::string& mode ) {
	return fopenUtf8( path.c_str(), mode.c_str() );
}

bool FileSystem::fileMove( const std::string& fpath, const std::string& newFilePath ) {
	try {
#if EE_PLATFORM == EE_PLATFORM_WIN
		std::filesystem::rename( String( fpath ).toWideString(),
								 String( newFilePath ).toWideString() );
#else
		std::filesystem::rename( fpath, newFilePath );
#endif
	} catch ( const std::filesystem::filesystem_error& ) {
		return false;
	}

	return true;
}

}} // namespace EE::System
