#include "utest.hpp"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstring>
#include <eepp/system/ipc.hpp>
#include <eepp/system/md5.hpp>
#include <eepp/system/sys.hpp>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#if EE_PLATFORM != EE_PLATFORM_WIN && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

using namespace EE;
using namespace EE::System;

namespace {

std::string uniqueEndpoint( const char* suffix ) {
	static std::atomic<unsigned int> counter{ 0 };
	return "eepp-test." + std::to_string( Sys::getProcessID() ) + "." + suffix + "." +
		   std::to_string( counter++ );
}

struct Messages {
	void receive( const void* data, std::size_t size ) {
		std::lock_guard<std::mutex> lock( mutex );
		values.emplace_back( static_cast<const char*>( data ), size );
		condition.notify_all();
	}

	bool waitFor( std::size_t count ) {
		std::unique_lock<std::mutex> lock( mutex );
		return condition.wait_for( lock, std::chrono::seconds( 3 ),
								   [&] { return values.size() >= count; } );
	}

	std::mutex mutex;
	std::condition_variable condition;
	std::vector<std::string> values;
};

#if EE_PLATFORM != EE_PLATFORM_WIN && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN

std::string nativePath( const std::string& endpoint ) {
	std::string directory = Sys::getTempPath();
	if ( directory.back() != '/' )
		directory += '/';
	return directory + "eepp-ipc-" + std::to_string( static_cast<unsigned long long>( getuid() ) ) +
		   '/' + MD5::fromString( endpoint ).toHexString();
}

int rawConnect( const std::string& endpoint ) {
	const int fd = socket( AF_UNIX, SOCK_STREAM, 0 );
	if ( fd < 0 )
		return -1;
	sockaddr_un address{};
	address.sun_family = AF_UNIX;
	const std::string path = nativePath( endpoint );
	std::memcpy( address.sun_path, path.c_str(), path.size() + 1 );
	if ( connect( fd, reinterpret_cast<sockaddr*>( &address ), sizeof( address ) ) != 0 ) {
		close( fd );
		return -1;
	}
	return fd;
}

bool sendRaw( const std::string& endpoint, const std::vector<Uint8>& bytes ) {
	const int fd = rawConnect( endpoint );
	if ( fd < 0 )
		return false;
	const bool sent =
		static_cast<ssize_t>( bytes.size() ) == write( fd, bytes.data(), bytes.size() );
	close( fd );
	return sent;
}

#endif

} // namespace

UTEST( IPC, BasicBinaryAndEmptyMessages ) {
	IPC listener;
	Messages messages;
	const std::string endpoint = uniqueEndpoint( "basic" );
	ASSERT_EQ( IPC::Status::Done,
			   listener.listen( endpoint, [&]( const void* data, std::size_t size ) {
				   messages.receive( data, size );
			   } ) );

	EXPECT_EQ( IPC::Status::Done, IPC::send( endpoint, "hello" ) );
	const std::vector<Uint8> binary{ 0, 1, 2, 0, 255, 3 };
	EXPECT_EQ( IPC::Status::Done, IPC::send( endpoint, binary.data(), binary.size() ) );
	const std::string large( 4096, 'x' );
	EXPECT_EQ( IPC::Status::Done, IPC::send( endpoint, large ) );
	EXPECT_EQ( IPC::Status::Done, IPC::send( endpoint, nullptr, 0 ) );
	ASSERT_TRUE( messages.waitFor( 4 ) );
	ASSERT_EQ( static_cast<std::size_t>( 4 ), messages.values.size() );
	EXPECT_TRUE( std::string( "hello" ) == messages.values[0] );
	EXPECT_TRUE( std::string( reinterpret_cast<const char*>( binary.data() ), binary.size() ) ==
				 messages.values[1] );
	EXPECT_TRUE( large == messages.values[2] );
	EXPECT_TRUE( messages.values[3].empty() );
}

UTEST( IPC, SequentialClientsPreserveOrder ) {
	IPC listener;
	Messages messages;
	const std::string endpoint = uniqueEndpoint( "sequence" );
	ASSERT_EQ( IPC::Status::Done,
			   listener.listen( endpoint, [&]( const void* data, std::size_t size ) {
				   messages.receive( data, size );
			   } ) );
	for ( int i = 0; i < 32; ++i )
		ASSERT_EQ( IPC::Status::Done, IPC::send( endpoint, std::to_string( i ) ) );
	ASSERT_TRUE( messages.waitFor( 32 ) );
	for ( int i = 0; i < 32; ++i )
		EXPECT_TRUE( std::to_string( i ) == messages.values[i] );
}

UTEST( IPC, OwnershipIsolationMissingAndOversized ) {
	IPC first;
	IPC duplicate;
	IPC isolated;
	const std::string endpoint = uniqueEndpoint( "ownership" );
	ASSERT_EQ( IPC::Status::Done, first.listen( endpoint, []( const void*, std::size_t ) {} ) );
	EXPECT_EQ( IPC::Status::AlreadyExists,
			   duplicate.listen( endpoint, []( const void*, std::size_t ) {} ) );
	EXPECT_EQ( IPC::Status::Done,
			   isolated.listen( endpoint + ".other", []( const void*, std::size_t ) {} ) );
	EXPECT_EQ( IPC::Status::NotFound, IPC::send( uniqueEndpoint( "missing" ), "message" ) );
	const Uint8 byte = 0;
	EXPECT_EQ( IPC::Status::MessageTooLarge,
			   IPC::send( endpoint, &byte, IPC::MaxMessageSize + 1 ) );
}

UTEST( IPC, CloseAndRelistenWhileBlocked ) {
	IPC listener;
	const std::string endpoint = uniqueEndpoint( "relisten" );
	ASSERT_EQ( IPC::Status::Done, listener.listen( endpoint, []( const void*, std::size_t ) {} ) );
	listener.close();
	EXPECT_FALSE( listener.isListening() );
	ASSERT_EQ( IPC::Status::Done, listener.listen( endpoint, []( const void*, std::size_t ) {} ) );
	listener.close();
}

#if EE_PLATFORM != EE_PLATFORM_WIN && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN

UTEST( IPC, RejectsInvalidFramesAndPartialDisconnects ) {
	IPC listener;
	Messages messages;
	const std::string endpoint = uniqueEndpoint( "frames" );
	ASSERT_EQ( IPC::Status::Done,
			   listener.listen( endpoint, [&]( const void* data, std::size_t size ) {
				   messages.receive( data, size );
			   } ) );

	ASSERT_TRUE( sendRaw( endpoint, { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 } ) );
	ASSERT_TRUE( sendRaw( endpoint, { 'E', 'E', 'I', 'P', 0, 2, 0, 0, 0, 0, 0, 0 } ) );
	ASSERT_TRUE( sendRaw( endpoint, { 'E', 'E', 'I', 'P', 0, 1, 0, 1, 0, 0, 0, 0 } ) );
	ASSERT_TRUE( sendRaw( endpoint, { 'E', 'E', 'I', 'P', 0, 1, 0, 0, 1, 0, 0, 1 } ) );
	ASSERT_TRUE( sendRaw( endpoint, { 'E', 'E', 'I', 'P', 0, 1, 0, 0, 0, 0 } ) );
	ASSERT_TRUE( sendRaw( endpoint, { 'E', 'E', 'I', 'P', 0, 1, 0, 0, 0, 0, 0, 8, 1, 2 } ) );

	EXPECT_EQ( IPC::Status::Done, IPC::send( endpoint, "still-alive" ) );
	ASSERT_TRUE( messages.waitFor( 1 ) );
	EXPECT_TRUE( std::string( "still-alive" ) == messages.values[0] );
}

UTEST( IPC, CloseInterruptsPartialClient ) {
	IPC listener;
	const std::string endpoint = uniqueEndpoint( "partial-close" );
	ASSERT_EQ( IPC::Status::Done, listener.listen( endpoint, []( const void*, std::size_t ) {} ) );
	const int fd = rawConnect( endpoint );
	ASSERT_TRUE( fd >= 0 );
	const Uint8 partial[]{ 'E', 'E' };
	ASSERT_EQ( static_cast<ssize_t>( sizeof( partial ) ), write( fd, partial, sizeof( partial ) ) );
	std::this_thread::sleep_for( std::chrono::milliseconds( 20 ) );
	listener.close();
	close( fd );
}

UTEST( IPC, RecoversStaleSocketWithoutRemovingLiveSocket ) {
	const std::string endpoint = uniqueEndpoint( "stale" );
	IPC directoryCreator;
	ASSERT_EQ( IPC::Status::Done, directoryCreator.listen( endpoint + ".prepare",
														   []( const void*, std::size_t ) {} ) );
	directoryCreator.close();

	const int stale = socket( AF_UNIX, SOCK_STREAM, 0 );
	ASSERT_TRUE( stale >= 0 );
	sockaddr_un address{};
	address.sun_family = AF_UNIX;
	const std::string path = nativePath( endpoint );
	std::memcpy( address.sun_path, path.c_str(), path.size() + 1 );
	ASSERT_EQ( 0, bind( stale, reinterpret_cast<sockaddr*>( &address ), sizeof( address ) ) );
	close( stale );

	IPC listener;
	ASSERT_EQ( IPC::Status::Done, listener.listen( endpoint, []( const void*, std::size_t ) {} ) );
	IPC duplicate;
	EXPECT_EQ( IPC::Status::AlreadyExists,
			   duplicate.listen( endpoint, []( const void*, std::size_t ) {} ) );
}

#endif
