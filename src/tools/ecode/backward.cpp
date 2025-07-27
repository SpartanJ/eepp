#include <eepp/config.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>

using namespace EE;
using namespace EE::System;

#if EE_PLATFORM != EE_PLATFORM_ANDROID && EE_PLATFORM != EE_PLATFORM_IOS

#if defined( ECODE_HAS_DW )
#define BACKWARD_HAS_DW 1
#endif

#include <backward-cpp/backward.hpp>

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

	static void display_crash_message( const std::string& crashFilePath ) {
		std::string crashMsg(
			String::format( "ecode has encountered an unrecoverable error and crashed! :'(\n"
							"A crash log has been saved at:\n%s\n"
							"Please feel free to use this file to report a bug at the ecode Github "
							"page so it can be fixed as soon as possible.",
							crashFilePath ) );

		std::wstring wCrashMsg( crashMsg.begin(), crashMsg.end() );
		std::wstring wTitle = L"ecode crashed!";

		MessageBoxW( nullptr, wCrashMsg.c_str(), wTitle.c_str(),
					 MB_OK | MB_ICONERROR | MB_TOPMOST );
	}

	static void handle_stacktrace( int skip_frames = 0 ) {
		std::string crashesPath( Sys::getProcessPath() );
		FileSystem::dirAddSlashAtEnd( crashesPath );
		crashesPath += "crashes";
		FileSystem::dirAddSlashAtEnd( crashesPath );
		std::string dateTimeStr( Sys::getDateTimeStr() );
		String::replaceAll( dateTimeStr, " ", "_" );
		String::replaceAll( dateTimeStr, ":", "-" );
		std::string crashFilePath(
			String::format( "%sstacktrace_%s.log", crashesPath, dateTimeStr ) );

		FileSystem::makeDir( crashesPath );

		std::ofstream outFile( crashFilePath );
		if ( !outFile.is_open() ) {
			print_to_console( "Error: Failed to open " + crashFilePath +
							  " for writing stack trace\n" );
			print_stacktrace_to_console( skip_frames, nullptr );
			return;
		}

		Printer printer;
		printer.address = true;
		printer.object = true;

		StackTrace st;
		st.set_machine_type( printer.resolver().machine_type() );
		st.set_thread_handle( thread_handle() );
		st.load_here( 64 + skip_frames, ctx() ); // Increased frame limit
		st.skip_n_firsts( skip_frames );

		printer.print( st, outFile );
		print_stacktrace_to_console( skip_frames, &st );

		outFile.close();

		display_crash_message( crashFilePath );
	}

	static void print_stacktrace_to_console( int skip_frames, const StackTrace* st = nullptr ) {
		if ( AttachConsole( ATTACH_PARENT_PROCESS ) ) {
			FILE* console = nullptr;
			freopen_s( &console, "CONOUT$", "w", stderr );
			if ( console ) {
				Printer printer;
				printer.address = true;
				printer.object = true;

				if ( st ) {
					printer.print( *st, std::cerr );
				} else {
					StackTrace new_st;
					new_st.set_machine_type( printer.resolver().machine_type() );
					new_st.set_thread_handle( thread_handle() );
					new_st.load_here( 64 + skip_frames, ctx() );
					new_st.skip_n_firsts( skip_frames );
					printer.print( new_st, std::cerr );
				}
				fflush( stderr );
			}
			FreeConsole();
		}
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
