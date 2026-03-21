#include <eepp/config.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <sstream>

using namespace EE;
using namespace EE::System;

#if EE_PLATFORM != EE_PLATFORM_ANDROID && EE_PLATFORM != EE_PLATFORM_IOS

#if defined( ECODE_HAS_DW )
#define BACKWARD_HAS_DW 1
#endif

#include <backward-cpp/backward.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN
#include <windows.h>
#include <shellapi.h> // Added for ShellExecuteW
#endif

namespace backward {

#if EE_PLATFORM == EE_PLATFORM_WIN

class WindowsSignalHandling {
  public:
	WindowsSignalHandling( const std::vector<int>& = std::vector<int>() ) :
		reporter_thread_( []() {
			/* We handle crashes in a utility thread:
			  backward structures and some Windows functions called here
			  need stack space, which we do not have when we encounter a
			  stack overflow.
			  To support reporting stack traces during a stack overflow,
			  we create a utility thread at startup, which waits until a
			  crash happens or the program exits normally. */

			{
				std::unique_lock<std::mutex> lk( mtx() );
				cv().wait( lk, [] { return crashed() != crash_status::running; } );
			}
			if ( crashed() == crash_status::crashed ) {
				handle_stacktrace( skip_recs() );
			}
			{
				std::unique_lock<std::mutex> lk( mtx() );
				crashed() = crash_status::ending;
			}
			cv().notify_one();
		} ) {

		// Disable Windows Error Reporting dialogs
		// SetErrorMode( SEM_NOGPFAULTERRORBOX | SEM_FAILCRITICALERRORS );

		SetUnhandledExceptionFilter( crash_handler );

		signal( SIGABRT, signal_handler );

		std::set_terminate( &terminator );
#ifndef BACKWARD_ATLEAST_CXX17
		std::set_unexpected( &terminator );
#endif
		_set_purecall_handler( &terminator );
		_set_invalid_parameter_handler( &invalid_parameter_handler );
	}

	bool loaded() const { return true; }

	~WindowsSignalHandling() {
		{
			std::unique_lock<std::mutex> lk( mtx() );
			crashed() = crash_status::normal_exit;
		}

		cv().notify_one();

		reporter_thread_.join();
	}

  private:
	static CONTEXT* ctx() {
		static CONTEXT data;
		return &data;
	}

	enum class crash_status { running, crashed, normal_exit, ending };

	static crash_status& crashed() {
		static crash_status data;
		return data;
	}

	static std::mutex& mtx() {
		static std::mutex data;
		return data;
	}

	static std::condition_variable& cv() {
		static std::condition_variable data;
		return data;
	}

	static HANDLE& thread_handle() {
		static HANDLE handle;
		return handle;
	}

	std::thread reporter_thread_;

	static const constexpr int signal_skip_recs =
#ifdef __clang__
		4
#else
		3
#endif
		;

	static int& skip_recs() {
		static int data;
		return data;
	}

	static inline void terminator() {
		crash_handler( signal_skip_recs );
		abort();
	}

	static inline void signal_handler( int ) {
		crash_handler( signal_skip_recs );
		abort();
	}

	static inline void __cdecl invalid_parameter_handler( const wchar_t*, const wchar_t*,
														  const wchar_t*, unsigned int,
														  uintptr_t ) {
		crash_handler( signal_skip_recs );
		abort();
	}

	NOINLINE static LONG WINAPI crash_handler( EXCEPTION_POINTERS* info ) {
		crash_handler( 0, info->ContextRecord );
		return EXCEPTION_CONTINUE_SEARCH;
	}

	NOINLINE static void crash_handler( int skip, CONTEXT* ct = nullptr ) {
		if ( ct == nullptr ) {
			RtlCaptureContext( ctx() );
		} else {
			memcpy( ctx(), ct, sizeof( CONTEXT ) );
		}
		DuplicateHandle( GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(),
						 &thread_handle(), 0, FALSE, DUPLICATE_SAME_ACCESS );

		skip_recs() = skip;

		{
			std::unique_lock<std::mutex> lk( mtx() );
			crashed() = crash_status::crashed;
		}

		cv().notify_one();

		{
			std::unique_lock<std::mutex> lk( mtx() );
			cv().wait( lk, [] { return crashed() != crash_status::crashed; } );
		}
	}
	// Helper to safely convert UTF-8 strings to std::wstring for the Windows API
	static std::wstring utf8_to_wstring( const std::string& str ) {
		if ( str.empty() )
			return std::wstring();
		int size_needed = MultiByteToWideChar( CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0 );
		std::wstring wstrTo( size_needed, 0 );
		MultiByteToWideChar( CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed );
		return wstrTo;
	}

	// Custom Window Procedure to handle button clicks on our crash dialog
	static LRESULT CALLBACK CrashDialogProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
		switch ( msg ) {
			case WM_COMMAND: {
				if ( LOWORD( wParam ) == 1 ) { // Report Button
					MessageBoxW(
						hwnd,
						L"Please quickly verify if this stack trace has already been reported to "
						L"avoid duplicate issues.\n\nClick OK to open the issue tracker.",
						L"Notice", MB_OK | MB_ICONINFORMATION );

					// Replace this URL with the actual ecode issue tracker URL
					ShellExecuteW( NULL, L"open", L"https://github.com/SpartanJ/ecode/issues", NULL,
								   NULL, SW_SHOWNORMAL );
				} else if ( LOWORD( wParam ) == 2 ) { // Close Button
					PostQuitMessage( 0 );
				}
				break;
			}
			case WM_DESTROY:
				PostQuitMessage( 0 );
				break;
			default:
				return DefWindowProcW( hwnd, msg, wParam, lParam );
		}
		return 0;
	}

	static void display_crash_message( const std::string& crashFilePath,
									   const std::string& stackTrace ) {
		HINSTANCE hInstance = GetModuleHandleW( NULL );

		// 1. Register a custom Window Class
		WNDCLASSEXW wc = { 0 };
		wc.cbSize = sizeof( WNDCLASSEXW );
		wc.lpfnWndProc = CrashDialogProc;
		wc.hInstance = hInstance;
		wc.hCursor = LoadCursor( NULL, IDC_ARROW );
		wc.hbrBackground = (HBRUSH)( COLOR_BTNFACE + 1 );
		wc.lpszClassName = L"EcodeCrashReporter";
		RegisterClassExW( &wc );

		// 2. Create the main Dialog Window
		HWND hwnd =
			CreateWindowExW( WS_EX_TOPMOST, L"EcodeCrashReporter", L"ecode crashed!",
							 WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_VISIBLE, CW_USEDEFAULT,
							 CW_USEDEFAULT, 640, 480, NULL, NULL, hInstance, NULL );

		HFONT hFont = (HFONT)GetStockObject( DEFAULT_GUI_FONT );

		// 3. Add the Label
		std::string crashMsg = "ecode has encountered an unrecoverable error and crashed! :'(\n"
							   "A crash log has been saved at:\n" +
							   crashFilePath + "\nError crash log:";
		HWND hLabel =
			CreateWindowExW( 0, L"STATIC", utf8_to_wstring( crashMsg ).c_str(),
							 WS_CHILD | WS_VISIBLE, 10, 10, 600, 56, hwnd, NULL, hInstance, NULL );
		SendMessage( hLabel, WM_SETFONT, (WPARAM)hFont, TRUE );

		// 4. Add the Text Area (Edit Control) for the Stack Trace
		HWND hEdit = CreateWindowExW( 0, L"EDIT", utf8_to_wstring( stackTrace ).c_str(),
							WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL | WS_BORDER |
							ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_AUTOHSCROLL,
									  10, 68, 600, 320, hwnd, NULL, hInstance, NULL );
		SendMessage( hEdit, WM_SETFONT, (WPARAM)hFont, TRUE );

		// 5. Add "Close" Button (Now on the left)
		HWND hCloseBtn =
			CreateWindowExW( 0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 10, 395,
							 150, 30, hwnd, (HMENU)2, hInstance, NULL );
		SendMessage( hCloseBtn, WM_SETFONT, (WPARAM)hFont, TRUE );

		// 6. Add "Report" Button (Now on the right)
		HWND hReportBtn = CreateWindowExW( 0, L"BUTTON", L"Report Issue...",
										   WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 460, 395, 150, 30,
										   hwnd, (HMENU)1, hInstance, NULL );
		SendMessage( hReportBtn, WM_SETFONT, (WPARAM)hFont, TRUE );

		// 7. Run the Message Loop for this window
		MSG msg;
		while ( GetMessage( &msg, NULL, 0, 0 ) > 0 ) {
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}
	}

	static void handle_stacktrace( int skip_frames = 0 ) {
		const auto appendCrashesPath = []( std::string& crashesPath ) {
			FileSystem::dirAddSlashAtEnd( crashesPath );
			crashesPath += "crashes";
			FileSystem::dirAddSlashAtEnd( crashesPath );
		};

		std::string crashesPath( Sys::getConfigPath( "ecode" ) );
		appendCrashesPath( crashesPath );

		if ( !FileSystem::fileExists( crashesPath ) && !FileSystem::makeDir( crashesPath, true ) ) {
			crashesPath = Sys::getProcessPath();
			appendCrashesPath( crashesPath );
			FileSystem::makeDir( crashesPath, true );
		}

		std::string dateTimeStr( Sys::getDateTimeStr() );
		String::replaceAll( dateTimeStr, " ", "_" );
		String::replaceAll( dateTimeStr, ":", "-" );
		std::string crashFilePath(
			String::format( "%sstacktrace_%s.log", crashesPath, dateTimeStr ) );

		Printer printer;
		printer.address = true;
		printer.object = true;

		StackTrace st;
		st.set_machine_type( printer.resolver().machine_type() );
		st.set_thread_handle( thread_handle() );
		st.load_here( 64 + skip_frames, ctx() );
		st.skip_n_firsts( skip_frames );

		// Capture stacktrace to a string in memory
		std::ostringstream oss;
		printer.print( st, oss );
		std::string stackTraceStr = oss.str();

		// Print to console (Keeping your existing logic active)
		print_to_console( stackTraceStr );

		// Write string to file
		std::ofstream outFile( crashFilePath );
		if ( !outFile.is_open() ) {
			print_to_console( "Error: Failed to open " + crashFilePath + 
				" for writing stack trace\n" + stackTraceStr );
		} else {
			outFile << stackTraceStr;
			outFile.close();
		}

		// Display the custom Windows dialog
		String::replaceAll( stackTraceStr, "\n", "\r\n" );
		display_crash_message( crashFilePath, stackTraceStr );
	}

	static void print_to_console( const std::string& message ) {
		if ( AttachConsole( ATTACH_PARENT_PROCESS ) ) {
			FILE* console = nullptr;
			freopen_s( &console, "CONOUT$", "w", stderr );
			if ( console ) {
				std::cerr << message;
				fflush( stderr );
			}
			FreeConsole();
		}
	}
};

backward::WindowsSignalHandling sh;

#else

backward::SignalHandling sh;

#endif

} // namespace backward

#endif
