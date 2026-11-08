#include "highfiledescriptors.hpp"
#include "utest.h"

#include <eepp/network/ipaddress.hpp>
#include <eepp/network/socketselector.hpp>
#include <eepp/network/tcplistener.hpp>
#include <eepp/network/tcpsocket.hpp>
#include <eepp/system/clock.hpp>

using namespace EE;
using namespace EE::Network;
using namespace EE::System;

static void checkSelectorLifecycle( utest_state_s& utest_state, int* utest_result ) {
	(void)utest_state;

	TcpListener listener;
	ASSERT_EQ( Socket::Done, listener.listen( Socket::AnyPort, IpAddress::LocalHost ) );

	SocketSelector selector;
	selector.add( listener );

	EXPECT_FALSE( selector.wait( Milliseconds( 1 ) ) );
	EXPECT_FALSE( selector.isReady( listener ) );

	// Negative timeouts are an immediate check, never an infinite wait.
	Clock clock;
	EXPECT_FALSE( selector.wait( Milliseconds( -5 ) ) );
	EXPECT_TRUE( clock.getElapsedTime() < Seconds( 1 ) );

	TcpSocket clientA;
	ASSERT_EQ( Socket::Done,
			   clientA.connect( IpAddress::LocalHost, listener.getLocalPort(), Seconds( 5 ) ) );
	ASSERT_TRUE( selector.wait( Seconds( 5 ) ) );
	ASSERT_TRUE( selector.isReady( listener ) );
	TcpSocket acceptedA;
	ASSERT_EQ( Socket::Done, listener.accept( acceptedA ) );
	selector.add( acceptedA );

	TcpSocket clientB;
	ASSERT_EQ( Socket::Done,
			   clientB.connect( IpAddress::LocalHost, listener.getLocalPort(), Seconds( 5 ) ) );
	ASSERT_TRUE( selector.wait( Seconds( 5 ) ) );
	ASSERT_TRUE( selector.isReady( listener ) );
	TcpSocket acceptedB;
	ASSERT_EQ( Socket::Done, listener.accept( acceptedB ) );
	selector.add( acceptedB );
	// Adding a socket twice must not register it twice: a single remove() drops it later.
	selector.add( acceptedB );

	// Removing the first registered socket must keep the others observable.
	selector.remove( listener );

	char value = 'b';
	ASSERT_EQ( Socket::Done, clientB.send( &value, sizeof( value ) ) );
	ASSERT_TRUE( selector.wait( Seconds( 5 ) ) );
	EXPECT_TRUE( selector.isReady( acceptedB ) );
	EXPECT_FALSE( selector.isReady( acceptedA ) );
	EXPECT_FALSE( selector.isReady( listener ) );

	char receivedValue = 0;
	std::size_t received = 0;
	ASSERT_EQ( Socket::Done,
			   acceptedB.receive( &receivedValue, sizeof( receivedValue ), received ) );
	ASSERT_EQ( static_cast<std::size_t>( 1 ), received );
	EXPECT_TRUE( receivedValue == 'b' );

	EXPECT_FALSE( selector.wait( Milliseconds( 1 ) ) );
	EXPECT_FALSE( selector.isReady( acceptedB ) );

	value = 'a';
	ASSERT_EQ( Socket::Done, clientA.send( &value, sizeof( value ) ) );
	ASSERT_TRUE( selector.wait( Seconds( 5 ) ) );
	EXPECT_TRUE( selector.isReady( acceptedA ) );
	EXPECT_FALSE( selector.isReady( acceptedB ) );
	ASSERT_EQ( Socket::Done,
			   acceptedA.receive( &receivedValue, sizeof( receivedValue ), received ) );
	EXPECT_TRUE( receivedValue == 'a' );

	// A peer disconnection must report the socket as ready so receive() can discover it.
	selector.remove( acceptedA );
	clientB.disconnect();
	ASSERT_TRUE( selector.wait( Seconds( 5 ) ) );
	EXPECT_TRUE( selector.isReady( acceptedB ) );
	EXPECT_EQ( Socket::Disconnected,
			   acceptedB.receive( &receivedValue, sizeof( receivedValue ), received ) );

	selector.remove( acceptedB );
	clientA.disconnect();
	EXPECT_FALSE( selector.wait( Milliseconds( 1 ) ) );
	EXPECT_FALSE( selector.isReady( acceptedA ) );
}

UTEST( SocketSelector, reportsReadinessForListenersAndClients ) {
	checkSelectorLifecycle( utest_state, utest_result );
}

#if EE_PLATFORM != EE_PLATFORM_WIN
UTEST( SocketSelector, handlesFileDescriptorsAboveFdSetSize ) {
	HighFileDescriptorReservation highFds;
	if ( !highFds.isReady() )
		UTEST_SKIP( "could not reserve enough file descriptors" );

	checkSelectorLifecycle( utest_state, utest_result );
}
#endif

UTEST( SocketSelector, emptySelectorWaitsForTimeoutButNeverForever ) {
	SocketSelector selector;

	Clock clock;
	EXPECT_FALSE( selector.wait( Milliseconds( 20 ) ) );
	EXPECT_TRUE( clock.getElapsedTime() >= Milliseconds( 15 ) );

	// Nothing could ever become ready, so an infinite wait must return instead of hanging.
	clock.restart();
	EXPECT_FALSE( selector.wait() );
	EXPECT_TRUE( clock.getElapsedTime() < Seconds( 1 ) );
}

UTEST( SocketSelector, writableInterestsMergeWithoutChangingReadReadiness ) {
	TcpListener listener;
	ASSERT_EQ( listener.listen( Socket::AnyPort, IpAddress::LocalHost ), Socket::Done );
	TcpSocket client;
	ASSERT_EQ( client.connect( IpAddress::LocalHost, listener.getLocalPort(), Seconds( 2 ) ),
			   Socket::Done );
	TcpSocket peer;
	ASSERT_EQ( listener.accept( peer ), Socket::Done );
	SocketSelector selector;
	selector.add( client, SocketSelector::Event::Write );
	selector.add( client );
	ASSERT_TRUE( selector.wait( Seconds( 2 ) ) );
	EXPECT_TRUE( selector.isReady( client, SocketSelector::Event::Write ) );
	EXPECT_FALSE( selector.isReady( client ) );
	selector.remove( client );
	EXPECT_FALSE( selector.wait( Milliseconds( 1 ) ) );
}

#if defined( EE_PLATFORM_POSIX ) && EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
#include <eepp/system/sys.hpp>
#include <pthread.h>
#include <signal.h>
#include <thread>

UTEST( SocketSelector, interruptedWaitRetainsItsDeadline ) {
	TcpListener listener;
	ASSERT_EQ( listener.listen( Socket::AnyPort, IpAddress::LocalHost ), Socket::Done );
	SocketSelector selector;
	selector.add( listener );
	struct sigaction action{}, previous{};
	action.sa_handler = +[]( int ) {};
	sigemptyset( &action.sa_mask );
	ASSERT_EQ( sigaction( SIGUSR1, &action, &previous ), 0 );
	struct Restore {
		struct sigaction previous;
		~Restore() { sigaction( SIGUSR1, &previous, nullptr ); }
	} restore{ previous };
	const auto target = pthread_self();
	std::thread interrupt( [target] {
		for ( unsigned i = 0; i < 3; ++i ) {
			Sys::sleep( Milliseconds( 10 ) );
			pthread_kill( target, SIGUSR1 );
		}
	} );
	Clock clock;
	const bool ready = selector.wait( Milliseconds( 80 ) );
	const auto elapsed = clock.getElapsedTime();
	interrupt.join();
	EXPECT_FALSE( ready );
	EXPECT_TRUE( elapsed >= Milliseconds( 70 ) );
	EXPECT_TRUE( elapsed < Seconds( 1 ) );
}
#endif
