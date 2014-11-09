#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/sys.hpp>
#include <sys/stat.h>
#include <list>
#include <algorithm>

#if EE_PLATFORM == EE_PLATFORM_WIN
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <windows.h>
#endif

#ifndef EE_COMPILER_MSVC
	#include <dirent.h>
#else
	#include <direct.h>
	#ifndef S_ISDIR
	#define S_ISDIR(mode)  (((mode) & S_IFMT) == S_IFDIR)
	#endif
#endif

namespace EE { namespace System {

std::string FileSystem::GetOSlash() {
	#if EE_PLATFORM == EE_PLATFORM_WIN
		return std::string( "\\" );
	#else
		return std::string( "/" );
	#endif
}

bool FileSystem::FileGet( const std::string& path, SafeDataPointer& data ) {
	if ( FileExists( path ) ) {
		IOStreamFile fs ( path , std::ios::in | std::ios::binary );

		eeSAFE_DELETE( data.Data );

		data.DataSize	= FileSize( path );
		data.Data		= eeNewArray( Uint8, ( data.DataSize ) );

		fs.Read( reinterpret_cast<char*> ( data.Data ), data.DataSize  );

		return true;
	}

	return false;
}

bool FileSystem::FileGet( const std::string& path, std::vector<Uint8>& data ) {
	if ( FileExists( path ) ) {
		IOStreamFile fs ( path, std::ios::in | std::ios::binary );
		Uint32 fsize = FileSize( path );

		data.clear();
		data.resize( fsize );

		fs.Read( reinterpret_cast<char*> (&data[0]), fsize  );

		return true;
	}

	return false;
}

bool FileSystem::FileCopy( const std::string& src, const std::string& dst ) {
	if ( FileExists( src ) ) {
		Int64	chunksize	= EE_1MB;
		Int64	size		= FileSize( src );
		Int64	size_left	= (Int32)size;
		Int64	allocate	= ( size < chunksize ) ? size : chunksize;
		Int64	copysize	= 0;

		SafeDataPointer data;
		data.DataSize	= (Uint32)allocate;
		data.Data		= eeNewArray( Uint8, ( data.DataSize ) );
		char * buff		= (char*)data.Data;

		IOStreamFile in( src, std::ios::binary | std::ios::in );
		IOStreamFile out( dst, std::ios::binary | std::ios::out );

		if ( in.IsOpen() && out.IsOpen() && size > 0 ) {
			do {
				if ( size_left - chunksize < 0 ) {
					copysize = size_left;
				} else {
					copysize = chunksize;
				}

				in.Read		( &buff[0], copysize );
				out.Write	( (const char*)&buff[0], copysize );

				size_left -= copysize;
			} while ( size_left > 0 );

			return true;
		}
	}

	return false;
}

std::string FileSystem::FileExtension( const std::string& filepath, const bool& lowerExt ) {
	std::string tstr( filepath.substr( filepath.find_last_of(".") + 1 ) );

	if ( lowerExt )
		String::ToLowerInPlace( tstr );

	return tstr;
}

std::string FileSystem::FileRemoveExtension( const std::string& filepath ) {
	return filepath.substr( 0, filepath.find_last_of(".") );
}

std::string FileSystem::FileNameFromPath( const std::string& filepath ) {
	return filepath.substr( filepath.find_last_of("/\\") + 1 );
}

std::string FileSystem::FileRemoveFileName( const std::string& filepath ) {
	return filepath.substr( 0, filepath.find_last_of("/\\") + 1 );
}

void FileSystem::FilePathRemoveProcessPath( std::string& path ) {
	static std::string ProcessPath = Sys::GetProcessPath();

	if ( String::StartsWith( path, ProcessPath ) && ProcessPath.length() < path.size() ) {
		path = path.substr( ProcessPath.length() );
	}
}


bool FileSystem::FileWrite( const std::string& filepath, const Uint8* data, const Uint32& dataSize ) {
	IOStreamFile fs( filepath, std::ios::out | std::ios::binary );

	if ( fs.IsOpen() ) {
		if ( dataSize ) {
			fs.Write( reinterpret_cast<const char*> (data), dataSize );
		}

		return true;
	}

	return false;
}

bool FileSystem::FileWrite( const std::string& filepath, const std::vector<Uint8>& data ) {
	return FileWrite( filepath, reinterpret_cast<const Uint8*> ( &data[0] ), (Uint32)data.size() );
}

bool FileSystem::FileRemove( const std::string& filepath ) {
	return 0 == remove( filepath.c_str() );
}

Uint32 FileSystem::FileGetModificationDate( const std::string& Filepath ) {
	struct stat st;
	int res = stat( Filepath.c_str(), &st );

	if ( 0 == res )
		return (Uint32)st.st_mtime;

	return 0;
}

void FileSystem::DirPathAddSlashAtEnd( std::string& path ) {
	if ( path.size() && path[ path.size() - 1 ] != '/' && path[ path.size() - 1 ] != '\\' )
		path += GetOSlash();
}

std::string FileSystem::RemoveLastFolderFromPath( std::string path ) {
	if ( path.size() > 1 && ( path[ path.size() - 1 ] == '/' || path[ path.size() - 1 ] == '\\' ) ) {
		path.resize( path.size() - 1 );
	}

	std::size_t pos = path.find_last_of( GetOSlash() );

	if ( std::string::npos != pos ) {
		std::string sstr;
		std::size_t pos2 = path.find_first_of( GetOSlash() );

		if ( pos2 != pos ) {
			sstr = path.substr(0,pos) + GetOSlash();
		} else {
			if ( pos == pos2 ) {
				sstr = path.substr(0,pos2+1);
			}
		}

		if ( sstr.size() ) {
			DirPathAddSlashAtEnd( sstr );

			return sstr;
		}
	}

	DirPathAddSlashAtEnd( path );

	return path;
}

bool FileSystem::IsDirectory( const String& path ) {
	return IsDirectory( path.ToUtf8() );
}

bool FileSystem::IsDirectory( const std::string& path ) {
#ifndef EE_COMPILER_MSVC
	DIR *dp = NULL;

	bool isdir = !( ( dp = opendir( path.c_str() ) ) == NULL);

	if ( NULL != dp )
		closedir(dp);

	return isdir;
#else
	return 0 != ( GetFileAttributes( (LPCTSTR) path.c_str() ) & FILE_ATTRIBUTE_DIRECTORY );
#endif
}

bool FileSystem::MakeDir( const std::string& path, const Uint16& mode ) {
	Int16 v;
#if EE_PLATFORM == EE_PLATFORM_WIN
	#ifdef EE_COMPILER_MSVC
	v = _mkdir( path.c_str() );
	#else
	v = mkdir( path.c_str() );
	#endif
#else
	v = mkdir( path.c_str(), mode );
#endif
	return v == 0;
}

std::vector<String> FileSystem::FilesGetInPath( const String& path, const bool& sortByName, const bool& foldersFirst ) {
	std::vector<String> files;

#ifdef EE_COMPILER_MSVC
	#ifdef UNICODE
		String mPath( path );

		if ( mPath[ mPath.size() - 1 ] == '/' || mPath[ mPath.size() - 1 ] == '\\' ) {
			mPath += "*";
		} else {
			mPath += "\\*";
		}

		WIN32_FIND_DATA findFileData;
		HANDLE hFind = FindFirstFile( (LPCWSTR)mPath.ToWideString().c_str(), &findFileData );

		if( hFind != INVALID_HANDLE_VALUE ) {
			String tmpstr( findFileData.cFileName );

			if ( tmpstr != "." && tmpstr != ".." )
				files.push_back( tmpstr );

			while( FindNextFile( hFind, &findFileData ) ) {
				tmpstr = String( findFileData.cFileName );

				if ( tmpstr != "." && tmpstr != ".." )
					files.push_back( tmpstr );
			}

			FindClose( hFind );
		}
	#else
		String mPath( path );

		if ( mPath[ mPath.size() - 1 ] == '/' || mPath[ mPath.size() - 1 ] == '\\' ) {
				mPath += "*";
		} else {
				mPath += "\\*";
		}

		WIN32_FIND_DATA findFileData;
		HANDLE hFind = FindFirstFile( (LPCTSTR) mPath.ToAnsiString().c_str(), &findFileData );

		if( hFind != INVALID_HANDLE_VALUE ) {
			String tmpstr( String::FromUtf8( findFileData.cFileName ) );

			if ( tmpstr != "." && tmpstr != ".." )
					files.push_back( tmpstr );

			while( FindNextFile( hFind, &findFileData ) ) {
					tmpstr = String::FromUtf8( findFileData.cFileName );

					if ( tmpstr != "." && tmpstr != ".." )
							files.push_back( tmpstr );
			}

			FindClose( hFind );
		}
	#endif
#else
	DIR *dp;
	struct dirent *dirp;

	if( ( dp = opendir( path.ToUtf8().c_str() ) ) == NULL )
		return files;

	while ( ( dirp = readdir(dp) ) != NULL) {
		if ( strcmp( dirp->d_name, ".." ) != 0 && strcmp( dirp->d_name, "." ) != 0 ) {

			char * p = &dirp->d_name[0];
			String tmp;

			while ( *p ) {
				unsigned char y = *p;
				tmp.push_back( y );
				p++;
			}

			files.push_back( tmp );
		}
	}

	closedir(dp);
#endif

	if ( sortByName == true )
	{
		std::sort( files.begin(), files.end() );
	}

	if ( foldersFirst == true )
	{
		String fpath( path );

		if ( fpath[ fpath.size() - 1 ] != '/' && fpath[ fpath.size() - 1 ] != '\\' )
			fpath += GetOSlash();

		std::list<String> folders;
		std::list<String> file;

		for ( size_t i = 0; i < files.size(); i++ ) {
			if ( FileSystem::IsDirectory( fpath + files[i] ) ) {
				folders.push_back( files[i] );
			} else {
				file.push_back( files[i] );
			}
		}

		files.clear();

		std::list<String>::iterator it;

		for ( it = folders.begin(); it != folders.end(); it++ )
			files.push_back( *it );

		for ( it = file.begin(); it != file.end(); it++ )
			files.push_back( *it );
	}

	return files;
}

std::vector<std::string> FileSystem::FilesGetInPath( const std::string& path, const bool& sortByName, const bool& foldersFirst ) {
	std::vector<std::string> files;

#ifdef EE_COMPILER_MSVC
	#ifdef UNICODE
		String mPath( String::FromUtf8( path ) );

		if ( mPath[ mPath.size() - 1 ] == '/' || mPath[ mPath.size() - 1 ] == '\\' ) {
			mPath += "*";
		} else {
			mPath += "\\*";
		}

		WIN32_FIND_DATA findFileData;
		HANDLE hFind = FindFirstFile( (LPCWSTR)mPath.ToWideString().c_str(), &findFileData );

		if( hFind != INVALID_HANDLE_VALUE ) {
			String tmpstr( findFileData.cFileName );

			if ( tmpstr != "." && tmpstr != ".." )
				files.push_back( tmpstr.ToUtf8() );

			while( FindNextFile(hFind, &findFileData ) ) {
				tmpstr = String( findFileData.cFileName );

				if ( tmpstr != "." && tmpstr != ".." )
					files.push_back( String( findFileData.cFileName ).ToUtf8() );
			}

			FindClose( hFind );
		}
	#else
		std::string mPath( path );

		if ( mPath[ mPath.size() - 1 ] == '/' || mPath[ mPath.size() - 1 ] == '\\' ) {
				mPath += "*";
		} else {
				mPath += "\\*";
		}

		WIN32_FIND_DATA findFileData;
		HANDLE hFind = FindFirstFile( (LPCTSTR) mPath.c_str(), &findFileData );

		if( hFind != INVALID_HANDLE_VALUE ) {
				std::string tmpstr( findFileData.cFileName );

				if ( tmpstr != "." && tmpstr != ".." )
						files.push_back( tmpstr );

				while( FindNextFile(hFind, &findFileData ) ) {
						tmpstr = std::string( findFileData.cFileName );

						if ( tmpstr != "." && tmpstr != ".." )
								files.push_back( std::string( findFileData.cFileName ) );
				}

				FindClose( hFind );
		}
	#endif
#else
	DIR *dp;
	struct dirent *dirp;

	if( ( dp = opendir( path.c_str() ) ) == NULL)
		return files;

	while ( ( dirp = readdir(dp) ) != NULL) {
		if ( strcmp( dirp->d_name, ".." ) != 0 && strcmp( dirp->d_name, "." ) != 0 )
			files.push_back( std::string( dirp->d_name ) );
	}

	closedir(dp);
#endif

	if ( sortByName == true )
	{
		std::sort( files.begin(), files.end() );
	}

	if ( foldersFirst == true )
	{
		String fpath( path );

		if ( fpath[ fpath.size() - 1 ] != '/' && fpath[ fpath.size() - 1 ] != '\\' )
			fpath += GetOSlash();

		std::list<String> folders;
		std::list<String> file;

		for ( size_t i = 0; i < files.size(); i++ ) {
			if ( FileSystem::IsDirectory( fpath + files[i] ) ) {
				folders.push_back( files[i] );
			} else {
				file.push_back( files[i] );
			}
		}

		files.clear();

		std::list<String>::iterator it;

		for ( it = folders.begin(); it != folders.end(); it++ )
			files.push_back( *it );

		for ( it = file.begin(); it != file.end(); it++ )
			files.push_back( *it );
	}

	return files;
}

Uint64 FileSystem::FileSize( const std::string& Filepath ) {
	struct stat st;
	int res = stat( Filepath.c_str(), &st );

	if ( 0 == res )
		return st.st_size;

	return 0;
}

bool FileSystem::FileExists( const std::string& Filepath ) {
	struct stat st;
	return ( stat( Filepath.c_str(), &st ) == 0 ) && !S_ISDIR( st.st_mode );
}

std::string FileSystem::SizeToString( const Int64& Size ) {
	double mem = static_cast<double>( Size );
	std::string size;
	Uint8 c = 0;

	while ( mem > 1024 ) {
		c++;
		mem = mem / 1024;
	}

	switch (c) {
		case 0: size = " bytes"; break;
		case 1: size = " KiB"; break;
		case 2: size = " MiB"; break;
		case 3: size = " GiB"; break;
		case 4: size = " TiB"; break;
		default: size = " WTF";
	}

	return std::string( String::ToStr( mem ) + size );
}

}}
