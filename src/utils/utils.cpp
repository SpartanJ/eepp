#include "utils.hpp"
#include "string.hpp"

#if EE_PLATFORM == EE_PLATFORM_APPLE
	#include <CoreFoundation/CoreFoundation.h>
	#include <sys/sysctl.h>
#elif EE_PLATFORM == EE_PLATFORM_WIN32
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
#elif EE_PLATFORM == EE_PLATFORM_LINUX
	#include <libgen.h>
	#include <unistd.h>
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN32
    #include <direct.h>
#else
	#include <sys/stat.h>
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN32
	#include <sys/utime.h>
#else
	#include <sys/time.h>
#endif

#ifndef EE_COMPILER_MSVC
	#include <dirent.h>
#endif

namespace EE { namespace Utils {

bool FileExists(const std::string& filepath) {
	std::fstream fs;
	fs.open( filepath.c_str(), std::ios::in );
	if( fs.is_open() ) {
		fs.close();
		return true;
	}
	return false;
}

Uint32 eeGetTicks() {
	return SDL_GetTicks();
}

void eeSleep( const Uint32& ms ) {
	SDL_Delay(ms);
}

std::string AppPath() {
#if EE_PLATFORM == EE_PLATFORM_APPLE
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

	throw CL_Error("get_exe_path failed");
#elif EE_PLATFORM == EE_PLATFORM_LINUX
	char exe_file[PATH_MAX + 1];
	int size;
	size = readlink("/proc/self/exe", exe_file, PATH_MAX);
	if (size < 0) {
		return "";
	} else {
		exe_file[size] = '\0';
		return std::string(dirname(exe_file)) + "/";
	}
#elif EE_PLATFORM == EE_PLATFORM_WIN32
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
#endif
}

std::vector<std::string> FilesGetInPath( const std::string& path ) {
	std::vector<std::string> files;

#ifdef EE_COMPILER_MSVC
	#ifdef UNICODE
		std::wstring mPath( stringTowstring( path ) );

		if ( mPath[ mPath.size() - 1 ] == L'/' || mPath[ mPath.size() - 1 ] == L'\\' ) {
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
  std::ifstream f;
  f.open(Filepath.c_str(), std::ios_base::binary | std::ios_base::in);
  if (!f.good() || f.eof() || !f.is_open()) { return 0; }
  f.seekg(0, std::ios_base::beg);
  std::ifstream::pos_type begin_pos = f.tellg();
  f.seekg(0, std::ios_base::end);
  return static_cast<Uint32>(f.tellg() - begin_pos);
}

eeDouble GetSystemTime() {
#if EE_PLATFORM == EE_PLATFORM_WIN32
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
#if EE_PLATFORM == EE_PLATFORM_WIN32
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
#if EE_PLATFORM == EE_PLATFORM_WIN32
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
	return MakeHash( reinterpret_cast<const Int8*>( &str[0] ) );
}

Uint32 MakeHash( const std::string& str ) {
	return MakeHash( reinterpret_cast<const Int8*>( &str[0] ) );
}

Uint32 MakeHash( const Int8* str ) {
	if ( NULL != str ) {
		Uint32 hash = 5381 + *str;

		while( *str ) {
			hash = *str + ( hash << 6 ) + ( hash << 16 ) - hash;
			str++;
		}

		hash += *( str - 1 );

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
		ifstream in( src.c_str() );
		ofstream out( dst.c_str() );

		if ( in.is_open() && out.is_open() ) {
			out << in.rdbuf();

			in.close();
			out.close();

			return true;
		}
	}

	return false;
}

std::string FileExtension( const std::string& filepath ) {
	std::string tstr( filepath.substr( filepath.find_last_of(".") + 1 ) );
	toLower( tstr );
	return tstr;
}

eeInt GetNumCPUs() {
	eeInt nprocs = -1;

	#if EE_PLATFORM == EE_PLATFORM_WIN32
		SYSTEM_INFO info;
		GetSystemInfo(&info);

		nprocs = (eeInt) info.dwNumberOfProcessors;
	#elif EE_PLATFORM == EE_PLATFORM_LINUX
		nprocs = sysconf(_SC_NPROCESSORS_ONLN);
	#elif EE_PLATFORM == EE_PLATFORM_APPLE
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
	#else
		#warning GetNumCPUs not implemented for this platform
	#endif

	if ( nprocs < 0 )
		nprocs = 1;

	return nprocs;
}

}}
