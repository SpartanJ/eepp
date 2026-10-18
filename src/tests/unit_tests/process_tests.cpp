#include <eepp/config.hpp>

#if EE_PLATFORM == EE_PLATFORM_LINUX || EE_PLATFORM == EE_PLATFORM_MACOS || \
	EE_PLATFORM == EE_PLATFORM_BSD

#include "utest.hpp"
#include "vfs_test_utils.hpp"
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <eepp/system/process.hpp>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace vfstest;
using EE::System::Process;

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
