#include "utils.hpp"
#include "string.hpp"

#if EE_PLATFORM == EE_PLATFORM_MACOSX
	#include <CoreFoundation/CoreFoundation.h>
	#include <sys/sysctl.h>
#elif EE_PLATFORM == EE_PLATFORM_WIN
	#include <windows.h>
#elif EE_PLATFORM == EE_PLATFORM_LINUX
	#include <libgen.h>
	#include <unistd.h>
#elif EE_PLATFORM == EE_PLATFORM_HAIKU
	#include <kernel/OS.h>
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN
    #include <direct.h>
#endif
#include <sys/stat.h>

#if EE_PLATFORM == EE_PLATFORM_WIN
	#include <sys/utime.h>
#else
	#include <sys/time.h>
#endif

#ifndef EE_COMPILER_MSVC
	#include <dirent.h>
#endif

static bool TickStarted = false;

#if EE_PLATFORM == EE_PLATFORM_WIN

#define TIME_WRAP_VALUE (~(DWORD)0)
/* The first high-resolution ticks value of the application */
static LARGE_INTEGER hires_start_ticks;
/* The number of ticks per second of the high-resolution performance counter */
static LARGE_INTEGER hires_ticks_per_second;
#endif

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_MACOSX || EE_PLATFORM == EE_PLATFORM_BSD
#define HAVE_CLOCK_GETTIME
#endif

#ifdef EE_PLATFORM_POSIX
#ifdef HAVE_CLOCK_GETTIME
static struct timespec start;
#else
static struct timeval start;
#endif
#endif

namespace EE { namespace Utils {

bool FileExists( const std::string& Filepath ) {
	struct stat st;
	return ( stat( Filepath.c_str(), &st ) == 0 );
}

static void eeStartTicks() {
#if EE_PLATFORM == EE_PLATFORM_WIN
    QueryPerformanceFrequency(&hires_ticks_per_second);
    QueryPerformanceCounter(&hires_start_ticks);
#else
	#ifdef HAVE_CLOCK_GETTIME
	clock_gettime(CLOCK_MONOTONIC, &start);
	#else
	gettimeofday(&start, NULL);
	#warning eeStartTicks implemented with gettimeofday. Probably the platform is not fully supported.
	#endif
#endif

	TickStarted = true;
}

Uint32 eeGetTicks() {
	if ( !TickStarted )
		eeStartTicks();

#if EE_PLATFORM == EE_PLATFORM_WIN
	LARGE_INTEGER hires_now;

    QueryPerformanceCounter(&hires_now);

    hires_now.QuadPart -= hires_start_ticks.QuadPart;
    hires_now.QuadPart *= 1000;
    hires_now.QuadPart /= hires_ticks_per_second.QuadPart;

    return (DWORD) hires_now.QuadPart;
#elif defined( EE_PLATFORM_POSIX )
	#ifdef HAVE_CLOCK_GETTIME
	Uint32 ticks;
	struct timespec now;
	clock_gettime(CLOCK_MONOTONIC, &now);
	ticks =
		(now.tv_sec - start.tv_sec) * 1000 + (now.tv_nsec -
											  start.tv_nsec) / 1000000;
	return (ticks);
	#else
	Uint32 ticks;
	struct timeval now;
	gettimeofday(&now, NULL);
	ticks =
		(now.tv_sec - start.tv_sec) * 1000 + (now.tv_usec -
											  start.tv_usec) / 1000;
	return (ticks);
	#endif
#else
	#warning eeGetTicks not implemented in this platform.
#endif
}

void eeSleep( const Uint32& ms ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	Sleep( ms );
#elif defined( EE_PLATFORM_POSIX )
	usleep( static_cast<unsigned long>( ms * 1000 ) );
#else
	#warning eeSleep not implemented in this platform.
#endif
}

std::string AppPath() {
#if EE_PLATFORM == EE_PLATFORM_MACOSX
	char exe_file[PATH_MAX + 1];
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	if (mainBundle) {
		CFURLRef mainURL = CFBundleCopyBundleURL(mainBundle);

		if (mainURL) {
			int ok = CFURLGetFileSystemRepresentation ( mainURL, (Boolean) true, (UInt8*)exe_file, PATH_MAX );

			if (ok) {
				return std::string(exe_file) + "/";
			}
		}
	}

	return "./";
#elif EE_PLATFORM == EE_PLATFORM_LINUX
	char exe_file[PATH_MAX + 1];
	int size;
	size = readlink("/proc/self/exe", exe_file, PATH_MAX);
	if (size < 0) {
		return "./";
	} else {
		exe_file[size] = '\0';
		return std::string(dirname(exe_file)) + "/";
	}
#elif EE_PLATFORM == EE_PLATFORM_WIN
	#ifdef UNICODE
		// Get path to executable:
		char szDrive[_MAX_DRIVE];
		char szDir[_MAX_DIR];
		char szFilename[_MAX_DIR];
		char szExt[_MAX_DIR];
		std::wstring dllName( _MAX_DIR, 0 );

		GetModuleFileName(0, &dllName[0], _MAX_PATH);

		std::string dllstrName( wstringTostring( dllName ) );

		#ifdef EE_COMPILER_MSVC
		_splitpath_s( dllstrName.c_str(), szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFilename, _MAX_DIR, szExt, _MAX_DIR );
		#else
		_splitpath(szDllName, szDrive, szDir, szFilename, szExt);
		#endif

		return std::string( szDrive ) + std::string( szDir );
	#else
        // Get path to executable:
        TCHAR szDllName[_MAX_PATH];
        TCHAR szDrive[_MAX_DRIVE];
        TCHAR szDir[_MAX_DIR];
        TCHAR szFilename[_MAX_DIR];
        TCHAR szExt[_MAX_DIR];
        GetModuleFileName(0, szDllName, _MAX_PATH);

        #ifdef EE_COMPILER_MSVC
        _splitpath_s(szDllName, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFilename, _MAX_DIR, szExt, _MAX_DIR );
        #else
        _splitpath(szDllName, szDrive, szDir, szFilename, szExt);
        #endif

        return std::string(szDrive) + std::string(szDir);
	#endif
#else
	#warning AppPath() not implemented on this platform. ( will return "./" )
	return "./";
#endif
}

std::vector<std::wstring> FilesGetInPath( const std::wstring& path ) {
	std::vector<std::wstring> files;

#ifdef EE_COMPILER_MSVC
	#ifdef UNICODE
		std::wstring mPath( path );

		if ( mPath[ mPath.size() - 1 ] == '/' || mPath[ mPath.size() - 1 ] == '\\' ) {
			mPath += L"*";
		} else {
			mPath += L"\\*";
		}

		WIN32_FIND_DATA findFileData;
		HANDLE hFind = FindFirstFile( mPath.c_str(), &findFileData );

		if( hFind != INVALID_HANDLE_VALUE ) {
			std::wstring tmpstr( findFileData.cFileName );

			if ( tmpstr != L"." && tmpstr != L".." )
				files.push_back( tmpstr );

			while( FindNextFile( hFind, &findFileData ) ) {
				tmpstr = std::wstring( findFileData.cFileName );

				if ( tmpstr != L"." && tmpstr != L".." )
					files.push_back( tmpstr );
			}

			FindClose( hFind );
		}
	#else
        std::wstring mPath( path );

		if ( mPath[ mPath.size() - 1 ] == '/' || mPath[ mPath.size() - 1 ] == '\\' ) {
                mPath += L"*";
        } else {
                mPath += L"\\*";
        }

        WIN32_FIND_DATA findFileData;
        HANDLE hFind = FindFirstFile( (LPCTSTR) mPath.c_str(), &findFileData );

        if( hFind != INVALID_HANDLE_VALUE ) {
			std::wstring tmpstr( stringTowstring( findFileData.cFileName ) );

			if ( tmpstr != L"." && tmpstr != L".." )
					files.push_back( tmpstr );

			while( FindNextFile( hFind, &findFileData ) ) {
					tmpstr = std::wstring( stringTowstring( findFileData.cFileName ) );

					if ( tmpstr != L"." && tmpstr != L".." )
							files.push_back( tmpstr );
			}

			FindClose( hFind );
        }
	#endif

	return files;
#else
	DIR *dp;
	struct dirent *dirp;

	if( ( dp = opendir( wstringTostring( path.c_str() ).c_str() ) ) == NULL)
		return files;

	while ( ( dirp = readdir(dp) ) != NULL) {
		if ( strcmp( dirp->d_name, ".." ) != 0 && strcmp( dirp->d_name, "." ) != 0 ) {

			char * p = &dirp->d_name[0];
			std::wstring tmp;

			while ( *p ) {
				unsigned char y = *p;
				tmp.push_back( y );
				p++;
			}

			files.push_back( tmp );
		}
	}

	closedir(dp);

	return files;
#endif
}


std::vector<std::string> FilesGetInPath( const std::string& path ) {
	std::vector<std::string> files;

#ifdef EE_COMPILER_MSVC
	#ifdef UNICODE
		std::wstring mPath( stringTowstring( path ) );

		if ( mPath[ mPath.size() - 1 ] == '/' || mPath[ mPath.size() - 1 ] == '\\' ) {
			mPath += L"*";
		} else {
			mPath += L"\\*";
		}

		WIN32_FIND_DATA findFileData;
		HANDLE hFind = FindFirstFile( mPath.c_str(), &findFileData );

		if( hFind != INVALID_HANDLE_VALUE ) {
			std::wstring tmpstr( findFileData.cFileName );

			if ( tmpstr != L"." && tmpstr != L".." )
				files.push_back( wstringTostring( tmpstr ) );

			while( FindNextFile(hFind, &findFileData ) ) {
				tmpstr = std::wstring( findFileData.cFileName );

				if ( tmpstr != L"." && tmpstr != L".." )
					files.push_back( std::string( wstringTostring( findFileData.cFileName ) ) );
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

	return files;
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

	return files;
#endif
}

Uint32 FileSize( const std::string& Filepath ) {
	struct stat st;
	int res = stat( Filepath.c_str(), &st );

	if ( 0 == res )
		return st.st_size;

	return 0;
}

eeDouble GetSystemTime() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	static LARGE_INTEGER Frequency;
	static BOOL          UseHighPerformanceTimer = QueryPerformanceFrequency(&Frequency);

	if (UseHighPerformanceTimer) {
		// High performance counter available : use it
		LARGE_INTEGER CurrentTime;
		QueryPerformanceCounter(&CurrentTime);
		return static_cast<double>(CurrentTime.QuadPart) / Frequency.QuadPart;
	} else
		// High performance counter not available : use GetTickCount (less accurate)
		return GetTickCount() * 0.001;
#else
	timeval Time = {0, 0};
	gettimeofday(&Time, NULL);

	return Time.tv_sec + Time.tv_usec / 1000000.;
#endif
}

bool IsDirectory( const std::wstring& path ) {
	return IsDirectory( wstringTostring( path ) );
}

bool IsDirectory( const std::string& path ) {
#ifndef EE_COMPILER_MSVC
	DIR *dp;
	bool isdir = !( ( dp = opendir( path.c_str() ) ) == NULL);
	closedir(dp);
	return isdir;
#else
	return GetFileAttributes( (LPCTSTR) path.c_str() ) != INVALID_FILE_ATTRIBUTES;
#endif
}

bool MakeDir( const std::string& path, const Uint16& mode ) {
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

std::string GetWindowsPath() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	#ifdef UNICODE
 		wchar_t Buffer[1024];
 		GetWindowsDirectory( Buffer, 1024 );
		return wstringTostring( std::wstring( Buffer ) );
	#else
		char Buffer[1024];
		GetWindowsDirectory( Buffer, 1024 );
		return std::string( Buffer );
	#endif
#else
	return std::string( "/usr/bin/" ); // :P
#endif
}

Uint32 MakeHash( const std::wstring& str ) {
	return MakeHash( reinterpret_cast<const Uint8*>( &str[0] ) );
}

Uint32 MakeHash( const std::string& str ) {
	return MakeHash( reinterpret_cast<const Uint8*>( &str[0] ) );
}

Uint32 MakeHash( const EE::String& str ) {
	return MakeHash( reinterpret_cast<const Uint8*>( str.data() ) );
}

Uint32 MakeHash( const Uint8 * str ) {
	//! This is djb2 + sdbm mixed. This hash doesn't exists, but worked.
	//! Since i found this pretty funny because i used this hash in a lot of projects and never found a collision, i'll let it stay here.
	/**
	Uint32 hash = 5381 + *str;

	while( *str ) {
		hash = *str + ( hash << 6 ) + ( hash << 16 ) - hash;
		str++;
	}

	hash += *( str - 1 );

	return hash;
	*/

	//! djb2
	if ( NULL != str ) {
		Uint32 hash = 5381;
		Int32 c;

		while ( ( c = *str++ ) )
			hash = ( ( hash << 5 ) + hash ) + c;

		return hash;
	}

	return 0;
}

bool FileGet( const std::string& path, std::vector<Uint8>& data ) {
	if ( FileExists( path ) ) {
		std::fstream fs ( path.c_str() , std::ios::in | std::ios::binary );
		Uint32 fsize = FileSize( path );

		data.clear();
		data.resize( fsize );

		fs.read( reinterpret_cast<char*> (&data[0]), fsize  );

		fs.close();

		return true;
	}

	return false;
}

bool FileCopy( const std::string& src, const std::string& dst ) {
	if ( FileExists( src ) ) {
		std::ifstream in( src.c_str() );
		std::ofstream out( dst.c_str() );

		if ( in.is_open() && out.is_open() ) {
			out << in.rdbuf();

			in.close();
			out.close();

			return true;
		}
	}

	return false;
}

std::string FileExtension( const std::string& filepath, const bool& lowerExt ) {
	std::string tstr( filepath.substr( filepath.find_last_of(".") + 1 ) );

	if ( lowerExt )
		toLower( tstr );

	return tstr;
}

std::string FileRemoveExtension( const std::string& filepath ) {
	return filepath.substr( 0, filepath.find_last_of(".") );
}

std::string FileNameFromPath( const std::string& filepath ) {
	return filepath.substr( filepath.find_last_of("/\\") + 1 );
}

std::string FileRemoveFileName( const std::string& filepath ) {
	return filepath.substr( 0, filepath.find_last_of("/\\") + 1 );
}

eeInt GetNumCPUs() {
	eeInt nprocs = -1;

	#if EE_PLATFORM == EE_PLATFORM_WIN
		SYSTEM_INFO info;
		GetSystemInfo(&info);

		nprocs = (eeInt) info.dwNumberOfProcessors;
	#elif EE_PLATFORM == EE_PLATFORM_LINUX
		nprocs = sysconf(_SC_NPROCESSORS_ONLN);
	#elif EE_PLATFORM == EE_PLATFORM_MACOSX
		int mib[2];
		size_t len;
		int maxproc = 1;

		mib[0] = CTL_HW;
		mib[1] = HW_NCPU;
		len = sizeof(maxproc);

		// sysctl != 0 == error
		if ( sysctl(mib, 2, &maxproc, &len, NULL, NULL == -1) )
			return 1;

		nprocs = maxproc;
	#elif EE_PLATFORM == EE_PLATFORM_HAIKU
		system_info info;

		if ( get_system_info( &info ) == B_OK ) {
			return info.cpu_count;
		}
	#else
		#warning GetNumCPUs not implemented on this platform ( it will return 1 ).
	#endif

	if ( nprocs < 0 )
		nprocs = 1;

	return nprocs;
}

bool FileWrite( const std::string& filepath, const Uint8* data, const Uint32& dataSize ) {
	std::fstream fs( filepath.c_str() , std::ios::out | std::ios::binary );

	if ( fs.is_open() ) {
		fs.write( reinterpret_cast<const char*> (data), dataSize );

		fs.close();

		return true;
	}

	return false;
}

bool FileWrite( const std::string& filepath, const std::vector<Uint8>& data ) {
	return FileWrite( filepath, reinterpret_cast<const Uint8*> ( &data[0] ), (Uint32)data.size() );
}

Uint32 FileGetModificationDate( const std::string& Filepath ) {
	struct stat st;
	int res = stat( Filepath.c_str(), &st );

	if ( 0 == res )
		return (Uint32)st.st_mtime;

	return 0;
}


std::string SaveTypeToExtension( const Uint32& Format ) {
	switch( Format ) { // I dont use the save types to avoid including something from EE::Graphics
		case 0: return "tga"; 	// EE_SAVE_TYPE_TGA
		case 1: return "bmp";	// EE_SAVE_TYPE_BMP
		case 2: return "png";	// EE_SAVE_TYPE_PNG
		case 3: return "dds";	// EE_SAVE_TYPE_DDS
	}

	return "";
}

void DirPathAddSlashAtEnd( std::string& path ) {
	if ( path[ path.size() - 1 ] != '/' && path[ path.size() - 1 ] != '\\' )
		path += GetOSlash();
}

std::string GetOSlash() {
	#if EE_PLATFORM == EE_PLATFORM_WIN
		return std::string( "\\" );
	#else
    	return std::string( "/" );
	#endif
}

std::string SizeToString( const Uint32& MemSize ) {
	std::string size = " bytes";
	eeDouble mem = static_cast<eeDouble>( MemSize );
	Uint8 c = 0;

	while ( mem > 1024 ) {
		c++;
		mem = mem / 1024;
	}

	switch (c) {
		case 0: size = " bytes"; break;
		case 1: size = " KB"; break;
		case 2: size = " MB"; break;
		case 3: size = " GB"; break;
		case 4: size = " TB"; break;
		default: size = " WTF";
	}

	return std::string( toStr( mem ) + size );
}

std::wstring SizeToWString( const Uint32& MemSize ) {
	std::wstring size = L" bytes";
	eeDouble mem = static_cast<eeDouble>( MemSize );
	Uint8 c = 0;

	while ( mem > 1024 ) {
		c++;
		mem = mem / 1024;
	}

	switch (c) {
		case 0: size = L" bytes"; break;
		case 1: size = L" KB"; break;
		case 2: size = L" MB"; break;
		case 3: size = L" GB"; break;
		case 4: size = L" TB"; break;
		default: size = L" WTF";
	}

	return std::wstring( toWStr( mem ) + size );
}

void Write32BitKey( Uint32 * Key, Uint32 Pos, Uint32 BitWrite ) {
	if ( BitWrite )
		( * Key ) |= ( 1 << Pos );
	else {
		if ( ( * Key ) & ( 1 << Pos ) )
			( * Key ) &= ~( 1 << Pos );
	}
}

bool Read32BitKey( Uint32 * Key, Uint32 Pos ) {
	return 0 != ( ( * Key ) & ( 1 << Pos ) );
}

void SetFlagValue( Uint32 * Key, Uint32 Val, Uint32 BitWrite ) {
	if ( BitWrite )
		( * Key ) |= Val;
	else {
		if ( ( * Key ) & Val )
			( * Key ) &= ~Val;
	}
}

}}
