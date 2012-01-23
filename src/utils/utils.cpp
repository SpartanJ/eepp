#include "utils.hpp"
#include "string.hpp"
#include "../system/ciostreamfile.hpp"

#if EE_PLATFORM == EE_PLATFORM_MACOSX
	#include <CoreFoundation/CoreFoundation.h>
	#include <sys/statvfs.h>
#elif EE_PLATFORM == EE_PLATFORM_WIN
	#include <windows.h>
#elif EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_ANDROID
	#include <libgen.h>
	#include <unistd.h>
	#if EE_PLATFORM != EE_PLATFORM_ANDROID
		#include <sys/statvfs.h>
	#else
		#include <sys/vfs.h>
		#define statvfs statfs
		#define fstatvfs fstatfs
	#endif
#elif EE_PLATFORM == EE_PLATFORM_HAIKU
	#include <kernel/OS.h>
	#include <kernel/image.h>
#elif EE_PLATFORM == EE_PLATFORM_SOLARIS
	#include <stdlib.h>
#endif



#if EE_PLATFORM == EE_PLATFORM_MACOSX || EE_PLATFORM == EE_PLATFORM_BSD
#include <sys/sysctl.h>
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

typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);

static std::string GetWindowsArch() {
	std::string arch = "Unknown";
	OSVERSIONINFOEX osvi;
	SYSTEM_INFO si;
	PGNSI pGNSI;
	BOOL bOsVersionInfoEx;

	ZeroMemory(&si, sizeof(SYSTEM_INFO));
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	bOsVersionInfoEx = GetVersionEx( (OSVERSIONINFO*) &osvi );

	if( bOsVersionInfoEx == FALSE ) return arch;

	// Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.
	pGNSI = (PGNSI) GetProcAddress( GetModuleHandle( TEXT( "kernel32.dll" ) ), "GetNativeSystemInfo" );

	if( NULL != pGNSI ) {
		pGNSI(&si);
	} else {
		GetSystemInfo(&si);
	}

	if ( osvi.dwMajorVersion >= 6 ) {
		if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 ) {
			arch = "x64";
		} else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL ) {
			arch = "x86";
		}
	} else {
		arch = "x86";
	}

	return arch;
}

static std::string GetWindowsVersion() {
	std::string os;
	OSVERSIONINFOEX osvi;
	SYSTEM_INFO si;
	PGNSI pGNSI;
	BOOL bOsVersionInfoEx;

	ZeroMemory(&si, sizeof(SYSTEM_INFO));
	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));

	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	bOsVersionInfoEx = GetVersionEx( (OSVERSIONINFO*) &osvi );

	if( bOsVersionInfoEx == FALSE ) return os;

	// Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.
	pGNSI = (PGNSI) GetProcAddress( GetModuleHandle( TEXT( "kernel32.dll" ) ), "GetNativeSystemInfo" );

	if( NULL != pGNSI ) {
		pGNSI(&si);
	} else {
		GetSystemInfo(&si);
	}

	os = "Microsoft ";

	if ( VER_PLATFORM_WIN32_NT == osvi.dwPlatformId && osvi.dwMajorVersion > 4 ) {
		if ( osvi.dwMajorVersion == 6 ) {
			if( osvi.dwMinorVersion == 0 ) {
				if( osvi.wProductType == VER_NT_WORKSTATION ) {
					os += "Windows Vista";
				} else {
					os += "Windows Server 2008";
				}
			}

			if ( osvi.dwMinorVersion == 1 ) {
				if( osvi.wProductType == VER_NT_WORKSTATION ) {
					os += "Windows 7";
				} else {
					os += "Windows Server 2008 R2";
				}
			}
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 ) {
			if ( GetSystemMetrics(SM_SERVERR2) ) {
				os += "Windows Server 2003 R2, ";
			} else if ( osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER ) {
				os += "Windows Storage Server 2003";
			} else if ( osvi.wSuiteMask & 0x00008000 ) { //VER_SUITE_WH_SERVER
				os += "Windows Home Server";
			} else if ( osvi.wProductType == VER_NT_WORKSTATION && si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ) {
				os += "Windows XP Professional x64 Edition";
			} else {
				os += "Windows Server 2003, ";
			}
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 ) {
			os += "Windows XP";
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 ) {
			os += "Windows 2000";
		}

		// Include service pack (if any) and build number.
		std::string CSDVer( osvi.szCSDVersion );
		if ( CSDVer.size() ) {
			os += " " + CSDVer;
		}
	} else if ( VER_PLATFORM_WIN32_WINDOWS == osvi.dwPlatformId ) {
		if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0) {
			if (osvi.szCSDVersion[1] == 'C' ||
				osvi.szCSDVersion[1] == 'B') {
				os += "Windows 95 OSR2";
			} else {
				os += "Windows 95";
			}
		} else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10) {
			if (osvi.szCSDVersion[1] == 'A') {
				os += "Windows 98 SE";
			} else {
				os += "Windows 98";
			}
		} else if (osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90) {
			os += "Windows ME";
		} else if (osvi.dwMajorVersion == 4) {
			os += "Windows unknown 95 family";
		} else {
			os += "Windows";
		}
	} else {
		os += "Windows";
	}

	return os;
}

#define TIME_WRAP_VALUE (~(DWORD)0)
/* The first high-resolution ticks value of the application */
static LARGE_INTEGER hires_start_ticks;
/* The number of ticks per second of the high-resolution performance counter */
static LARGE_INTEGER hires_ticks_per_second;

#endif

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD
#define HAVE_CLOCK_GETTIME
#endif

#if defined( EE_PLATFORM_POSIX )
#include <sys/utsname.h>
#endif

#ifdef EE_PLATFORM_POSIX
#ifdef HAVE_CLOCK_GETTIME
static struct timespec start;
#else
static struct timeval start;
#endif
#endif

using namespace EE::System;

namespace EE { namespace Utils {

std::string GetOSName() {
#if defined( EE_PLATFORM_POSIX )
	struct utsname os;

	if ( -1 != uname( &os ) ) {
		return std::string( os.sysname ) + " " + std::string( os.release );
	}

	return "Unknown";
#elif EE_PLATFORM == EE_PLATFORM_WIN
	return GetWindowsVersion();
#else
	return "Unknown";
#endif
}

std::string GetOSArchitecture() {
#if defined( EE_PLATFORM_POSIX )
	struct utsname os;

	if ( -1 != uname( &os ) ) {
		return std::string( os.machine );
	}

	return "Unknown";
#elif EE_PLATFORM == EE_PLATFORM_WIN
	return GetWindowsArch();
#else
	return "Unknown";
#endif
}

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

std::string GetProcessPath() {
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

		std::string dllstrName( String( dllName ).ToUtf8() );

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
#elif EE_PLATFORM == EE_PLATFORM_BSD
	int mib[4];
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;
	char buf[1024];
	size_t cb = sizeof(buf);
	sysctl(mib, 4, buf, &cb, NULL, 0);

	return FileRemoveFileName( std::string( buf ) );
#elif EE_PLATFORM == EE_PLATFORM_SOLARIS
	return FileRemoveFileName( std::string( getexecname() ) );
#elif EE_PLATFORM == EE_PLATFORM_HAIKU
	image_info info;
	int32 cookie = 0;

	while ( B_OK == get_next_image_info( 0, &cookie, &info ) ) {
		if ( info.type == B_APP_IMAGE )
			break;
	}

	return FileRemoveFileName( std::string( info.name ) );
#elif EE_PLATFORM == EE_PLATFORM_ANDROID
	return "/sdcard/";
#else
	#warning GetProcessPath() not implemented on this platform. ( will return "./" )
	return "./";
#endif
}

std::vector<String> FilesGetInPath( const String& path ) {
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
		HANDLE hFind = FindFirstFile( mPath.ToWideString().c_str(), &findFileData );

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

	return files;
#else
	DIR *dp;
	struct dirent *dirp;

	if( ( dp = opendir( path.ToUtf8().c_str() ) ) == NULL)
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

	return files;
#endif
}


std::vector<std::string> FilesGetInPath( const std::string& path ) {
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
		HANDLE hFind = FindFirstFile( mPath.ToWideString().c_str(), &findFileData );

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

Uint64 FileSize( const std::string& Filepath ) {
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

bool IsDirectory( const String& path ) {
	return IsDirectory( path.ToUtf8() );
}

bool IsDirectory( const std::string& path ) {
#ifndef EE_COMPILER_MSVC
	DIR *dp = NULL;

	bool isdir = !( ( dp = opendir( path.c_str() ) ) == NULL);

	if ( NULL != dp )
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

Uint32 MakeHash( const std::string& str ) {
	return MakeHash( reinterpret_cast<const Uint8*>( &str[0] ) );
}

Uint32 MakeHash( const Uint8 * str ) {
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

bool FileGet( const std::string& path, SafeDataPointer& data ) {
	if ( FileExists( path ) ) {
		cIOStreamFile fs ( path , std::ios::in | std::ios::binary );

		eeSAFE_DELETE( data.Data );

		data.DataSize	= FileSize( path );
		data.Data		= eeNewArray( Uint8, ( data.DataSize ) );

		fs.Read( reinterpret_cast<char*> ( data.Data ), data.DataSize  );

		return true;
	}

	return false;
}

bool FileGet( const std::string& path, std::vector<Uint8>& data ) {
	if ( FileExists( path ) ) {
		cIOStreamFile fs ( path, std::ios::in | std::ios::binary );
		Uint32 fsize = FileSize( path );

		data.clear();
		data.resize( fsize );

		fs.Read( reinterpret_cast<char*> (&data[0]), fsize  );

		return true;
	}

	return false;
}

bool FileCopy( const std::string& src, const std::string& dst ) {
	if ( FileExists( src ) ) {
		Int64	chunksize	= EE_1MB;
		Int64	size		= FileSize( src );
		Int64	size_left	= (Int32)size;
		Int64	allocate	= ( size < chunksize ) ? size : chunksize;
		Int64	copysize	= 0;

		char buff[ allocate ];

		cIOStreamFile in( src, std::ios::binary | std::ios::in );
		cIOStreamFile out( dst, std::ios::binary | std::ios::out );

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

void FilePathRemoveProcessPath( std::string& path ) {
	static std::string ProcessPath = GetProcessPath();

	Int32 pos = StrStartsWith( ProcessPath, path );

	if ( -1 != pos && (Uint32)(pos + 1) < path.size() )
		path = path.substr( pos + 1 );
}

eeInt GetCPUCount() {
	eeInt nprocs = -1;

	#if EE_PLATFORM == EE_PLATFORM_WIN
		SYSTEM_INFO info;
		GetSystemInfo(&info);

		nprocs = (eeInt) info.dwNumberOfProcessors;
	#elif EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_SOLARIS || EE_PLATFORM == EE_PLATFORM_ANDROID
		nprocs = sysconf(_SC_NPROCESSORS_ONLN);
	#elif EE_PLATFORM == EE_PLATFORM_MACOSX || EE_PLATFORM == EE_PLATFORM_BSD
		int mib[2];
		int maxproc = 1;
		size_t len = sizeof(int);

		mib[0] = CTL_HW;
		mib[1] = HW_NCPU;

		len = sizeof(maxproc);

		sysctl( mib, 2, &maxproc, &len, NULL, 0 );

		nprocs = maxproc;
	#elif EE_PLATFORM == EE_PLATFORM_HAIKU
		system_info info;

		if ( B_OK == get_system_info( &info ) ) {
			nprocs = info.cpu_count;
		}
	#else
		#warning GetCPUCount not implemented on this platform ( it will return 1 ).
	#endif

	if ( nprocs < 1 )
		nprocs = 1;

	return nprocs;
}

bool FileWrite( const std::string& filepath, const Uint8* data, const Uint32& dataSize ) {
	cIOStreamFile fs( filepath, std::ios::out | std::ios::binary );

	if ( fs.IsOpen() ) {
		fs.Write( reinterpret_cast<const char*> (data), dataSize );
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
	if ( path.size() && path[ path.size() - 1 ] != '/' && path[ path.size() - 1 ] != '\\' )
		path += GetOSlash();
}

std::string GetOSlash() {
	#if EE_PLATFORM == EE_PLATFORM_WIN
		return std::string( "\\" );
	#else
		return std::string( "/" );
	#endif
}

std::string RemoveLastFolderFromPath( std::string path ) {
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

Int64 GetDiskFreeSpace(const std::string& path) {
#if defined( EE_PLATFORM_POSIX )
	struct statvfs data;
	statvfs(path.c_str(),  &data);
	return (Int64)data.f_bsize * (Int64)data.f_bfree;
#elif EE_PLATFORM == EE_PLATFORM_WIN
	Int64 AvailableBytes;
	Int64 TotalBytes;
	Int64 FreeBytes;
	GetDiskFreeSpaceEx(path.c_str(),(PULARGE_INTEGER) &AvailableBytes,
	(PULARGE_INTEGER) &TotalBytes, (PULARGE_INTEGER) &FreeBytes);
	return FreeBytes;
#else
	return -1;
#endif
}

}}
