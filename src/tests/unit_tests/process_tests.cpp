#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_WIN || EE_PLATFORM == EE_PLATFORM_LINUX || \
	EE_PLATFORM == EE_PLATFORM_MACOS || EE_PLATFORM == EE_PLATFORM_BSD

#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <eepp/system/process.hpp>
#include <optional>
#if EE_PLATFORM != EE_PLATFORM_WIN
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

using namespace vfstest;
using EE::System::Process;

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
		ASSERT_TRUE( changed.wait_for( lock, std::chrono::seconds( 2 ), [&] {
			return output.find( "ready" ) != std::string::npos;
		} ) );
	}
	ASSERT_TRUE( process->isAlive() );
	const auto started = std::chrono::steady_clock::now();
	process.reset();
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - started );
	printf( "  Async process teardown: %lld ms\n", static_cast<long long>( elapsed.count() ) );
	EXPECT_LT( elapsed.count(), 1000 );
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
		ASSERT_TRUE( changed.wait_for( lock, std::chrono::seconds( 2 ),
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
	const auto until = std::chrono::steady_clock::now() + std::chrono::milliseconds( 20 );
	while ( !finished && std::chrono::steady_clock::now() < until ) {
		::pthread_kill( joining.native_handle(), SIGUSR1 );
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
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
	EXPECT_TRUE( changed.wait_for( lock, std::chrono::seconds( 10 ),
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
		std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
		--active;
	} );
	const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 2 );
	while ( !delivered && std::chrono::steady_clock::now() < until )
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	ASSERT_TRUE( delivered.load() );
	process.destroy();
	destroyed = true;
	EXPECT_EQ( 0, active.load() );
	std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
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
				std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
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
		ASSERT_TRUE( changed.wait_for( lock, std::chrono::seconds( 10 ),
									   [&] { return output.find( '\n' ) != std::string::npos; } ) );
	}
	ASSERT_TRUE( process.isAlive() );
	// The stale flags made this kill() return at once, leaving the child running.
	EXPECT_TRUE( process.kill() );
	EXPECT_FALSE( process.isAlive() );
#if EE_PLATFORM != EE_PLATFORM_WIN
	const pid_t pid = static_cast<pid_t>( std::strtol( output.c_str(), nullptr, 10 ) );
	ASSERT_GT( pid, 0 );
	const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 2 );
	bool gone = false;
	while ( !gone && std::chrono::steady_clock::now() < until ) {
		gone = ::kill( pid, 0 ) != 0 && errno == ESRCH;
		if ( !gone )
			std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
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
	const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 2 );
	while ( !delivering && std::chrono::steady_clock::now() < until )
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
	ASSERT_TRUE( delivering.load() );
	std::atomic<bool> killed{ false };
	std::thread killing( [&] {
		process.kill();
		killed = true;
	} );
	const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds( 2 );
	while ( !killed && std::chrono::steady_clock::now() < deadline )
		std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );
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
		ASSERT_TRUE( changed.wait_for( lock, std::chrono::seconds( 2 ),
									   [&] { return output.find( '\n' ) != std::string::npos; } ) );
		pid = static_cast<pid_t>( std::strtol( output.c_str(), nullptr, 10 ) );
	}
	ASSERT_GT( pid, 0 );
	EXPECT_TRUE( child.kill() );
	// A zombie still answers kill( pid, 0 ); a reaped child is gone.
	const auto until = std::chrono::steady_clock::now() + std::chrono::seconds( 2 );
	bool gone = false;
	while ( !gone && std::chrono::steady_clock::now() < until ) {
		gone = ::kill( pid, 0 ) != 0 && errno == ESRCH;
		if ( !gone )
			std::this_thread::sleep_for( std::chrono::milliseconds( 5 ) );
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
		ASSERT_TRUE( changed.wait_for( lock, std::chrono::seconds( 10 ), [&] {
			return output.find( "ready" ) != std::string::npos;
		} ) );
	}
	const auto started = std::chrono::steady_clock::now();
	process->kill();
	process.reset();
	const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now() - started );
	EXPECT_LT( elapsed.count(), 1000 );
}

#endif
