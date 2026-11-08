#include <eepp/config.hpp>
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
#include "utest.hpp"
#include <cstring>
#include <eepp/network/locallistener.hpp>
#include <eepp/network/socketselector.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/uuid.hpp>
using namespace EE;
using namespace EE::Network;
using namespace EE::System;

UTEST( LocalSocket, nativeStreamConnectsAndPreservesData ) {
	if ( Sys::isRunningOnWine() )
		UTEST_SKIP( "Wine does not support AF_UNIX sockets" );
	// macOS's per-user TMPDIR can exceed sun_path. A random basename avoids collisions.
#if EE_PLATFORM == EE_PLATFORM_MACOS
	const auto path = "/tmp/eepp-local-" + UUID().toString();
#else
	const auto path = Sys::getTempPath() + "/eepp-local-" + UUID().toString();
#endif
	LocalListener listener;
	ASSERT_EQ( listener.listen( path ), Socket::Done );
	LocalListener competing;
	EXPECT_EQ( competing.listen( path ), Socket::Error );
	LocalSocket client;
	ASSERT_EQ( client.connect( path, Seconds( 2 ) ), Socket::Done );
	LocalSocket peer;
	ASSERT_EQ( listener.accept( peer ), Socket::Done );
	const char payload[] = "local\0stream";
	ASSERT_EQ( peer.send( payload, sizeof( payload ) ), Socket::Done );
	SocketSelector selector;
	selector.add( client );
	ASSERT_TRUE( selector.wait( Seconds( 2 ) ) );
	char buffer[64]{};
	size_t count = 0;
	ASSERT_EQ( client.receive( buffer, sizeof( buffer ), count ), Socket::Done );
	EXPECT_EQ( count, sizeof( payload ) );
	EXPECT_EQ( memcmp( buffer, payload, sizeof( payload ) ), 0 );
	ASSERT_EQ( client.send( payload, sizeof( payload ) ), Socket::Done );
	selector.clear();
	selector.add( peer );
	ASSERT_TRUE( selector.wait( Seconds( 2 ) ) );
	ASSERT_EQ( peer.receive( buffer, sizeof( buffer ), count ), Socket::Done );
	EXPECT_EQ( count, sizeof( payload ) );
	listener.close();
	EXPECT_FALSE( FileSystem::fileExists( path ) );
}

UTEST( LocalSocket, rejectsEmptyNulAndOverlongPaths ) {
	LocalSocket socket;
	EXPECT_EQ( socket.connect( "" ), Socket::Error );
	EXPECT_EQ( socket.connect( std::string( "bad\0path", 8 ) ), Socket::Error );
	EXPECT_EQ( socket.connect( std::string( 4096, 'x' ) ), Socket::Error );
	LocalListener listener;
	EXPECT_EQ( listener.listen( "" ), Socket::Error );
}

#if EE_PLATFORM == EE_PLATFORM_LINUX
UTEST( LocalSocket, fullListenQueueIsAnErrorInsteadOfACompletedConnection ) {
	const auto path = Sys::getTempPath() + "/eepp-local-" + UUID().toString();
	LocalListener listener;
	ASSERT_EQ( listener.listen( path, 1 ), Socket::Done );
	// Linux permits backlog + 1 queued clients. The next connect returns EAGAIN rather
	// than starting an asynchronous connection; its fd still polls writable with SO_ERROR 0.
	LocalSocket first;
	LocalSocket second;
	ASSERT_EQ( first.connect( path ), Socket::Done );
	ASSERT_EQ( second.connect( path ), Socket::Done );
	LocalSocket overflow;
	EXPECT_EQ( overflow.connect( path, Milliseconds( 100 ) ), Socket::Error );
	LocalSocket nonblocking;
	nonblocking.setBlocking( false );
	EXPECT_EQ( nonblocking.connect( path ), Socket::Error );
}
#endif

#endif
