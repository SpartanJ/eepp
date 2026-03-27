#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/window/backend/backendhelper.hpp>

using namespace EE::System;

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <cstdint>
#include <initguid.h>
#include <objbase.h>
#include <shellapi.h>
#include <tlhelp32.h>
#include <windows.h>

namespace EE { namespace Window { namespace Backend {

bool BackendHelper::isProcessRunning( const char* processName, bool killProcess ) {
	bool exists = false;
	PROCESSENTRY32 entry = {};
	entry.dwSize = sizeof( PROCESSENTRY32 );
	HANDLE snapshot = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
	if ( Process32First( snapshot, &entry ) ) {
		while ( Process32Next( snapshot, &entry ) ) {
#ifdef UNICODE
			if ( EE::String( entry.szExeFile ).toUtf8() == std::string( processName ) ) {
#else
			if ( !stricmp( entry.szExeFile, processName ) ) {
#endif
				exists = true;
				if ( killProcess ) {
					HANDLE aProc = OpenProcess( PROCESS_TERMINATE, 0, entry.th32ProcessID );
					if ( aProc ) {
						TerminateProcess( aProc, 9 );
						CloseHandle( aProc );
					}
				}
				break;
			}
		}
	}
	CloseHandle( snapshot );
	return exists;
}

#ifndef ERROR_ELEVATION_REQUIRED
#define ERROR_ELEVATION_REQUIRED ( 740 )
#endif

bool BackendHelper::processLaunch( std::string command, HWND windowHwnd ) {
#ifdef UNICODE
	wchar_t expandedCmd[1024] = {};
#else
	char expandedCmd[1024] = {};
#endif
	static PROCESS_INFORMATION pi = {};
	static STARTUPINFO si = {};
	si.cb = sizeof( si );
#if UNICODE
	ExpandEnvironmentStrings( EE::String::fromUtf8( command ).toWideString().c_str(), expandedCmd,
							  1024 );
#else
	ExpandEnvironmentStrings( command.c_str(), expandedCmd, 1024 );
#endif
	if ( CreateProcess( NULL, expandedCmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi ) ) {
		WaitForSingleObject( pi.hProcess, 10000 );
		CloseHandle( pi.hProcess );
		CloseHandle( pi.hThread );
		return true;
	} else {
		DWORD error = GetLastError();
		if ( error == ERROR_ELEVATION_REQUIRED && 0 != windowHwnd ) {
#ifdef UNICODE
			std::intptr_t res = reinterpret_cast<std::intptr_t>(
				ShellExecute( windowHwnd, L"open", expandedCmd, L"", NULL, SW_SHOWDEFAULT ) );
#else
			std::intptr_t res = reinterpret_cast<std::intptr_t>(
				ShellExecute( windowHwnd, "open", expandedCmd, "", NULL, SW_SHOWDEFAULT ) );
#endif
			if ( res <= 32 ) {
				return false;
			}
			return true;
		}
	}
	return false;
}

DEFINE_GUID( CLSID_UIHostNoLaunch, 0x4CE576FA, 0x83DC, 0x4f88, 0x95, 0x1C, 0x9D, 0x07, 0x82, 0xB4,
			 0xE3, 0x76 );

DEFINE_GUID( IID_ITipInvocation, 0x37c994e7, 0x432b, 0x4834, 0xa2, 0xf7, 0xdc, 0xe1, 0xf1, 0x3b,
			 0x83, 0x4b );

struct ITipInvocation : IUnknown {
	virtual HRESULT STDMETHODCALLTYPE Toggle( HWND wnd ) = 0;
};

static bool WIN_OSK_VISIBLE = false;

int BackendHelper::showOSK( HWND windowHwnd ) {
	if ( !isProcessRunning( "TabTip.exe" ) ) {
		isProcessRunning( "WindowsInternal.ComposableShell.Experiences.TextInput.InputApp.EXE",
						  true );

		std::string programFiles( Sys::getOSArchitecture() == "x64" ? "%ProgramW6432%"
																	: "%ProgramFiles(x86)%" );
		processLaunch( programFiles + "\\Common Files\\microsoft shared\\ink\\TabTip.exe",
					   windowHwnd );
	}

	CoInitialize( 0 );

	ITipInvocation* tip;
	CoCreateInstance( CLSID_UIHostNoLaunch, 0, CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
					  IID_ITipInvocation, (void**)&tip );
	if ( tip != NULL ) {
		tip->Toggle( GetDesktopWindow() );
		tip->Release();
		WIN_OSK_VISIBLE = true;
	}

	return 0;
}

int BackendHelper::hideOSK() {
	WIN_OSK_VISIBLE = false;
	return PostMessage( GetDesktopWindow(), WM_SYSCOMMAND, (int)SC_CLOSE, 0 );
}

bool BackendHelper::isOSKActive() {
	return WIN_OSK_VISIBLE;
}

bool BackendHelper::isDarkModeEnabled() {
	HKEY hKey;
	DWORD value = 1;
	DWORD valueSize = sizeof( value );

	if ( RegOpenKeyExA( HKEY_CURRENT_USER,
						"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0,
						KEY_READ, &hKey ) == ERROR_SUCCESS ) {
		RegQueryValueExA( hKey, "AppsUseLightTheme", nullptr, nullptr,
						  reinterpret_cast<LPBYTE>( &value ), &valueSize );
		RegCloseKey( hKey );
	}

	return value == 0;
}

typedef HRESULT( WINAPI* DwmSetWindowAttributeFunc )( HWND, DWORD, LPCVOID, DWORD );

constexpr DWORD DWMWA_USE_IMMERSIVE_DARK_MODE = 20;

void BackendHelper::setUserTheme( HWND hwnd ) {
	HMODULE hDwmapi = LoadLibraryA( "dwmapi.dll" );
	if ( !hDwmapi ) {
		return;
	}

	auto DwmSetWindowAttribute = reinterpret_cast<DwmSetWindowAttributeFunc>(
		GetProcAddress( hDwmapi, "DwmSetWindowAttribute" ) );
	if ( !DwmSetWindowAttribute ) {
		FreeLibrary( hDwmapi );
		return;
	}

	BOOL darkMode = isDarkModeEnabled() ? TRUE : FALSE;
	DwmSetWindowAttribute( hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof( darkMode ) );

	FreeLibrary( hDwmapi );
}

}}} // namespace EE::Window::Backend

#elif defined( EE_X11_PLATFORM )
#include <signal.h>
#include <unistd.h>

static pid_t ONBOARD_PID = 0;

namespace EE { namespace Window { namespace Backend {

void BackendHelper::showOSK() {
	if ( ONBOARD_PID == 0 ) {
		if ( FileSystem::fileExists( "/usr/bin/onboard" ) ) {
			pid_t pid = fork();

			if ( pid == 0 ) {
				execl( "/usr/bin/onboard", "onboard", (char*)NULL );
			} else if ( pid != -1 ) {
				ONBOARD_PID = pid;
			}
		} else {
			EE::System::Log::error(
				"\"onboard\" must be installed to be able to use the On Screen Keyboard" );
		}
	}
}

void BackendHelper::hideOSK() {
	if ( ONBOARD_PID != 0 ) {
		kill( ONBOARD_PID, SIGTERM );
		ONBOARD_PID = 0;
	}
}

bool BackendHelper::isOSKActive() {
	return ONBOARD_PID != 0;
}

}}} // namespace EE::Window::Backend

#else

namespace EE { namespace Window { namespace Backend {

void BackendHelper::hideOSK() {}

bool BackendHelper::isOSKActive() {
	return false;
}

}}} // namespace EE::Window::Backend

#endif
