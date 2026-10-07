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
