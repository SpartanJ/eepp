#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <ctype.h>
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/sys.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

// This taints the System module!
#if EE_PLATFORM == EE_PLATFORM_ANDROID
#include <eepp/window/engine.hpp>
#endif

#if defined( EE_PLATFORM_POSIX )
#include <dirent.h>
#include <dlfcn.h>
#include <sys/utsname.h>
#include <unistd.h>
#endif

#if EE_PLATFORM == EE_PLATFORM_MACOS
#include <CoreFoundation/CoreFoundation.h>
#include <libproc.h>
#include <unistd.h>
#elif EE_PLATFORM == EE_PLATFORM_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <stdlib.h>
#include <windows.h>
#undef GetDiskFreeSpace
#undef GetTempPath

// clang-format off
#include <psapi.h>
#include <tlhelp32.h>
// clang-format on

// Dynamically load PSAPI functions for Windows
typedef BOOL( WINAPI* EnumProcesses_t )( DWORD*, DWORD, DWORD* );
typedef BOOL( WINAPI* EnumProcessModules_t )( HANDLE, HMODULE*, DWORD, LPDWORD );
typedef DWORD( WINAPI* GetModuleBaseName_t )( HANDLE, HMODULE, LPSTR, DWORD );

#elif EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_ANDROID
#include <libgen.h>
#include <mntent.h>
#include <sys/sysinfo.h>
#elif EE_PLATFORM == EE_PLATFORM_HAIKU
#include <Directory.h>
#include <OS.h>
#include <Path.h>
#include <Volume.h>
#include <VolumeRoster.h>
#include <fs_info.h>
#include <kernel/OS.h>
#include <kernel/image.h>
#include <sys/statvfs.h>
#elif EE_PLATFORM == EE_PLATFORM_SOLARIS
#include <stdlib.h>
#elif EE_PLATFORM == EE_PLATFORM_BSD
#include <sys/sysctl.h>
#include <sys/types.h>
#include <sys/user.h>
#endif

#if EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_IOS
#include <libproc.h>
#include <mach-o/dyld.h>
#include <spawn.h>
#endif

#if EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_BSD || \
	EE_PLATFORM == EE_PLATFORM_IOS
#include <sys/mount.h>
#include <sys/sysctl.h>
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <direct.h>
#include <sys/utime.h>
#include <tchar.h>
#else
#include <sys/time.h>
#endif

#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
#include <eepp/network/uri.hpp>
#include <emscripten.h>
using namespace EE::Network;
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

std::unordered_map<std::string, std::string> _getEnvironmentVariables() {
	std::unordered_map<std::string, std::string> ret;
	char** env;
#if defined( WIN ) && ( _MSC_VER >= 1900 )
	env = *__p__environ();
#else
	extern char** environ;
	env = environ;
#endif

	for ( ; *env; ++env ) {
		auto var = EE::String::split( *env, "=" );

		if ( var.size() == 2 ) {
			ret.insert( std::make_pair( var[0], var[1] ) );
		} else if ( var.size() > 2 ) {
			auto val( var[1] );
			for ( size_t i = 2; i < var.size(); ++i )
				val += var[i];
			ret.insert( std::make_pair( var[0], val ) );
		}
	}

	return ret;
}

namespace EE { namespace System {

#if EE_PLATFORM == EE_PLATFORM_WIN

typedef void( WINAPI* PGNSI )( LPSYSTEM_INFO );
typedef void( WINAPI* RtlGetVersion_FUNC )( OSVERSIONINFOEXW* );

BOOL GetWindowsVersion( OSVERSIONINFOEX* os ) {
	HMODULE hMod;
	RtlGetVersion_FUNC func;
#ifdef UNICODE
	OSVERSIONINFOEXW* osw = os;
#else
	OSVERSIONINFOEXW o;
	OSVERSIONINFOEXW* osw = &o;
#endif

	hMod = LoadLibrary( TEXT( "ntdll.dll" ) );
	if ( hMod ) {
		func = (RtlGetVersion_FUNC)GetProcAddress( hMod, "RtlGetVersion" );

		if ( func == NULL ) {
			FreeLibrary( hMod );
			return FALSE;
		}

		ZeroMemory( osw, sizeof( *osw ) );
		osw->dwOSVersionInfoSize = sizeof( *osw );
		func( osw );

#ifndef UNICODE
		os->dwBuildNumber = osw->dwBuildNumber;
		os->dwMajorVersion = osw->dwMajorVersion;
		os->dwMinorVersion = osw->dwMinorVersion;
		os->dwOSVersionInfoSize = sizeof( *os );
		os->dwPlatformId = osw->dwPlatformId;
		WCHAR* src = osw->szCSDVersion;
		unsigned char* dtc = (unsigned char*)os->szCSDVersion;
		while ( *src )
			*dtc++ = (unsigned char)*src++;
		*dtc = '\0';
		os->wProductType = osw->wProductType;
		os->wReserved = osw->wReserved;
		os->wServicePackMajor = osw->wServicePackMajor;
		os->wServicePackMinor = osw->wServicePackMinor;
		os->wSuiteMask = osw->wSuiteMask;
#endif

	} else {
		return FALSE;
	}

	FreeLibrary( hMod );

	return TRUE;
}

static std::string GetWindowsArch() {
	std::string arch = "Unknown";
	OSVERSIONINFOEX osvi;
	SYSTEM_INFO si;
	PGNSI pGNSI;
	BOOL bOsVersionInfoEx = TRUE;

	ZeroMemory( &si, sizeof( SYSTEM_INFO ) );
	ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );

	osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );

	if ( !GetWindowsVersion( &osvi ) ) {
		bOsVersionInfoEx = GetVersionEx( (OSVERSIONINFO*)&osvi );
	}

	if ( bOsVersionInfoEx == FALSE )
		return arch;

	// Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.
	pGNSI =
		(PGNSI)GetProcAddress( GetModuleHandle( TEXT( "kernel32.dll" ) ), "GetNativeSystemInfo" );

	if ( NULL != pGNSI ) {
		pGNSI( &si );
	} else {
		GetSystemInfo( &si );
	}

	if ( si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
		 si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64 ) {
		arch = "x86_64";
	} else if ( si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ) {
		arch = "x86";
	} else if ( si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64 ) {
		arch = "arm64";
	} else if ( si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM ) {
		arch = "arm";
	}

	return arch;
}

static std::string GetWindowsVersion() {
	std::string os;
	OSVERSIONINFOEX osvi;
	PGNSI pGNSI;
	BOOL bOsVersionInfoEx;

	ZeroMemory( &osvi, sizeof( OSVERSIONINFOEX ) );

	osvi.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );

	bOsVersionInfoEx = GetWindowsVersion( &osvi );
	if ( !bOsVersionInfoEx ) {
		bOsVersionInfoEx = GetVersionEx( (OSVERSIONINFO*)&osvi );
	}

	if ( bOsVersionInfoEx == FALSE )
		return os;

	os = "Microsoft ";

	if ( VER_PLATFORM_WIN32_NT == osvi.dwPlatformId && osvi.dwMajorVersion > 4 ) {
		if ( osvi.dwMajorVersion == 10 ) {
			if ( osvi.dwMinorVersion == 0 ) {
				if ( osvi.wProductType == VER_NT_WORKSTATION ) {
					if ( osvi.dwBuildNumber >= 22000 ) {
						os += "Windows 11";
					} else {
						os += "Windows 10";
					}
				} else {
					os += "Windows Server 2016";
				}
			}
		}

		if ( osvi.dwMajorVersion == 6 ) {
			if ( osvi.dwMinorVersion == 0 ) {
				if ( osvi.wProductType == VER_NT_WORKSTATION ) {
					os += "Windows Vista";
				} else {
					os += "Windows Server 2008";
				}
			}

			if ( osvi.dwMinorVersion == 1 ) {
				if ( osvi.wProductType == VER_NT_WORKSTATION ) {
					os += "Windows 7";
				} else {
					os += "Windows Server 2008 R2";
				}
			}

			if ( osvi.dwMinorVersion == 2 ) {
				if ( osvi.wProductType == VER_NT_WORKSTATION ) {
					os += "Windows 8";
				} else {
					os += "Windows Server 2012";
				}
			}

			if ( osvi.dwMinorVersion == 3 ) {
				if ( osvi.wProductType == VER_NT_WORKSTATION ) {
					os += "Windows 8.1";
				} else {
					os += "Windows Server 2012 R2";
				}
			}
		}

		if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 ) {
			// Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.
			pGNSI = (PGNSI)GetProcAddress( GetModuleHandle( TEXT( "kernel32.dll" ) ),
										   "GetNativeSystemInfo" );

			SYSTEM_INFO si;
			ZeroMemory( &si, sizeof( SYSTEM_INFO ) );

			if ( NULL != pGNSI ) {
				pGNSI( &si );
			} else {
				GetSystemInfo( &si );
			}

			if ( GetSystemMetrics( SM_SERVERR2 ) ) {
				os += "Windows Server 2003 R2, ";
			} else if ( osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER ) {
				os += "Windows Storage Server 2003";
			} else if ( osvi.wSuiteMask & 0x00008000 ) { // VER_SUITE_WH_SERVER
				os += "Windows Home Server";
			} else if ( osvi.wProductType == VER_NT_WORKSTATION &&
						si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ) {
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
		std::string CSDVer( EE::String( osvi.szCSDVersion ).toUtf8() );
#else
		std::string CSDVer( osvi.szCSDVersion );
#endif

		if ( CSDVer.size() ) {
			os += " " + CSDVer;
		}
	} else if ( VER_PLATFORM_WIN32_WINDOWS == osvi.dwPlatformId ) {
		if ( osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 0 ) {
			if ( osvi.szCSDVersion[1] == 'C' || osvi.szCSDVersion[1] == 'B' ) {
				os += "Windows 95 OSR2";
			} else {
				os += "Windows 95";
			}
		} else if ( osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 10 ) {
			if ( osvi.szCSDVersion[1] == 'A' ) {
				os += "Windows 98 SE";
			} else {
				os += "Windows 98";
			}
		} else if ( osvi.dwMajorVersion == 4 && osvi.dwMinorVersion == 90 ) {
			os += "Windows ME";
		} else if ( osvi.dwMajorVersion == 4 ) {
			os += "Windows unknown 95 family";
		} else {
			os += "Windows";
		}
	} else {
		os += "Windows";
	}

	return os;
}

#define TIME_WRAP_VALUE ( ~(DWORD)0 )
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

std::string Sys::getOSName( bool showReleaseName ) {
#if defined( EE_PLATFORM_POSIX )
	struct utsname os;

	if ( -1 != uname( &os ) ) {
		return std::string( os.sysname ) +
			   ( showReleaseName ? " " + std::string( os.release ) : "" );
	}

	return "Unknown";
#elif EE_PLATFORM == EE_PLATFORM_WIN
	return showReleaseName ? GetWindowsVersion() : "Windows";
#else
	return "Unknown";
#endif
}

std::string Sys::getOSArchitecture() {
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

Sys::PlatformType Sys::getPlatformType() {
#if EE_PLATFORM == EE_PLATFORM_LINUX
	return Sys::PlatformType::Linux;
#elif EE_PLATFORM == EE_PLATFORM_ANDROID
	return Sys::PlatformType::Android;
#elif EE_PLATFORM == EE_PLATFORM_BSD
#if defined( __FreeBSD__ )
	return Sys::PlatformType::FreeBSD;
#elif defined( __OpenBSD__ )
	return Sys::PlatformType::OpenBSD;
#elif defined( __NetBSD__ )
	return Sys::PlatformType::NetBSD;
#elif defined( __DragonFly__ )
	return Sys::PlatformType::DragonFlyBSD;
#else
	return Sys::PlatformType::BSD;
#endif
#elif EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	return Sys::PlatformType::Emscripten;
#elif EE_PLATFORM == EE_PLATFORM_HAIKU
	return Sys::PlatformType::Haiku;
#elif EE_PLATFORM == EE_PLATFORM_IOS
	return Sys::PlatformType::iOS;
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	return Sys::PlatformType::macOS;
#elif EE_PLATFORM == EE_PLATFORM_SOLARIS
	return Sys::PlatformType::Solaris;
#elif EE_PLATFORM_WIN
	return Sys::PlatformType::Windows;
#else
	return Sys::PlatformType::Unknown;
#endif
}

std::string Sys::getPlatform() {
#if EE_PLATFORM == EE_PLATFORM_LINUX
	return "Linux";
#elif EE_PLATFORM == EE_PLATFORM_ANDROID
	return "Android";
#elif EE_PLATFORM == EE_PLATFORM_BSD
#if defined( __FreeBSD__ )
	return "FreeBSD";
#elif defined( __OpenBSD__ )
	return "OpenBSD";
#elif defined( __NetBSD__ )
	return "NetBSD";
#elif defined( __DragonFly__ )
	return "DragonFlyBSD";
#else
	return "BSD";
#endif
#elif EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	return "Emscripten";
#elif EE_PLATFORM == EE_PLATFORM_HAIKU
	return "Haiku";
#elif EE_PLATFORM == EE_PLATFORM_IOS
	return "iOS";
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	return "macOS";
#elif EE_PLATFORM == EE_PLATFORM_SOLARIS
	return "Solaris";
#elif EE_PLATFORM_WIN
	return "Windows";
#else
	return "Unknown";
#endif
}

static void eeStartTicks() {
	static bool TickStarted = false;

	if ( !TickStarted ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
		QueryPerformanceFrequency( &hires_ticks_per_second );
		QueryPerformanceCounter( &hires_start_ticks );
#else
#ifdef EE_HAVE_CLOCK_GETTIME
		clock_gettime( CLOCK_MONOTONIC, &start );
#else
		gettimeofday( &start, NULL );
#endif
#endif

		TickStarted = true;
	}
}

Uint64 Sys::getTicks() {
	eeStartTicks();

#if EE_PLATFORM == EE_PLATFORM_WIN
	LARGE_INTEGER hires_now;

	QueryPerformanceCounter( &hires_now );

	hires_now.QuadPart -= hires_start_ticks.QuadPart;
	hires_now.QuadPart *= 1000;
	hires_now.QuadPart /= hires_ticks_per_second.QuadPart;

	return (Uint64)hires_now.QuadPart;
#elif defined( EE_PLATFORM_POSIX )
#ifdef EE_HAVE_CLOCK_GETTIME
	Uint64 ticks;
	struct timespec now;
	clock_gettime( CLOCK_MONOTONIC, &now );
	ticks = ( now.tv_sec - start.tv_sec ) * 1000 + ( now.tv_nsec - start.tv_nsec ) / 1000000;
	return ( ticks );
#else
	Uint64 ticks;
	struct timeval now;
	gettimeofday( &now, NULL );
	ticks = ( now.tv_sec - start.tv_sec ) * 1000 + ( now.tv_usec - start.tv_usec ) / 1000;
	return ( ticks );
#endif
#else
#warning Sys::getTicks() not implemented in this platform.
#endif
}

void Sys::sleep( const Time& time ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	HANDLE timer;
	LARGE_INTEGER ft;

	ft.QuadPart = -( 10 * time.asMicroseconds() ); // Convert to 100 nanosecond interval, negative
												   // value indicates relative time

	timer = CreateWaitableTimer( NULL, TRUE, NULL );
	SetWaitableTimer( timer, &ft, 0, NULL, NULL, 0 );
	WaitForSingleObject( timer, INFINITE );
	CloseHandle( timer );
#elif defined( EE_PLATFORM_POSIX )
	Uint64 usecs = time.asMicroseconds();

	// Construct the time to wait
	timespec ti;
	ti.tv_nsec = ( usecs % 1000000 ) * 1000;
	ti.tv_sec = usecs / 1000000;

	// Wait...
	// If nanosleep returns -1, we check errno. If it is EINTR
	// nanosleep was interrupted and has set ti to the remaining
	// duration. We continue sleeping until the complete duration
	// has passed. We stop sleeping if it was due to an error.
	while ( ( nanosleep( &ti, &ti ) == -1 ) && ( errno == EINTR ) ) {
	}
#else
#warning Sys::sleep() not implemented in this platform.
#endif
}

static std::string sGetProcessPath() {
#if EE_PLATFORM == EE_PLATFORM_MACOS
	char pathbuf[PROC_PIDPATHINFO_MAXSIZE];
	pid_t pid = getpid();
	int ret = proc_pidpath( pid, pathbuf, sizeof( pathbuf ) );
	if ( ret >= 0 )
		return FileSystem::fileRemoveFileName( std::string( pathbuf ) );

	char exe_file[PATH_MAX + 1];
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	if ( mainBundle ) {
		CFURLRef mainURL = CFBundleCopyBundleURL( mainBundle );

		if ( mainURL ) {
			int ok = CFURLGetFileSystemRepresentation( mainURL, ( Boolean ) true, (UInt8*)exe_file,
													   PATH_MAX );

			if ( ok ) {
				return std::string( exe_file ) + "/";
			}
		}
	}

	return "./";
#elif EE_PLATFORM == EE_PLATFORM_LINUX
	char exe_file[PATH_MAX + 1];
	int size;
	size = readlink( "/proc/self/exe", exe_file, PATH_MAX );
	if ( size < 0 ) {
		return "./";
	} else {
		exe_file[size] = '\0';
		return std::string( dirname( exe_file ) ) + "/";
	}
#elif EE_PLATFORM == EE_PLATFORM_WIN
#ifdef UNICODE
	// Get path to executable:
	char szDrive[_MAX_DRIVE];
	char szDir[_MAX_DIR];
	char szFilename[_MAX_DIR];
	char szExt[_MAX_DIR];
	std::wstring dllName( _MAX_DIR, 0 );

	GetModuleFileName( 0, &dllName[0], _MAX_PATH );

	std::string dllstrName( String( dllName ).toUtf8() );

#ifdef EE_COMPILER_MSVC
	_splitpath_s( dllstrName.c_str(), szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFilename, _MAX_DIR,
				  szExt, _MAX_DIR );
#else
	_splitpath( szDllName, szDrive, szDir, szFilename, szExt );
#endif

	return std::string( szDrive ) + std::string( szDir );
#else
	// Get path to executable:
	TCHAR szDllName[_MAX_PATH];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFilename[_MAX_DIR];
	TCHAR szExt[_MAX_DIR];
	GetModuleFileName( 0, szDllName, _MAX_PATH );

#ifdef EE_COMPILER_MSVC
	_splitpath_s( szDllName, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, szFilename, _MAX_DIR, szExt,
				  _MAX_DIR );
#else
	_splitpath( szDllName, szDrive, szDir, szFilename, szExt );
#endif

	return std::string( szDrive ) + std::string( szDir );
#endif
#elif EE_PLATFORM == EE_PLATFORM_BSD
	int mib[4];
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_PATHNAME;
	mib[3] = -1;
	char buf[1024];
	size_t cb = sizeof( buf );
	sysctl( mib, 4, buf, &cb, NULL, 0 );

	return FileSystem::fileRemoveFileName( std::string( buf ) );
#elif EE_PLATFORM == EE_PLATFORM_SOLARIS
	return fileRemoveFileName( std::string( getexecname() ) );
#elif EE_PLATFORM == EE_PLATFORM_HAIKU
	image_info info;
	int32 cookie = 0;

	while ( B_OK == get_next_image_info( 0, &cookie, &info ) ) {
		if ( info.type == B_APP_IMAGE )
			break;
	}

	return FileSystem::fileRemoveFileName( std::string( info.name ) );
#elif EE_PLATFORM == EE_PLATFORM_ANDROID
	return Window::Engine::instance()->getPlatformHelper()->getExternalStoragePath() + "/";
#else
#warning Sys::getProcessPath() not implemented on this platform. ( will return "./" )
	return "./";
#endif
}

std::string Sys::getProcessPath() {
	static std::string path = sGetProcessPath();
	return path;
}

double Sys::getSystemTime() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	static LARGE_INTEGER Frequency;
	static BOOL UseHighPerformanceTimer = QueryPerformanceFrequency( &Frequency );

	if ( UseHighPerformanceTimer ) {
		// High performance counter available : use it
		LARGE_INTEGER CurrentTime;
		QueryPerformanceCounter( &CurrentTime );
		return static_cast<double>( CurrentTime.QuadPart ) / Frequency.QuadPart;
	} else
		// High performance counter not available : use GetTickCount (less accurate)
		return GetTickCount() * 0.001;
#else
	timeval Time = { 0, 0 };
	gettimeofday( &Time, NULL );

	return Time.tv_sec + Time.tv_usec / 1000000.;
#endif
}

Uint64 Sys::getProcessID() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return GetCurrentProcessId();
#elif EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	return getpid();
#elif EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	return 0; // just return 0
#else
#warning Sys::getProcessID() not implemented in this platform
#endif
}

std::string Sys::getDateTimeStr() {
	time_t rawtime;
	time( &rawtime );

	char buf[64];

	struct tm* timeinfo;
	timeinfo = localtime( &rawtime );

	strftime( buf, sizeof( buf ), "%Y-%m-%d %X", timeinfo );

	return std::string( buf );
}

std::string Sys::epochToString( const Uint64& epochTimestamp, const std::string& format ) {
	std::time_t t = epochTimestamp;
	auto tm = *std::localtime( &t );
	std::ostringstream oss;
	oss << std::put_time( &tm, format.c_str() );
	return oss.str();
}

#define EE_MAX_CFG_PATH_LEN 1024
std::string Sys::getConfigPath( const std::string& appname ) {
	char path[EE_MAX_CFG_PATH_LEN];

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifdef EE_COMPILER_MSVC

	char* ppath;
	size_t ssize = EE_MAX_CFG_PATH_LEN;

	_dupenv_s( &ppath, &ssize, "APPDATA" );

	_snprintf( path, EE_MAX_CFG_PATH_LEN, "%s\\%s", ppath, appname.c_str() );

	free( ppath );

	if ( !ssize )
		return std::string();

#else
	char* home = getenv( "APPDATA" );

	if ( !home )
		return std::string();

	_snprintf( path, EE_MAX_CFG_PATH_LEN, "%s\\%s", home, appname.c_str() );

#endif
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	char* home = getenv( "HOME" );

	if ( NULL == home ) {
		return std::string();
	}

	snprintf( path, EE_MAX_CFG_PATH_LEN, "%s/Library/Application Support/%s", home,
			  appname.c_str() );
#elif EE_PLATFORM == EE_PLATFORM_HAIKU
	char* home = getenv( "HOME" );

	if ( NULL == home ) {
		return std::string();
	}

	snprintf( path, EE_MAX_CFG_PATH_LEN, "%s/config/settings/%s", home, appname.c_str() );
#elif EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_BSD || \
	EE_PLATFORM == EE_PLATFORM_SOLARIS
	char* config = getenv( "XDG_CONFIG_HOME" );

	if ( NULL != config ) {
		snprintf( path, EE_MAX_CFG_PATH_LEN, "%s/%s", config, appname.c_str() );
	} else {
		char* home = getenv( "HOME" );

		if ( NULL == home ) {
			return std::string();
		}

		snprintf( path, EE_MAX_CFG_PATH_LEN, "%s/.config/%s", home, appname.c_str() );
	}
#elif EE_PLATFORM == EE_PLATFORM_IOS
	return getProcessPath() + "config";
#elif EE_PLATFORM == EE_PLATFORM_ANDROID
	return Window::Engine::instance()->getPlatformHelper()->getInternalStoragePath() + "/";
#else
#warning Sys::getConfigPath not implemented for this platform ( it will use HOME directory + /.appname )

	char* home = getenv( "HOME" );

	if ( NULL == home ) {
		return std::string();
	}

	snprintf( path, EE_MAX_CFG_PATH_LEN, "%s/.%s", home, appname.c_str() );
#endif

	return std::string( path );
}

std::string Sys::getTempPath() {
	char path[EE_MAX_CFG_PATH_LEN];

#if EE_PLATFORM == EE_PLATFORM_WIN
	DWORD dwRetVal = GetTempPathA( EE_MAX_CFG_PATH_LEN, path );

	if ( 0 <= dwRetVal || dwRetVal > EE_MAX_CFG_PATH_LEN ) {
		return std::string( "C:\\WINDOWS\\TEMP\\" );
	}
#elif EE_PLATFORM == EE_PLATFORM_ANDROID
	String::strCopy(
		path,
		std::string( Window::Engine::instance()->getPlatformHelper()->getInternalStoragePath() +
					 "/tmp/" )
			.c_str(),
		EE_MAX_CFG_PATH_LEN );
#else
	char* tmpdir = getenv( "TMPDIR" );

	if ( NULL != tmpdir ) {
		String::strCopy( path, tmpdir, EE_MAX_CFG_PATH_LEN );
	} else {
		String::strCopy( path, "/tmp", EE_MAX_CFG_PATH_LEN );
	}
#endif

	std::string rpath( path );

	FileSystem::dirAddSlashAtEnd( rpath );

	return rpath;
}

int Sys::getCPUCount() {
	int nprocs = -1;

#if EE_PLATFORM == EE_PLATFORM_WIN
	SYSTEM_INFO info;
	GetSystemInfo( &info );

	nprocs = (int)info.dwNumberOfProcessors;
#elif EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_SOLARIS || \
	EE_PLATFORM == EE_PLATFORM_ANDROID
	nprocs = sysconf( _SC_NPROCESSORS_ONLN );
#elif EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	return EM_ASM_INT( { return navigator.hardwareConcurrency; } );
#elif EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_BSD || \
	EE_PLATFORM == EE_PLATFORM_IOS
	int mib[2];
	int maxproc = 1;
	size_t len = sizeof( int );

	mib[0] = CTL_HW;
	mib[1] = HW_NCPU;

	len = sizeof( maxproc );

	sysctl( mib, 2, &maxproc, &len, NULL, 0 );

	nprocs = maxproc;
#elif EE_PLATFORM == EE_PLATFORM_HAIKU
	system_info info;

	if ( B_OK == get_system_info( &info ) ) {
		nprocs = info.cpu_count;
	}
#else
#warning Sys::getCPUCount not implemented on this platform ( it will return 1 ).
#endif

	if ( nprocs < 1 )
		nprocs = 1;

	return nprocs;
}

#if EE_PLATFORM == EE_PLATFORM_WIN
int WIN_SetError( std::string prefix = "" ) {
	TCHAR buffer[1024];
	FormatMessage( FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 0, buffer,
				   eeARRAY_SIZE( buffer ), NULL );
	Log::warning( "%s%s%s", prefix.c_str(), !prefix.empty() ? ": " : "", buffer );
	return -1;
}
#endif

void* Sys::loadObject( const std::string& sofile ) {
#if defined( EE_PLATFORM_POSIX )
	void* handle = dlopen( sofile.c_str(), RTLD_NOW | RTLD_LOCAL );

	const char* loaderror = (char*)dlerror();

	if ( handle == NULL ) {
		Log::warning( "Failed loading %s: %s", sofile.c_str(), loaderror );
	}

	return ( handle );
#elif EE_PLATFORM == EE_PLATFORM_WIN
#ifdef UNICODE
	void* handle = (void*)LoadLibrary( String::fromUtf8( sofile ).toWideString().c_str() );
#else
	LPTSTR tstr = const_cast<char*>( sofile.c_str() );
	void* handle = (void*)LoadLibrary( tstr );
#endif
	/* Generate an error message if all loads failed */
	if ( handle == NULL ) {
		WIN_SetError( "Failed loading " + sofile );
	}
	return handle;
#else
#warning Sys::loadObject not implemented in this platform
#endif
}

void Sys::unloadObject( void* handle ) {
#if defined( EE_PLATFORM_POSIX )
	if ( handle != NULL ) {
		dlclose( handle );
	}
#elif EE_PLATFORM == EE_PLATFORM_WIN
	if ( handle != NULL ) {
		FreeLibrary( (HMODULE)handle );
	}
#else
#warning Sys::unloadObject not implemented in this platform
#endif
}

void* Sys::loadFunction( void* handle, const std::string& name ) {
#if defined( EE_PLATFORM_POSIX )
	void* symbol = dlsym( handle, name.c_str() );

	if ( symbol == NULL ) {
		/* append an underscore for platforms that need that. */
		std::string _name( "_" + name );

		symbol = dlsym( handle, _name.c_str() );

		if ( symbol == NULL ) {
			Log::warning( "Failed loading %s: %s", name.c_str(), (const char*)dlerror() );
		}
	}

	return ( symbol );
#elif EE_PLATFORM == EE_PLATFORM_WIN
	void* symbol = (void*)GetProcAddress( (HMODULE)handle, name.c_str() );

	if ( symbol == NULL ) {
		WIN_SetError( "Failed loading function " + name );
	}

	return symbol;
#else
#warning Sys::loadFunction not implemented in this platform
#endif
}

std::vector<std::string> Sys::parseArguments( int argc, char* argv[] ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	int rargc;
	LPWSTR* rargv = CommandLineToArgvW( GetCommandLineW(), &rargc );
	std::vector<std::string> args;
	if ( rargc <= 1 )
		return {};
	for ( int i = 1; i < rargc; i++ ) {
		args.emplace_back( String( rargv[i] ).toUtf8() );
	}
	return args;
#elif EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	if ( argc < 1 )
		return {};
	std::vector<std::string> args;
	for ( int i = 1; i < argc; i++ ) {
		auto split = String::split( std::string( argv[i] ), '=' );
		if ( split.size() == 2 ) {
			std::string arg( split[0] + "=" + URI::decode( split[1] ) );
			args.emplace_back( !String::startsWith( arg, "--" ) ? ( std::string( "--" ) + arg )
																: arg );
		}
	}
	return args;
#else
	std::vector<std::string> args;
	if ( argc > 1 ) {
		for ( int i = 1; i < argc; i++ )
			args.push_back( argv[i] );
	}
	return args;
#endif
}

#if EE_PLATFORM == EE_PLATFORM_WIN
static inline bool isDriveReady( const wchar_t* path ) {
	DWORD fileSystemFlags;
	const UINT driveType = GetDriveTypeW( path );
	return ( driveType != DRIVE_REMOVABLE && driveType != DRIVE_CDROM ) ||
		   GetVolumeInformationW( path, nullptr, 0, nullptr, nullptr, &fileSystemFlags, nullptr,
								  0 ) == TRUE;
}
#endif

std::vector<std::string> Sys::getLogicalDrives() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	std::vector<std::string> ret;
	const UINT oldErrorMode = ::SetErrorMode( SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX );
	Uint32 driveBits = (Uint32)GetLogicalDrives() & 0x3ffffff;
	wchar_t driveName[] = L"A:\\";

	while ( driveBits ) {
		if ( ( driveBits & 1 ) && isDriveReady( driveName ) )
			ret.emplace_back( String::fromWide( driveName ).toUtf8() );
		driveName[0]++;
		driveBits = driveBits >> 1;
	}
	::SetErrorMode( oldErrorMode );
	return ret;
#elif EE_PLATFORM == EE_PLATFORM_LINUX
	std::vector<std::string> ret;
	struct mntent* ent;
	FILE* file = setmntent( "/etc/mtab", "r" );
	if ( file == NULL ) {
		file = setmntent( "/proc/mounts", "r" );
		if ( file == NULL )
			return ret;
	}
	while ( NULL != ( ent = getmntent( file ) ) ) {
		std::string mntType( ent->mnt_type );
		if ( mntType == "rootfs" || mntType == "devtmpfs" || mntType == "tmpfs" ||
			 mntType == "devpts" || mntType == "hugetlbfs" || mntType == "mqueue" ||
			 mntType == "ramfs" || mntType == "rpc_pipefs" || mntType == "fuse.gvfsd-fuse" ||
			 mntType == "fuse.portal" || mntType == "overlay" || mntType == "nsfs" )
			continue;

		std::string mntDir( ent->mnt_dir );
		if ( String::startsWith( mntDir, "/proc" ) || String::startsWith( mntDir, "/var/run" ) ||
			 String::startsWith( mntDir, "/sys" ) || String::startsWith( mntDir, "/var/lock" ) )
			continue;

		ret.emplace_back( std::move( mntDir ) );
	}
	endmntent( file );
	return ret;
#elif EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_BSD || \
	EE_PLATFORM == EE_PLATFORM_IOS
	std::vector<std::string> ret;
	struct statfs* mounts;
	int numMounts = getmntinfo( &mounts, MNT_NOWAIT );
	if ( numMounts <= 0 )
		return ret;
	for ( int i = 0; i < numMounts; ++i ) {
		auto mnt = mounts[i];
		std::string mntType( mnt.f_mntfromname );

		if ( mntType == "devfs" || mntType == "autofs" )
			continue;

		ret.emplace_back( mnt.f_mntonname );
	}
	return ret;
#elif EE_PLATFORM == EE_PLATFORM_HAIKU
	std::vector<std::string> ret;
	BVolumeRoster mounts;
	BVolume vol;
	mounts.Rewind();
	while ( mounts.GetNextVolume( &vol ) == B_NO_ERROR ) {
		fs_info fsinfo;
		fs_stat_dev( vol.Device(), &fsinfo );
		BDirectory directory;
		BEntry entry;
		BPath path;
		status_t rc;
		rc = vol.GetRootDirectory( &directory );
		if ( rc < B_OK )
			continue;
		rc = directory.GetEntry( &entry );
		if ( rc < B_OK )
			continue;
		rc = entry.GetPath( &path );
		if ( rc < B_OK )
			continue;
		const char* str = path.Path();
		ret.emplace_back( str );
	}
	return ret;
#else
	return {};
#endif
}

#if EE_PLATFORM == EE_PLATFORM_WIN
#define PATH_SEP_CHAR ';'
#else
#define PATH_SEP_CHAR ':'
#endif
std::string Sys::which( const std::string& exeName,
						const std::vector<std::string>& customSearchPaths ) {
	if ( exeName.find_first_of( FileSystem::getOSSlash() ) != std::string::npos &&
		 FileSystem::fileExists( exeName ) )
		return exeName;

	std::vector<std::string> PATHS = getEnvSplitted( "PATH" );
#if EE_PLATFORM == EE_PLATFORM_WIN
	static std::vector<std::string> PATHEXTS = getEnvSplitted( "PATHEXT" );
	std::string exePath;
#endif

	if ( !customSearchPaths.empty() ) {
		for ( const auto& searchPath : customSearchPaths )
			PATHS.emplace_back( searchPath );
	}

#if EE_PLATFORM == EE_PLATFORM_WIN
	bool hasExtension = false;
	for ( const auto& pathExt : PATHEXTS ) {
		if ( String::endsWith( exeName, pathExt ) ) {
			hasExtension = true;
			break;
		}
	}
#endif

	for ( const auto& path : PATHS ) {
		std::string fpath( path );
		FileSystem::dirAddSlashAtEnd( fpath );
		fpath += exeName;
#if EE_PLATFORM == EE_PLATFORM_WIN
		if ( hasExtension ) {
			if ( FileSystem::fileExists( fpath ) )
				return fpath;
		} else {
			for ( const auto& pathext : PATHEXTS ) {
				exePath = fpath + pathext;
				if ( FileSystem::fileExists( exePath ) )
					return exePath;
			}
		}
#else
		if ( FileSystem::fileExists( fpath ) )
			return fpath;
#endif
	}
	return "";
}

std::string Sys::getEnv( const std::string& name ) {
#if EE_PLATFORM == EE_PLATFORM_WIN && defined( EE_COMPILER_MSVC )
	wchar_t* envbuf;
	size_t envsize;
	_wdupenv_s( &envbuf, &envsize, String( name ).toWideString().c_str() );
	std::string env;
	if ( NULL != envbuf )
		env = String::fromWide( envbuf ).toUtf8();
	free( envbuf );
	return env;
#else
	char* env = ::getenv( name.c_str() );
	return NULL == env ? std::string() : std::string( env );
#endif
}

std::vector<std::string> Sys::getEnvSplitted( const std::string& name ) {
	return String::split( getEnv( name.c_str() ), PATH_SEP_CHAR );
}

#if EE_PLATFORM == EE_PLATFORM_WIN
static ULONG_PTR GetParentProcessId() {
	ULONG_PTR pbi[6];
	ULONG ulSize = 0;
	LONG( WINAPI * NtQueryInformationProcess )
	( HANDLE ProcessHandle, ULONG ProcessInformationClass, PVOID ProcessInformation,
	  ULONG ProcessInformationLength, PULONG ReturnLength );
	*(FARPROC*)&NtQueryInformationProcess =
		GetProcAddress( LoadLibraryA( "NTDLL.DLL" ), "NtQueryInformationProcess" );
	if ( NtQueryInformationProcess ) {
		if ( NtQueryInformationProcess( GetCurrentProcess(), 0, &pbi, sizeof( pbi ), &ulSize ) >=
				 0 &&
			 ulSize == sizeof( pbi ) )
			return pbi[5];
	}
	return (ULONG_PTR)-1;
}

static bool isWineRunning() {
	HMODULE hntdll = GetModuleHandle( TEXT( "ntdll.dll" ) );
	if ( !hntdll )
		return false;
	void* pwine_get_version = (void*)GetProcAddress( hntdll, "wine_get_version" );
	return pwine_get_version != NULL;
}
#endif

bool Sys::windowAttachConsole() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	// WINE doesn't need to attach any console
	if ( isWineRunning() )
		return true;
	ULONG_PTR ppid = GetParentProcessId();
	if ( ppid == (ULONG_PTR)-1 ) {
		return false;
	} else {
		AttachConsole( ppid );
	}

	freopen( "CONIN$", "r", stdin );
	freopen( "CONOUT$", "w", stdout );
	freopen( "CONOUT$", "w", stderr );

	std::cout.clear();
	std::cerr.clear();
	std::cin.clear();

	std::wcout.clear();
	std::wcerr.clear();
	std::wcin.clear();
#endif
	return true;
}

#if EE_PLATFORM == EE_PLATFORM_WIN
static int windowsSystem( const std::string& programPath, const std::string& workingDirectory ) {
	STARTUPINFOW si;
	PROCESS_INFORMATION pi;
	ZeroMemory( &si, sizeof( si ) );
	si.cb = sizeof( si );
	ZeroMemory( &pi, sizeof( pi ) );

	std::wstring workingDir = String( workingDirectory ).toWideString();

	if ( CreateProcessW( NULL, (LPWSTR)String( programPath ).toWideString().c_str(), NULL, NULL,
						 FALSE, 0, NULL, workingDir.empty() ? NULL : workingDir.c_str(), &si,
						 &pi ) ) {
		int pid = static_cast<int>( pi.dwProcessId );
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
		return pid;
	}

	return 0;
}
#endif

int Sys::execute( const std::string& cmd, const std::string& workingDir ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return windowsSystem( cmd, workingDir );
#elif EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
	pid_t pid = fork();
	if ( pid == 0 ) {
		std::vector<std::string> cmdArr = String::split( cmd, " ", "", "\"", true );
		std::vector<const char*> strings;
		for ( size_t i = 0; i < cmdArr.size(); ++i )
			strings.push_back( cmdArr[i].c_str() );
		strings.push_back( NULL );

		if ( !workingDir.empty() )
			FileSystem::changeWorkingDirectory( workingDir );

		execvp( strings[0], (char* const*)strings.data() );
		exit( 0 );
	}
	return pid;
#endif
}

bool Sys::isMobile() {
#if EE_PLATFORM == EE_PLATFORM_ANDROID || EE_PLATFORM == EE_PLATFORM_IOS
	return true;
#else
	return false;
#endif
}

std::unordered_map<std::string, std::string> Sys::getEnvironmentVariables() {
	return _getEnvironmentVariables();
}

std::string Sys::getProcessFilePath() {
#if EE_PLATFORM != EE_PLATFORM_WIN
	char exename[PATH_MAX];
#endif

#if EE_PLATFORM == EE_PLATFORM_WIN
	std::wstring exename( _MAX_DIR, 0 );
	DWORD size = GetModuleFileNameW( 0, &exename[0], _MAX_PATH );
	if ( size > 0 && size < _MAX_PATH )
		exename.resize( size ); // Resize to actual size without extra null characters
	return String( exename ).toUtf8();
#elif EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_ANDROID
	char path[] = "/proc/self/exe";
	ssize_t len = readlink( path, exename, PATH_MAX - 1 );
	if ( len > 0 )
		exename[len] = '\0';
#elif EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_IOS
	/* use realpath to resolve a symlink if the process was launched from one.
	** This happens when Homebrew installs a cack and creates a symlink in
	** /usr/loca/bin for launching the executable from the command line. */
	unsigned size = PATH_MAX;
	char exepath[size];
	_NSGetExecutablePath( exepath, &size );
	realpath( exepath, exename );
#elif EE_PLATFORM == EE_PLATFORM_BSD
	size_t len = PATH_MAX;
	const int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
	sysctl( mib, 4, exename, &len, NULL, 0 );
#elif EE_PLATFORM == EE_PLATFORM_HAIKU
	image_info info;
	int32 cookie = 0;

	while ( B_OK == get_next_image_info( 0, &cookie, &info ) ) {
		if ( info.type == B_APP_IMAGE )
			break;
	}

	return FileSystem::fileNameFromPath( std::string( info.name ) );
#else
	*exename = 0;
#endif

#if EE_PLATFORM != EE_PLATFORM_WIN
	return std::string( exename );
#endif
}

Int64 Sys::getProcessCreationTime( Uint64 pid ) {
	Int64 creationTime = -1;

#if EE_PLATFORM == EE_PLATFORM_WIN
	int rpid = static_cast<int>( pid );
	HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION, FALSE, rpid );
	if ( hProcess == NULL ) {
		return -1;
	}

	FILETIME creationFileTime, exitFileTime, kernelFileTime, userFileTime;
	if ( GetProcessTimes( hProcess, &creationFileTime, &exitFileTime, &kernelFileTime,
						  &userFileTime ) ) {
		ULARGE_INTEGER ull;
		ull.LowPart = creationFileTime.dwLowDateTime;
		ull.HighPart = creationFileTime.dwHighDateTime;

		// Convert from Windows file time to Unix timestamp
		creationTime =
			static_cast<time_t>( ( ull.QuadPart - 116444736000000000ULL ) / 10000000ULL );
	} else {
		creationTime = -1;
	}

	CloseHandle( hProcess );

#elif EE_PLATFORM == EE_PLATFORM_LINUX
	std::ifstream statFile( "/proc/" + std::to_string( pid ) + "/stat" );
	if ( !statFile.is_open() ) {
		return -1;
	}

	std::string token;
	long startTimeTicks = 0;
	int field = 1;
	while ( statFile >> token ) {
		if ( field == 22 ) { // The 22nd field is the start time in clock ticks
			startTimeTicks = std::stol( token );
			break;
		}
		field++;
	}

	struct sysinfo sysInfo;
	sysinfo( &sysInfo );
	long uptime = sysInfo.uptime;

	long clockTicksPerSecond = sysconf( _SC_CLK_TCK );
	creationTime = time( NULL ) - uptime + ( startTimeTicks / clockTicksPerSecond );

	statFile.close();

#elif EE_PLATFORM == EE_PLATFORM_MACOS
	struct proc_bsdinfo procInfo;
	int rpid = static_cast<int>( pid );
	int status = proc_pidinfo( rpid, PROC_PIDTBSDINFO, 0, &procInfo, sizeof( procInfo ) );
	if ( status <= 0 ) {
		return -1;
	}

	creationTime = procInfo.pbi_start_tvsec;

#elif EE_PLATFORM == EE_PLATFORM_BSD
	struct kinfo_proc proc;
	int rpid = static_cast<int>( pid );
	size_t procLen = sizeof( proc );
	int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, rpid };

	if ( sysctl( mib, 4, &proc, &procLen, NULL, 0 ) < 0 ) {
		return -1;
	}

	creationTime = proc.ki_start.tv_sec;

#elif EE_PLATFORM == EE_PLATFORM_HAIKU
	int rpid = static_cast<int>( pid );
	int32 cookie = 0;
	team_info teamInfo;
	while ( get_next_team_info( &cookie, &teamInfo ) == B_OK ) {
		if ( teamInfo.team != rpid )
			continue;
		return teamInfo.start_time;
	}
#endif

	return creationTime;
}

std::vector<Uint64> Sys::pidof( const std::string& processName ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	std::vector<Uint64> pids;
	std::vector<std::string> extensions = getEnvSplitted( "PATHEXT" );

	HMODULE hPsapi = LoadLibrary( TEXT( "psapi.dll" ) );
	if ( !hPsapi )
		return pids;

	EnumProcesses_t EnumProcesses = (EnumProcesses_t)GetProcAddress( hPsapi, "EnumProcesses" );
	EnumProcessModules_t EnumProcessModules =
		(EnumProcessModules_t)GetProcAddress( hPsapi, "EnumProcessModules" );
	GetModuleBaseName_t GetModuleBaseName =
		(GetModuleBaseName_t)GetProcAddress( hPsapi, "GetModuleBaseNameA" );

	if ( !EnumProcesses || !EnumProcessModules || !GetModuleBaseName ) {
		FreeLibrary( hPsapi );
		eePRINTL( "EnumProcesses or EnumProcessModules or GetModuleBaseName failed" );
		return pids;
	}

	DWORD processIds[1024], cbNeeded;
	if ( !EnumProcesses( processIds, sizeof( processIds ), &cbNeeded ) ) {
		FreeLibrary( hPsapi );
		eePRINTL( "EnumProcesses failed" );
		return pids;
	}

	DWORD numProcesses = cbNeeded / sizeof( DWORD );

	for ( DWORD i = 0; i < numProcesses; ++i ) {
		if ( processIds[i] == 0 )
			continue;

		HANDLE hProcess =
			OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processIds[i] );
		if ( hProcess ) {
			HMODULE hMod;
			DWORD cbNeededMod;
			if ( EnumProcessModules( hProcess, &hMod, sizeof( hMod ), &cbNeededMod ) ) {
				char szProcessName[MAX_PATH];
				if ( GetModuleBaseName( hProcess, hMod, szProcessName,
										sizeof( szProcessName ) / sizeof( char ) ) ) {
					std::string actualName( szProcessName, std::strlen( szProcessName ) );

					// Check if the process name matches the input name with or without extensions
					if ( actualName == processName ) {
						pids.push_back( processIds[i] );
					} else {
						for ( const auto& ext : extensions ) {
							std::string extName = processName + ext;
							if ( actualName == extName ) {
								pids.push_back( processIds[i] );
								break;
							}
						}
					}
				}
			}
			CloseHandle( hProcess );
		}
	}

	for ( auto pid : pids )
		eePRINTL( "Found pid %d", pid );

	FreeLibrary( hPsapi );
	return pids;
#elif EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_ANDROID

	std::vector<Uint64> pids;
	DIR* dir = opendir( "/proc" );
	if ( !dir ) {
		return pids;
	}

	struct dirent* entry;
	while ( ( entry = readdir( dir ) ) != NULL ) {
		if ( entry->d_type == DT_DIR && isdigit( entry->d_name[0] ) ) {
			std::string pidDir = "/proc/" + std::string( entry->d_name );
			std::string cmdPath = pidDir + "/comm";
			FILE* cmdFile = fopen( cmdPath.c_str(), "r" );
			if ( cmdFile ) {
				char cmdline[256];
				if ( fgets( cmdline, sizeof( cmdline ), cmdFile ) != NULL ) {
					cmdline[strcspn( cmdline, "\n" )] = 0; // Remove newline
					if ( processName == cmdline ) {
						pids.push_back( atoi( entry->d_name ) );
					}
				}
				fclose( cmdFile );
			}
		}
	}

	closedir( dir );
	return pids;
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	std::vector<Uint64> pids;

	int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
	size_t len;

	if ( sysctl( mib, 4, NULL, &len, NULL, 0 ) == -1 ) {
		return pids;
	}

	struct kinfo_proc* procs = (struct kinfo_proc*)malloc( len );
	if ( !procs ) {
		return pids;
	}

	if ( sysctl( mib, 4, procs, &len, NULL, 0 ) == -1 ) {
		free( procs );
		return pids;
	}

	int proc_count = len / sizeof( struct kinfo_proc );

	for ( int i = 0; i < proc_count; i++ ) {
		std::string name( procs[i].kp_proc.p_comm );
		if ( name == processName ) {
			pids.push_back( procs[i].kp_proc.p_pid );
		}
	}

	free( procs );
	return pids;
#elif EE_PLATFORM == EE_PLATFORM_BSD
	std::vector<Uint64> pids;

	int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PROC, 0 };
	size_t len;

	if ( sysctl( mib, 4, NULL, &len, NULL, 0 ) == -1 ) {
		return pids;
	}

	struct kinfo_proc* procs = (struct kinfo_proc*)malloc( len );
	if ( !procs ) {
		return pids;
	}

	if ( sysctl( mib, 4, procs, &len, NULL, 0 ) == -1 ) {
		free( procs );
		return pids;
	}

	int proc_count = len / sizeof( struct kinfo_proc );

	for ( int i = 0; i < proc_count; i++ ) {
		std::string name( procs[i].ki_comm );
		if ( name == processName ) {
			pids.push_back( procs[i].ki_pid );
		}
	}

	free( procs );
	return pids;
#elif EE_PLATFORM == EE_PLATFORM_HAIKU
	std::vector<Uint64> pids;
	int32 cookie = 0;
	team_info teamInfo;
	while ( get_next_team_info( &cookie, &teamInfo ) == B_OK ) {
		if ( std::string_view{ teamInfo.name } == std::string_view{ processName } )
			pids.push_back( teamInfo.team );
	}
	return pids;
#else
#warning Platform not supported
	return {};
#endif
}

std::vector<std::pair<Uint64, std::string>> Sys::listProcesses() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	std::vector<std::pair<Uint64, std::string>> pids;
	std::vector<std::string> extensions = getEnvSplitted( "PATHEXT" );

	HMODULE hPsapi = LoadLibrary( TEXT( "psapi.dll" ) );
	if ( !hPsapi )
		return pids;

	EnumProcesses_t EnumProcesses = (EnumProcesses_t)GetProcAddress( hPsapi, "EnumProcesses" );
	EnumProcessModules_t EnumProcessModules =
		(EnumProcessModules_t)GetProcAddress( hPsapi, "EnumProcessModules" );
	GetModuleBaseName_t GetModuleBaseName =
		(GetModuleBaseName_t)GetProcAddress( hPsapi, "GetModuleBaseNameA" );

	if ( !EnumProcesses || !EnumProcessModules || !GetModuleBaseName ) {
		FreeLibrary( hPsapi );
		eePRINTL( "EnumProcesses or EnumProcessModules or GetModuleBaseName failed" );
		return pids;
	}

	DWORD processIds[1024], cbNeeded;
	if ( !EnumProcesses( processIds, sizeof( processIds ), &cbNeeded ) ) {
		FreeLibrary( hPsapi );
		eePRINTL( "EnumProcesses failed" );
		return pids;
	}

	DWORD numProcesses = cbNeeded / sizeof( DWORD );
	pids.reserve( numProcesses );

	for ( DWORD i = 0; i < numProcesses; ++i ) {
		if ( processIds[i] == 0 )
			continue;

		HANDLE hProcess =
			OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processIds[i] );
		if ( hProcess ) {
			HMODULE hMod;
			DWORD cbNeededMod;
			if ( EnumProcessModules( hProcess, &hMod, sizeof( hMod ), &cbNeededMod ) ) {
				char szProcessName[MAX_PATH];
				if ( GetModuleBaseName( hProcess, hMod, szProcessName,
										sizeof( szProcessName ) / sizeof( char ) ) ) {
					std::string actualName( szProcessName, std::strlen( szProcessName ) );
					pids.emplace_back( processIds[i], std::move( actualName ) );
				}
			}
			CloseHandle( hProcess );
		}
	}

	FreeLibrary( hPsapi );
	return pids;
#elif EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_ANDROID
	std::vector<std::pair<Uint64, std::string>> pids;
	DIR* dir = opendir( "/proc" );
	if ( !dir ) {
		return pids;
	}

	struct dirent* entry;
	while ( ( entry = readdir( dir ) ) != NULL ) {
		if ( entry->d_type == DT_DIR && isdigit( entry->d_name[0] ) ) {
			std::string pidDir = "/proc/" + std::string( entry->d_name );
			std::string cmdPath = pidDir + "/comm";
			FILE* cmdFile = fopen( cmdPath.c_str(), "r" );
			if ( cmdFile ) {
				char cmdline[256];
				if ( fgets( cmdline, sizeof( cmdline ), cmdFile ) != NULL ) {
					cmdline[strcspn( cmdline, "\n" )] = 0; // Remove newline
					pids.emplace_back( atoi( entry->d_name ), std::string{ cmdline } );
				}
				fclose( cmdFile );
			}
		}
	}

	closedir( dir );
	return pids;
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	std::vector<std::pair<Uint64, std::string>> pids;

	int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0 };
	size_t len;

	if ( sysctl( mib, 4, NULL, &len, NULL, 0 ) == -1 ) {
		return pids;
	}

	struct kinfo_proc* procs = (struct kinfo_proc*)malloc( len );
	if ( !procs ) {
		return pids;
	}

	if ( sysctl( mib, 4, procs, &len, NULL, 0 ) == -1 ) {
		free( procs );
		return pids;
	}

	int proc_count = len / sizeof( struct kinfo_proc );
	pids.reserve( proc_count );

	for ( int i = 0; i < proc_count; i++ ) {
		std::string name( procs[i].kp_proc.p_comm );
		pids.emplace_back( procs[i].kp_proc.p_pid, name );
	}

	free( procs );
	return pids;
#elif EE_PLATFORM == EE_PLATFORM_BSD
	std::vector<std::pair<Uint64, std::string>> pids;

	int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PROC, 0 };
	size_t len;

	if ( sysctl( mib, 4, NULL, &len, NULL, 0 ) == -1 ) {
		return pids;
	}

	struct kinfo_proc* procs = (struct kinfo_proc*)malloc( len );
	if ( !procs ) {
		return pids;
	}

	if ( sysctl( mib, 4, procs, &len, NULL, 0 ) == -1 ) {
		free( procs );
		return pids;
	}

	int proc_count = len / sizeof( struct kinfo_proc );
	pids.reserve( proc_count );

	for ( int i = 0; i < proc_count; i++ ) {
		std::string name( procs[i].ki_comm );
		pids.emplace_back( procs[i].ki_pid, name );
	}

	free( procs );
	return pids;
#elif EE_PLATFORM == EE_PLATFORM_HAIKU
	std::vector<std::pair<Uint64, std::string>> pids;
	int32 cookie = 0;
	team_info teamInfo;
	while ( get_next_team_info( &cookie, &teamInfo ) == B_OK ) {
		pids.push_back( teamInfo.team, std::string{ teamInfo.name } );
	}
	return pids;
#else
#warning Platform not supported
	return {};
#endif
}

#pragma pack( push, 1 )

// Basic structure of the Shell Link Header (size = 76 bytes)
struct ShellLinkHeader {
	uint32_t headerSize;
	uint8_t guid[16];
	uint32_t linkFlags;
	uint32_t fileAttributes;
	uint64_t creationTime;
	uint64_t accessTime;
	uint64_t writeTime;
	uint32_t fileSize;
	uint32_t iconIndex;
	uint32_t showCommand;
	uint16_t hotKey;
	uint16_t reserved1;
	uint32_t reserved2;
	uint32_t reserved3;
};

#pragma pack( pop )

std::string Sys::getShortcutTarget( const std::string& lnkFilePath ) {
	if ( FileSystem::fileExtension( lnkFilePath ) != "lnk" )
		return "";

	std::vector<Uint8> data;
	FileSystem::fileGet( lnkFilePath, data );

	ShellLinkHeader* header = reinterpret_cast<ShellLinkHeader*>( data.data() );

	if ( header->headerSize != 76 )
		return "";

	size_t offset = sizeof( ShellLinkHeader );

	if ( header->linkFlags & 0x01 ) {
		uint16_t idListSize = *reinterpret_cast<uint16_t*>( &data[offset] );
		offset += 2 + idListSize; // Skip the IDList section
	}

	if ( header->linkFlags & 0x02 ) {
		uint32_t linkInfoSize = *reinterpret_cast<uint32_t*>( &data[offset] );

		if ( linkInfoSize > 0 ) {
			uint32_t localBasePathOffset = *reinterpret_cast<uint32_t*>( &data[offset + 16] );

			if ( localBasePathOffset != 0 && ( offset + localBasePathOffset ) < data.size() ) {
				std::string targetPath(
					reinterpret_cast<char*>( &data[offset + localBasePathOffset] ) );
				return targetPath;
			}
		}
	}

	return "";
}

std::string Sys::getUserDirectory() {
#ifdef _WIN32
	// On Windows, use USERPROFILE or HOMEDRIVE + HOMEPATH
	const char* userProfile = std::getenv( "USERPROFILE" );
	if ( userProfile ) {
		return std::string( userProfile );
	} else {
		// Fallback to HOMEDRIVE + HOMEPATH
		const char* homeDrive = std::getenv( "HOMEDRIVE" );
		const char* homePath = std::getenv( "HOMEPATH" );
		if ( homeDrive && homePath )
			return std::string( homeDrive ) + homePath;
	}
	return "";
#else
	// On Unix-based systems, use HOME
	return std::string{ std::getenv( "HOME" ) };
#endif
}

}} // namespace EE::System
