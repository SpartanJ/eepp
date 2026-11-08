#include <eepp/config.hpp>
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
#include "utest.hpp"
#include <eepp/network/streamrelay.hpp>
#include <eepp/network/tcplistener.hpp>
#include <eepp/network/tcpsocket.hpp>
#include <eepp/system/clock.hpp>
using namespace EE::Network;
using namespace EE::System;

UTEST( StreamRelay, boundedBidirectionalTransfersWithSlowReaders ) {
	TcpListener first, second;
	ASSERT_EQ( first.listen( 0, IpAddress::LocalHost ), Socket::Done );
	ASSERT_EQ( second.listen( 0, IpAddress::LocalHost ), Socket::Done );
	StreamRelay<TcpSocket, TcpSocket> relay;
	ASSERT_EQ( relay.left().connect( IpAddress::LocalHost, first.getLocalPort() ), Socket::Done );
	ASSERT_EQ( relay.right().connect( IpAddress::LocalHost, second.getLocalPort() ), Socket::Done );
	TcpSocket left, right;
	ASSERT_EQ( first.accept( left ), Socket::Done );
	ASSERT_EQ( second.accept( right ), Socket::Done );
	left.setBlocking( false );
	right.setBlocking( false );
	std::string data( 512 * 1024, 'x' );
	for ( size_t i = 0; i < data.size(); ++i )
		data[i] = static_cast<char>( i );
	size_t sentLeft = 0, sentRight = 0;
	std::string receivedLeft, receivedRight;
	Clock clock;
	unsigned iterations = 0;
	while ( clock.getElapsedTime() < Seconds( 5 ) &&
			( receivedLeft.size() < data.size() || receivedRight.size() < data.size() ) ) {
		SocketSelector selector;
		relay.watch( selector );
		selector.wait( Milliseconds( 1 ) );
		ASSERT_TRUE( relay.poll( selector ) );
		auto send = [&]( TcpSocket& socket, size_t& sent ) {
			if ( sent == data.size() )
				return;
			size_t count = 0;
			socket.send( data.data() + sent, data.size() - sent, count );
			sent += count;
		};
		send( left, sentLeft );
		send( right, sentRight );
		if ( ++iterations < 20 )
			continue;
		auto read = []( TcpSocket& socket, std::string& result ) {
			char buffer[4096];
			size_t count = 0;
			if ( socket.receive( buffer, sizeof( buffer ), count ) == Socket::Done )
				result.append( buffer, count );
		};
		read( left, receivedLeft );
		read( right, receivedRight );
	}
	EXPECT_TRUE( receivedLeft == data );
	EXPECT_TRUE( receivedRight == data );
}
#endif
