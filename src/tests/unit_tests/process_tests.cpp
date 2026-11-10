#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_LINUX || \
	EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_BSD || \
	EE_PLATFORM == EE_PLATFORM_HAIKU

#include <eepp/system/monotonicclock.hpp>
#include <eepp/system/time.hpp>

#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <eepp/system/process.hpp>
#include <eepp/system/sys.hpp>
#include <optional>
#if EE_PLATFORM != EE_PLATFORM_WIN
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace EE::System;

using namespace vfstest;
using EE::System::Process;

UTEST( Process, foregroundReturnsExitCodeAndRestoresHandlers ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	EXPECT_EQ( Process::runForeground( "cmd.exe", { "/D", "/Q", "/C", "exit /B 23" } ), 23 );
#else
	EXPECT_EQ( Process::runForeground( "/bin/sh", { "-c", "exit 23" } ), 23 );
#endif
	EXPECT_EQ( Process::runForeground( "", {} ), 127 );
}

#if EE_PLATFORM != EE_PLATFORM_WIN
UTEST( Process, signalExitCodesSurviveJoinAndAlivePolling ) {
	for ( bool poll : { false, true } ) {
		Process child;
		ASSERT_TRUE( child.create( "/bin/sh", std::vector<std::string>{ "-c", "kill -TERM $$" } ) );
		if ( poll ) {
			for ( unsigned i = 0; i < 200 && child.isAlive(); ++i )
				Sys::sleep( EE::System::Milliseconds( 10 ) );
			ASSERT_FALSE( child.isAlive() );
		}
		int code = 0;
		ASSERT_TRUE( child.join( &code ) );
		EXPECT_EQ( code, 128 + SIGTERM );
	}
	EXPECT_EQ( Process::runForeground( "/bin/sh", { "-c", "kill -TERM $$" } ), 128 + SIGTERM );
}

UTEST( Process, gracefulStopLetsTheChildFlush ) {
	TempDir temp;
	Process child;
	ASSERT_TRUE( child.create(
		"/bin/sh",
		std::vector<std::string>{ "-c", "trap 'printf flushed > \"$FLUSH_PATH\"; exit 23' TERM; "
										"printf ready; while :; do sleep 0.1; done" },
		Process::getDefaultOptions() | Process::EnableAsync,
		{ { "FLUSH_PATH", temp.path + "flushed" } } ) );
	std::string ready;
	child.readAllStdOut( ready, EE::System::Seconds( 2 ) );
	ASSERT_STREQ( ready.c_str(), "ready" );
	ASSERT_TRUE( child.requestStop() );
	for ( unsigned i = 0; i < 200 && child.isAlive(); ++i )
		EE::System::Sys::sleep( EE::System::Milliseconds( 10 ) );
	ASSERT_FALSE( child.isAlive() );
	int code = 0;
	ASSERT_TRUE( child.join( &code ) );
	EXPECT_EQ( code, 23 );
	std::string flushed;
	ASSERT_TRUE( FileSystem::fileGet( temp.path + "flushed", flushed ) );
	EXPECT_STREQ( flushed.c_str(), "flushed" );
}

UTEST( Process, foregroundForwardsHangupAndBoundsAnUncooperativeChild ) {
	TempDir temp;
	const auto ready = temp.path + "ready";
	std::atomic<bool> finished{ false };
	std::thread stop( [&] {
		while ( !finished && !FileSystem::fileExists( ready ) )
			EE::System::Sys::sleep( EE::System::Milliseconds( 5 ) );
		if ( !finished )
			::kill( getpid(), SIGHUP );
	} );
	const int result = Process::runForeground(
		"/bin/sh",
		{ "-c", "trap '' HUP; printf ready > \"$READY_PATH\"; while :; do sleep 0.1; done" },
		{ { "READY_PATH", ready } }, "", EE::System::Milliseconds( 100 ) );
	finished = true;
	stop.join();
	EXPECT_EQ( result, 128 + SIGHUP );
}
#endif

UTEST( Process, inheritedStdioDoesNotCreateOrCloseBorrowedPipes ) {
	int code = -1;
	{
		Process child;
#if EE_PLATFORM == EE_PLATFORM_WIN
		const std::string executable = "cmd.exe";
		const std::vector<std::string> arguments{ "/D", "/Q", "/C", "exit /B 23" };
#else
		const std::string executable = "/bin/sh";
		const std::vector<std::string> arguments{ "-c", "exit 23" };
#endif
		ASSERT_TRUE( child.create( executable, arguments,
								   Process::getDefaultOptions() | Process::InheritStdio ) );
		EXPECT_TRUE( child.getStdIn() == nullptr );
		EXPECT_TRUE( child.getStdOut() == nullptr );
		EXPECT_TRUE( child.getStdErr() == nullptr );
		ASSERT_TRUE( child.join( &code ) );
		EXPECT_FALSE( child.requestStop() );
	}
	EXPECT_EQ( code, 23 );
	// Destruction must not fclose or otherwise take ownership of our inherited standard streams.
	EXPECT_EQ( fflush( stdout ), 0 );
	EXPECT_EQ( fflush( stderr ), 0 );
}

UTEST( Process, inheritedStdioRejectsPipeOnlyOptions ) {
	Process child;
	EXPECT_FALSE( child.create( "unused", std::vector<std::string>{},
								Process::InheritStdio | Process::EnableAsync ) );
	EXPECT_FALSE( child.create( "unused", std::vector<std::string>{},
								Process::InheritStdio | Process::CombinedStdoutStderr ) );
	EXPECT_FALSE( child.isAlive() );
}

#if EE_PLATFORM != EE_PLATFORM_WIN
UTEST( Process, inheritedStdinReadsFromTheCallersDescriptor ) {
	TempDir temp;
	const auto inputPath = temp.path + "input";
	const auto outputPath = temp.path + "output";
	ASSERT_TRUE( EE::System::FileSystem::fileWrite( inputPath, "inherited marker\n" ) );
	const int input = open( inputPath.c_str(), O_RDONLY );
	ASSERT_GE( input, 0 );
	const int original = dup( STDIN_FILENO );
	ASSERT_GE( original, 0 );
	Process child;
	bool created = false;
	{
		struct Restore {
			int original;
			int input;

			~Restore() {
				dup2( original, STDIN_FILENO );
				close( original );
				close( input );
			}
		} restore{ original, input };
		ASSERT_GE( dup2( input, STDIN_FILENO ), 0 );
		created = child.create(
			"/bin/sh", { "-c", "read token; printf '%s' \"$token\" > \"$1\"", "sh", outputPath },
			Process::InheritEnvironment | Process::InheritStdio );
	}
	ASSERT_TRUE( created );
	const auto deadline = MonotonicClock::now() + Seconds( 2 );
	while ( child.isAlive() && MonotonicClock::now() < deadline )
		Sys::sleep( Milliseconds( 1 ) );
	if ( child.isAlive() )
		child.kill();
	int code = -1;
	ASSERT_TRUE( child.join( &code ) );
	EXPECT_EQ( code, 0 );
	std::string data;
	ASSERT_TRUE( EE::System::FileSystem::fileGet( outputPath, data ) );
	EXPECT_STREQ( data.c_str(), "inherited marker" );
}
#endif

UTEST( Process, asyncReaderTeardownDoesNotWaitForChildInput ) {
	std::mutex mutex;
	std::condition_variable changed;
	std::string output;
	std::optional<Process> process( std::in_place );
#if EE_PLATFORM == EE_PLATFORM_WIN
	const std::string shell = "cmd.exe";
	const std::vector<std::string> arguments{ "/D", "/Q", "/C", "echo ready&set /p token=" };
#else
	const std::string shell = "/bin/sh";
	const std::vector<std::string> arguments{ "-c", "trap '' TERM; printf ready; read token" };
#endif
	ASSERT_TRUE( process->create( shell, arguments,
								  Process::getDefaultOptions() | Process::EnableAsync |
									  Process::CombinedStdoutStderr ) );
	process->startAsyncRead( [&]( const char* bytes, size_t count ) {
		std::lock_guard<std::mutex> lock( mutex );
		output.append( bytes, count );
		changed.notify_all();
	} );
	{
		std::unique_lock<std::mutex> lock( mutex );
		ASSERT_TRUE( changed.wait_for( lock, Seconds( 2 ).toChrono(), [&] {
			return output.find( "ready" ) != std::string::npos;
		} ) );
	}
	ASSERT_TRUE( process->isAlive() );
	const auto started = MonotonicClock::now();
	process.reset();
	const auto elapsed = ( MonotonicClock::now() - started );
	printf( "  Async process teardown: %lld ms\n",
			static_cast<long long>( elapsed.asMilliseconds() ) );
	EXPECT_LT( elapsed.asMilliseconds(), 1000 );
}

#if EE_PLATFORM != EE_PLATFORM_WIN
UTEST( Process, joinRetriesInterruptedWaits ) {
	TempDir temp;
	const std::string fifo = temp.path + "release";
	ASSERT_EQ( 0, ::mkfifo( fifo.c_str(), 0600 ) );
	struct sigaction action{};
	action.sa_handler = []( int ) {};
	(void)sigemptyset( &action.sa_mask );
	struct sigaction previous{};
	ASSERT_EQ( 0, ::sigaction( SIGUSR1, &action, &previous ) );
	struct RestoreSignal {
		const struct sigaction& previous;

		~RestoreSignal() { ::sigaction( SIGUSR1, &previous, nullptr ); }
	} restore{ previous };
	std::mutex mutex;
	std::condition_variable changed;
	std::string output;
	Process child;
	ASSERT_TRUE( child.create(
		"/bin/sh", { "-c", "printf '%s\\n' \"$$\"; read token < \"$1\"; exit 23", "sh", fifo },
		Process::getDefaultOptions() | Process::EnableAsync ) );
	child.startAsyncRead( [&]( const char* bytes, size_t count ) {
		std::lock_guard<std::mutex> lock( mutex );
		output.append( bytes, count );
		changed.notify_all();
	} );
	pid_t pid;
	{
		std::unique_lock<std::mutex> lock( mutex );
		ASSERT_TRUE( changed.wait_for( lock, Seconds( 2 ).toChrono(),
									   [&] { return output.find( '\n' ) != std::string::npos; } ) );
		pid = static_cast<pid_t>( std::strtol( output.c_str(), nullptr, 10 ) );
	}
	ASSERT_GT( pid, 0 );
	std::atomic<bool> finished{ false };
	bool joined = false;
	int code = -1;
	std::thread joining( [&] {
		joined = child.join( &code );
		finished = true;
	} );
	// The child waits on a separate FIFO: join's closing stdin cannot release it. Send
	// non-restarting signals while waitpid is blocked, then release the child explicitly.
	const auto until = MonotonicClock::now() + Milliseconds( 20 );
	while ( !finished && MonotonicClock::now() < until ) {
		::pthread_kill( joining.native_handle(), SIGUSR1 );
		Sys::sleep( Milliseconds( 1 ) );
	}
	int release = ::open( fifo.c_str(), O_RDWR | O_NONBLOCK );
	if ( release >= 0 ) {
		const char token[] = "go\n";
		EXPECT_EQ( static_cast<ssize_t>( sizeof( token ) - 1 ),
				   ::write( release, token, sizeof( token ) - 1 ) );
		::close( release );
	} else {
		// An I/O failure must not strand the joining thread or the child.
		::kill( pid, SIGTERM );
		EXPECT_TRUE( false );
	}
	joining.join();
	EXPECT_TRUE( joined );
	EXPECT_EQ( 23, code );
}

#endif

namespace {

/** Runs script in the platform shell (cmd.exe takes posix's place on Windows). */
bool createShell( Process& process, const char* posix, const char* windows ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	(void)posix;
	return process.create( "cmd.exe", std::vector<std::string>{ "/D", "/Q", "/C", windows },
						   Process::getDefaultOptions() | Process::EnableAsync );
#else
	(void)windows;
	return process.create( "/bin/sh", std::vector<std::string>{ "-c", posix },
						   Process::getDefaultOptions() | Process::EnableAsync );
#endif
}

/** A child that writes continuously, so that its reader is always delivering. */
bool createChatty( Process& process ) {
	return createShell( process, "while :; do echo chatter; done",
						"for /L %i in (0,0,1) do @echo chatter" );
}

} // namespace

UTEST( Process, aMissingReadCallbackIsIgnored ) {
	// SSHProcessTransport only reads stdout: a ProxyCommand's warning on stderr used to throw
	// std::bad_function_call on the reader thread.
	std::mutex mutex;
	std::condition_variable changed;
	std::string output;
	Process process;
	ASSERT_TRUE(
		createShell( process, "echo warning 1>&2; echo done", "echo warning 1>&2& echo done" ) );
	process.startAsyncRead( [&]( const char* bytes, size_t count ) {
		std::lock_guard<std::mutex> lock( mutex );
		output.append( bytes, count );
		changed.notify_all();
	} );
	std::unique_lock<std::mutex> lock( mutex );
	EXPECT_TRUE( changed.wait_for( lock, Seconds( 10 ).toChrono(),
								   [&] { return output.find( "done" ) != std::string::npos; } ) );
	EXPECT_EQ( std::string::npos, output.find( "warning" ) );
}

UTEST( Process, destroyingRightAfterStartingTheReadersIsSafe ) {
	// The readers used to read the process state when they started and keep using its pipes
	// after kill() had freed them: destroy at every point of a reader's life, including before
	// it started. Run under ASan/TSan to see the difference.
	std::atomic<bool> released{ false };
	std::atomic<int> lateCallbacks{ 0 };
	for ( int i = 0; i < 100; ++i ) {
		std::optional<Process> process( std::in_place );
		ASSERT_TRUE( createChatty( *process ) );
		process->startAsyncRead( [&]( const char*, size_t ) {
			if ( released )
				++lateCallbacks;
		} );
		if ( i % 2 )
			process->kill();
		process.reset();
		released = true;
		std::this_thread::yield();
		released = false;
	}
	EXPECT_EQ( 0, lateCallbacks.load() );
}

UTEST( Process, destroyStopsTheReadersBeforeClosingTheirPipes ) {
	std::atomic<int> active{ 0 };
	std::atomic<int> afterDestroy{ 0 };
	std::atomic<bool> destroyed{ false };
	std::atomic<bool> delivered{ false };
	Process process;
	// destroy() leaves a running child alone: write a bounded amount, so that it ends by itself
	// (Windows' cmd.exe keeps looping on a closed pipe).
	ASSERT_TRUE( createShell( process,
							  "i=0; while [ $i -lt 5000 ]; do echo chatter; i=$((i+1)); done",
							  "for /L %i in (1,1,5000) do @echo chatter" ) );
	process.startAsyncRead( [&]( const char*, size_t ) {
		++active;
		if ( destroyed )
			++afterDestroy;
		delivered = true;
		Sys::sleep( Milliseconds( 5 ) );
		--active;
	} );
	const auto until = MonotonicClock::now() + Seconds( 2 );
	while ( !delivered && MonotonicClock::now() < until )
		Sys::sleep( Milliseconds( 1 ) );
	ASSERT_TRUE( delivered.load() );
	process.destroy();
	destroyed = true;
	EXPECT_EQ( 0, active.load() );
	Sys::sleep( Milliseconds( 20 ) );
	EXPECT_EQ( 0, afterDestroy.load() );
}

UTEST( Process, destroyAfterJoinKeepsTheLastOutput ) {
	// Like the ecode build runner: join(), then destroy() while the reader still drains.
	for ( int i = 0; i < 5; ++i ) {
		std::mutex mutex;
		std::string output;
		Process process;
		ASSERT_TRUE( createShell( process, "printf first; sleep 0.01; printf last",
								  "echo first& ping -n 2 127.0.0.1 >nul & echo last" ) );
		process.startAsyncRead( [&]( const char* bytes, size_t count ) {
			bool first;
			{
				std::lock_guard<std::mutex> lock( mutex );
				first = output.empty();
				output.append( bytes, count );
			}
			// Still delivering "first" when the child exits: "last" is unread when destroy()
			// starts.
			if ( first )
				Sys::sleep( Milliseconds( 50 ) );
		} );
		int code = -1;
		EXPECT_TRUE( process.join( &code ) );
		process.destroy();
		std::lock_guard<std::mutex> lock( mutex );
		EXPECT_NE( std::string::npos, output.find( "first" ) );
		EXPECT_NE( std::string::npos, output.find( "last" ) );
	}
}

UTEST( Process, aKilledProcessCanBeReused ) {
	Process process;
	ASSERT_TRUE( createChatty( process ) );
	process.startAsyncRead( []( const char*, size_t ) {} );
	EXPECT_TRUE( process.kill() );
	std::mutex mutex;
	std::condition_variable changed;
	std::string output;
	ASSERT_TRUE( createShell( process, "echo $$; exec sleep 30",
							  "echo started& ping -n 30 127.0.0.1 >nul" ) );
	EXPECT_FALSE( process.isShuttingDown() );
	EXPECT_FALSE( process.killed() );
	process.startAsyncRead( [&]( const char* bytes, size_t count ) {
		std::lock_guard<std::mutex> lock( mutex );
		output.append( bytes, count );
		changed.notify_all();
	} );
	{
		std::unique_lock<std::mutex> lock( mutex );
		ASSERT_TRUE( changed.wait_for( lock, Seconds( 10 ).toChrono(),
									   [&] { return output.find( '\n' ) != std::string::npos; } ) );
	}
	ASSERT_TRUE( process.isAlive() );
	// The stale flags made this kill() return at once, leaving the child running.
	EXPECT_TRUE( process.kill() );
	EXPECT_FALSE( process.isAlive() );
#if EE_PLATFORM != EE_PLATFORM_WIN
	const pid_t pid = static_cast<pid_t>( std::strtol( output.c_str(), nullptr, 10 ) );
	ASSERT_GT( pid, 0 );
	const auto until = MonotonicClock::now() + Seconds( 2 );
	bool gone = false;
	while ( !gone && MonotonicClock::now() < until ) {
		gone = ::kill( pid, 0 ) != 0 && errno == ESRCH;
		if ( !gone )
			Sys::sleep( Milliseconds( 5 ) );
	}
	EXPECT_TRUE( gone );
#endif
}

UTEST( Process, killNeverWaitsForAReadCallback ) {
	// Like LSPClientServer's destructor: kill() while holding a lock the read callback takes.
	std::mutex mutex;
	std::atomic<bool> delivering{ false };
	Process process;
	ASSERT_TRUE( createChatty( process ) );
	process.startAsyncRead( [&]( const char*, size_t ) {
		delivering = true;
		std::lock_guard<std::mutex> lock( mutex );
	} );
	std::unique_lock<std::mutex> lock( mutex );
	const auto until = MonotonicClock::now() + Seconds( 2 );
	while ( !delivering && MonotonicClock::now() < until )
		Sys::sleep( Milliseconds( 1 ) );
	ASSERT_TRUE( delivering.load() );
	std::atomic<bool> killed{ false };
	std::thread killing( [&] {
		process.kill();
		killed = true;
	} );
	const auto deadline = MonotonicClock::now() + Seconds( 2 );
	while ( !killed && MonotonicClock::now() < deadline )
		Sys::sleep( Milliseconds( 1 ) );
	EXPECT_TRUE( killed.load() );
	lock.unlock();
	killing.join();
}

#if EE_PLATFORM != EE_PLATFORM_WIN
UTEST( Process, killedChildrenAreReaped ) {
	std::mutex mutex;
	std::condition_variable changed;
	std::string output;
	Process child;
	ASSERT_TRUE( child.create( "/bin/sh",
							   std::vector<std::string>{ "-c", "echo $$; exec sleep 30" },
							   Process::getDefaultOptions() | Process::EnableAsync ) );
	child.startAsyncRead( [&]( const char* bytes, size_t count ) {
		std::lock_guard<std::mutex> lock( mutex );
		output.append( bytes, count );
		changed.notify_all();
	} );
	pid_t pid;
	{
		std::unique_lock<std::mutex> lock( mutex );
		ASSERT_TRUE( changed.wait_for( lock, Seconds( 2 ).toChrono(),
									   [&] { return output.find( '\n' ) != std::string::npos; } ) );
		pid = static_cast<pid_t>( std::strtol( output.c_str(), nullptr, 10 ) );
	}
	ASSERT_GT( pid, 0 );
	EXPECT_TRUE( child.kill() );
	// A zombie still answers kill( pid, 0 ); a reaped child is gone.
	const auto until = MonotonicClock::now() + Seconds( 2 );
	bool gone = false;
	while ( !gone && MonotonicClock::now() < until ) {
		gone = ::kill( pid, 0 ) != 0 && errno == ESRCH;
		if ( !gone )
			Sys::sleep( Milliseconds( 5 ) );
	}
	EXPECT_TRUE( gone );
}
#endif

UTEST( Process, teardownDoesNotWaitForAGrandchildHoldingThePipe ) {
	// The grandchild keeps the output pipe open after its parent is killed, without writing to
	// it: on Windows only cancelling the reader's pending read releases it.
	std::mutex mutex;
	std::condition_variable changed;
	std::string output;
	std::optional<Process> process( std::in_place );
	ASSERT_TRUE( createShell( process.value(), "sleep 3 & printf ready; wait",
							  "start /b cmd /D /C \"ping -n 6 127.0.0.1 >nul\"& echo ready& "
							  "ping -n 30 127.0.0.1 >nul" ) );
	process->startAsyncRead( [&]( const char* bytes, size_t count ) {
		std::lock_guard<std::mutex> lock( mutex );
		output.append( bytes, count );
		changed.notify_all();
	} );
	{
		std::unique_lock<std::mutex> lock( mutex );
		ASSERT_TRUE( changed.wait_for( lock, Seconds( 10 ).toChrono(), [&] {
			return output.find( "ready" ) != std::string::npos;
		} ) );
	}
	const auto started = MonotonicClock::now();
	process->kill();
	process.reset();
	const auto elapsed = ( MonotonicClock::now() - started );
	EXPECT_LT( elapsed.asMilliseconds(), 1000 );
}

#endif
