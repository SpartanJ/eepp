#include <eepp/system/sys.hpp>
#include <eepp/system/filesystem.hpp>
#include <ctime>
#include <cstdlib>
#include <climits>
#include <ctype.h>

// This taints the System module!
#if EE_PLATFORM == EE_PLATFORM_ANDROID
#include <eepp/window/cengine.hpp>
#endif

#if defined( EE_PLATFORM_POSIX )
	#include <sys/utsname.h>

	#if EE_PLATFORM != EE_PLATFORM_ANDROID
		#include <sys/statvfs.h>
	#else
		#include <sys/vfs.h>
		#define statvfs statfs
		#define fstatvfs fstatfs
	#endif
#endif

#if EE_PLATFORM == EE_PLATFORM_MACOSX
	#include <CoreFoundation/CoreFoundation.h>
#elif EE_PLATFORM == EE_PLATFORM_WIN
	#include <windows.h>
	#undef GetDiskFreeSpace
	#undef GetTempPath
#elif EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_ANDROID
	#include <libgen.h>
	#include <unistd.h>
#elif EE_PLATFORM == EE_PLATFORM_HAIKU
	#include <kernel/OS.h>
	#include <kernel/image.h>
#elif EE_PLATFORM == EE_PLATFORM_SOLARIS
	#include <stdlib.h>
#endif

#if EE_PLATFORM == EE_PLATFORM_MACOSX || EE_PLATFORM == EE_PLATFORM_BSD || EE_PLATFORM == EE_PLATFORM_IOS
	#include <sys/sysctl.h>
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN
	#include <direct.h>
	#include <sys/utime.h>
#else
	#include <sys/time.h>
#endif

namespace EE { namespace System {

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

			if ( osvi.dwMinorVersion == 2 ) {
				if( osvi.wProductType == VER_NT_WORKSTATION ) {
					os += "Windows 8";
				} else {
					os += "Windows Server 2012";
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
		#if defined( EE_COMPILER_MSVC ) && defined( UNICODE )
		std::string CSDVer( EE::String( osvi.szCSDVersion ).ToUtf8() );
		#else
		std::string CSDVer( osvi.szCSDVersion );
		#endif

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

#ifdef EE_PLATFORM_POSIX

#ifdef EE_HAVE_CLOCK_GETTIME
static struct timespec start;
#else
static struct timeval start;
#endif

#endif

std::string Sys::GetOSName() {
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

std::string Sys::GetOSArchitecture() {
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

static void eeStartTicks() {
	static bool TickStarted = false;

	if ( !TickStarted ) {
	#if EE_PLATFORM == EE_PLATFORM_WIN
		QueryPerformanceFrequency(&hires_ticks_per_second);
		QueryPerformanceCounter(&hires_start_ticks);
	#else
		#ifdef EE_HAVE_CLOCK_GETTIME
		clock_gettime(CLOCK_MONOTONIC, &start);
		#else
		gettimeofday(&start, NULL);
		#endif
	#endif

		TickStarted = true;
	}
}

Uint32 Sys::GetTicks() {
	eeStartTicks();

#if EE_PLATFORM == EE_PLATFORM_WIN
	LARGE_INTEGER hires_now;

	QueryPerformanceCounter(&hires_now);

	hires_now.QuadPart -= hires_start_ticks.QuadPart;
	hires_now.QuadPart *= 1000;
	hires_now.QuadPart /= hires_ticks_per_second.QuadPart;

	return (DWORD) hires_now.QuadPart;
#elif defined( EE_PLATFORM_POSIX )
	#ifdef EE_HAVE_CLOCK_GETTIME
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

void Sys::Sleep( const Uint32& ms ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	::Sleep( ms );
#elif defined( EE_PLATFORM_POSIX )
	// usleep( static_cast<unsigned long>( ms * 1000 ) );

	// usleep is not reliable enough (it might block the
	// whole process instead of just the current thread)
	// so we must use pthread_cond_timedwait instead

	// this implementation is inspired from Qt
	// and taken from SFML

	Uint64 usecs = ms * 1000;

	// get the current time
	timeval tv;
	gettimeofday(&tv, NULL);

	// construct the time limit (current time + time to wait)
	timespec ti;
	ti.tv_nsec = (tv.tv_usec + (usecs % 1000000)) * 1000;
	ti.tv_sec = tv.tv_sec + (usecs / 1000000) + (ti.tv_nsec / 1000000000);
	ti.tv_nsec %= 1000000000;

	// create a mutex and thread condition
	pthread_mutex_t mutex;
	pthread_mutex_init(&mutex, 0);
	pthread_cond_t condition;
	pthread_cond_init(&condition, 0);

	// wait...
	pthread_mutex_lock(&mutex);
	pthread_cond_timedwait(&condition, &mutex, &ti);
	pthread_mutex_unlock(&mutex);

	// destroy the mutex and condition
	pthread_cond_destroy(&condition);
	pthread_mutex_destroy(&mutex);

#else
	#warning Sys::Sleep not implemented in this platform.
#endif
}

static std::string sGetProcessPath() {
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
	if ( NULL != Window::cEngine::instance() && NULL != Window::cEngine::instance()->GetCurrentWindow() )
		return Window::cEngine::instance()->GetCurrentWindow()->GetExternalStoragePath();

	return "/sdcard/";
#else
	#warning Sys::GetProcessPath() not implemented on this platform. ( will return "./" )
	return "./";
#endif
}

std::string Sys::GetProcessPath() {
	static std::string path = sGetProcessPath();
	return path;
}

eeDouble Sys::GetSystemTime() {
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

std::string Sys::GetDateTimeStr() {
	time_t rawtime;
	time ( &rawtime );

#ifdef EE_COMPILER_MSVC
	char buf[256];
	struct tm timeinfo;
	localtime_s ( &timeinfo, &rawtime );
	asctime_s( &buf[0], 256, &timeinfo );
	return std::string( buf );
#else
	struct tm * timeinfo;
	timeinfo = localtime ( &rawtime );
	return std::string( asctime (timeinfo) );
#endif
}

#define EE_MAX_CFG_PATH_LEN 1024
std::string Sys::GetConfigPath( std::string appname ) {
	char path[EE_MAX_CFG_PATH_LEN];

	#if EE_PLATFORM == EE_PLATFORM_WIN
		#ifdef EE_COMPILER_MSVC

		char * ppath;
		size_t ssize = EE_MAX_CFG_PATH_LEN;

		_dupenv_s( &ppath, &ssize, "APPDATA" );

		String::StrCopy( path, ppath, EE_MAX_CFG_PATH_LEN );

		free( ppath );

		if( !ssize )
			return std::string();

		#else
		char * home = getenv("APPDATA");

		if( !home )
			return std::string();

		_snprintf(path, EE_MAX_CFG_PATH_LEN, "%s\\%s", home, appname.c_str() );

		#endif
	#elif EE_PLATFORM == EE_PLATFORM_MACOSX
		char *home = getenv("HOME");

		if( NULL == home ) {
			return std::string();
		}

		snprintf(path, EE_MAX_CFG_PATH_LEN, "%s/Library/Application Support/%s", home, appname.c_str() );
	#elif EE_PLATFORM == EE_PLATFORM_HAIKU
		char *home = getenv("HOME");

		if( NULL == home ) {
			return std::string();
		}

		snprintf(path, EE_MAX_CFG_PATH_LEN, "%s/config/settings/%s", home, appname.c_str() );
	#elif EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD || EE_PLATFORM == EE_PLATFORM_SOLARIS
		char * config = getenv("XDG_CONFIG_HOME");

		if ( NULL != config ) {
			snprintf(path, EE_MAX_CFG_PATH_LEN, "%s/%s", config, appname.c_str() );
		} else {
			char *home = getenv("HOME");

			if( NULL == home ) {
				return std::string();
			}

			snprintf(path, EE_MAX_CFG_PATH_LEN, "%s/.config/%s", home, appname.c_str() );
		}
	#elif EE_PLATFORM == EE_PLATFORM_IOS
		return GetProcessPath() + "config";
	#elif EE_PLATFORM == EE_PLATFORM_ANDROID
		if ( NULL != Window::cEngine::instance() )
			return Window::cEngine::instance()->GetCurrentWindow()->GetInternalStoragePath();

		return std::string();
	#else
		#warning Sys::GetConfigPath not implemented for this platform ( it will use HOME directory + /.appname )

		char *home = getenv("HOME");

		if( NULL == home ) {
			return std::string();
		}

		snprintf(path, EE_MAX_CFG_PATH_LEN, "%s/.%s", home, appname.c_str() );
    #endif

	return std::string( path );
}

std::string Sys::GetTempPath() {
	char path[EE_MAX_CFG_PATH_LEN];

	#if EE_PLATFORM == EE_PLATFORM_WIN
		DWORD dwRetVal = GetTempPathA(EE_MAX_CFG_PATH_LEN, path);

		if ( 0 <= dwRetVal || dwRetVal > EE_MAX_CFG_PATH_LEN ) {
			return std::string( "C:\\WINDOWS\\TEMP\\" );
		}
	#elif EE_PLATFORM == EE_PLATFORM_ANDROID
		if ( NULL != Window::cEngine::instance() ) {
			String::StrCopy( path, Window::cEngine::instance()->GetCurrentWindow()->GetInternalStoragePath().c_str(), EE_MAX_CFG_PATH_LEN );
		} else {
			String::StrCopy( path, "/tmp", EE_MAX_CFG_PATH_LEN );
		}
	#else
		char * tmpdir = getenv("TMPDIR");

		if ( NULL != tmpdir ) {
			String::StrCopy( path, tmpdir, EE_MAX_CFG_PATH_LEN );
		} else {
			String::StrCopy( path, "/tmp", EE_MAX_CFG_PATH_LEN );
		}
	#endif

	std::string rpath( path );

	FileSystem::DirPathAddSlashAtEnd( rpath );

	return rpath;
}

eeInt Sys::GetCPUCount() {
	eeInt nprocs = -1;

	#if EE_PLATFORM == EE_PLATFORM_WIN
		SYSTEM_INFO info;
		GetSystemInfo(&info);

		nprocs = (eeInt) info.dwNumberOfProcessors;
	#elif EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_SOLARIS || EE_PLATFORM == EE_PLATFORM_ANDROID
		nprocs = sysconf(_SC_NPROCESSORS_ONLN);
	#elif EE_PLATFORM == EE_PLATFORM_MACOSX || EE_PLATFORM == EE_PLATFORM_BSD || EE_PLATFORM == EE_PLATFORM_IOS
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
		#warning Sys::GetCPUCount not implemented on this platform ( it will return 1 ).
	#endif

	if ( nprocs < 1 )
		nprocs = 1;

	return nprocs;
}

Int64 Sys::GetDiskFreeSpace(const std::string& path) {
#if defined( EE_PLATFORM_POSIX )
	struct statvfs data;
	statvfs(path.c_str(),  &data);
	#if EE_PLATFORM != EE_PLATFORM_MACOSX
	return (Int64)data.f_bsize * (Int64)data.f_bfree;
	#else
	return (Int64)data.f_frsize * (Int64)data.f_bfree;
	#endif
#elif EE_PLATFORM == EE_PLATFORM_WIN
	Int64 AvailableBytes;
	Int64 TotalBytes;
	Int64 FreeBytes;
	#ifdef UNICODE
	GetDiskFreeSpaceEx((LPCWSTR)path.c_str(),(PULARGE_INTEGER) &AvailableBytes,
	#else
	GetDiskFreeSpaceEx(path.c_str(),(PULARGE_INTEGER) &AvailableBytes,
	#endif
	(PULARGE_INTEGER) &TotalBytes, (PULARGE_INTEGER) &FreeBytes);

	return FreeBytes;
#else
	return -1;
#endif
}

}}
