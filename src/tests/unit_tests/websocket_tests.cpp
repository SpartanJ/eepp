#include "utest.hpp"
#include <atomic>
#include <cstring>
#include <eepp/network/socketselector.hpp>
#include <eepp/network/tcplistener.hpp>
#include <eepp/network/websocket.hpp>
#include <eepp/system/sys.hpp>
#include <thread>

using namespace EE;
using namespace EE::Network;
using namespace EE::System;

UTEST( WebSocket, publishedHandshakeVectorAndRequestValidation ) {
	const auto accepted = WebSocket::acceptKey( "dGhlIHNhbXBsZSBub25jZQ==" );
	EXPECT_STREQ( accepted.c_str(), "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=" );
	const std::string request =
		"GET /chat HTTP/1.1\r\nHost: server.example.com\r\n"
		"Upgrade: websocket\r\nConnection: keep-alive, Upgrade\r\n"
		"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nSec-WebSocket-Version: 13\r\n\r\n";
	EXPECT_FALSE( WebSocket::serverResponse( request ).empty() );
	EXPECT_TRUE( WebSocket::serverResponse( "GET / HTTP/1.1\r\n\r\n" ).empty() );
	auto invalid = request;
	invalid.replace( invalid.find( "13" ), 2, "12" );
	EXPECT_TRUE( WebSocket::serverResponse( invalid ).empty() );
	EXPECT_TRUE( WebSocket::serverResponse( request + std::string( 65536, 'x' ) ).empty() );
}

UTEST( WebSocket, maskedFragmentationAndInterleavedControlFrames ) {
	std::vector<std::pair<WebSocket::Opcode, std::string>> seen;
	WebSocket::Decoder decoder(
		WebSocket::Mask::Required,
		[&]( WebSocket::Opcode op, std::string_view data ) { seen.emplace_back( op, data ); } );
	auto first = WebSocket::encode( "hel", WebSocket::Opcode::Text, WebSocket::Role::Client );
	first[0] &= 127;
	auto continuation = WebSocket::encode( "lo", WebSocket::Opcode::Text, WebSocket::Role::Client );
	continuation[0] = static_cast<char>( 128 );
	const auto wire =
		first + WebSocket::encode( "ping", WebSocket::Opcode::Ping, WebSocket::Role::Client ) +
		continuation;
	for ( const char byte : wire )
		ASSERT_TRUE( decoder.feed( std::string_view( &byte, 1 ) ) );
	ASSERT_EQ( seen.size(), 2u );
	EXPECT_EQ( seen[0].first, WebSocket::Opcode::Ping );
	EXPECT_STREQ( seen[0].second.c_str(), "ping" );
	EXPECT_EQ( seen[1].first, WebSocket::Opcode::Text );
	EXPECT_STREQ( seen[1].second.c_str(), "hello" );
}

UTEST( WebSocket, extendedLengthsLimitsAndCaptureSkipping ) {
	for ( const size_t size : { 0u, 125u, 126u, 65535u, 65536u } ) {
		const std::string value( size, 'x' );
		std::string received;
		WebSocket::Decoder decoder(
			WebSocket::Mask::Forbidden,
			[&]( WebSocket::Opcode, std::string_view data ) { received.assign( data ); } );
		const auto wire =
			WebSocket::encode( value, WebSocket::Opcode::Binary, WebSocket::Role::Server );
		ASSERT_TRUE( decoder.feed( wire ) );
		EXPECT_EQ( received.size(), value.size() );
	}
	const auto oversized = WebSocket::encode( std::string( 32, 'x' ), WebSocket::Opcode::Text,
											  WebSocket::Role::Server );
	WebSocket::Decoder strict( WebSocket::Mask::Forbidden, {}, 16 );
	EXPECT_FALSE( strict.feed( oversized ) );
	int calls = 0;
	bool capture = false;
	WebSocket::Decoder observer(
		WebSocket::Mask::Forbidden, [&]( WebSocket::Opcode, std::string_view ) { ++calls; }, 16,
		[&] { return capture; }, true );
	EXPECT_TRUE( observer.feed( oversized ) );
	capture = true;
	EXPECT_TRUE( observer.feed( oversized ) );
	EXPECT_EQ( calls, 0 );
	EXPECT_TRUE( observer.feed(
		WebSocket::encode( "ok", WebSocket::Opcode::Text, WebSocket::Role::Server ) ) );
	EXPECT_EQ( calls, 1 );
}

UTEST( WebSocket, rejectsProtocolViolationsAndStopsExactlyAtMessageBoundaries ) {
	const auto server = WebSocket::encode( "ok", WebSocket::Opcode::Text, WebSocket::Role::Server );
	WebSocket::Decoder masking( WebSocket::Mask::Required, {} );
	EXPECT_FALSE( masking.feed( server ) );
	for ( const std::string& invalid :
		  { std::string( "\x83\x00", 2 ), std::string( "\x09\x00", 2 ),
			std::string( "\x80\x00", 2 ), std::string( "\x81\x7e\x00\x01x", 5 ),
			std::string( "\x81\x01\xff", 3 ), std::string( "\x88\x01x", 3 ),
			std::string( "\x88\x02\x03\xed", 4 ) } ) {
		WebSocket::Decoder decoder( WebSocket::Mask::Forbidden, {} );
		EXPECT_FALSE( decoder.feed( invalid ) );
	}
	int calls = 0;
	WebSocket::Decoder decoder( WebSocket::Mask::Forbidden,
								[&]( WebSocket::Opcode, std::string_view ) { ++calls; } );
	size_t consumed = 0;
	ASSERT_TRUE( decoder.feed( server + server, consumed, true ) );
	EXPECT_EQ( consumed, server.size() );
	EXPECT_EQ( calls, 1 );
}

UTEST( WebSocket, loopbackHandshakeMessagesPingAndClose ) {
	TcpListener listener;
	ASSERT_EQ( listener.listen( Socket::AnyPort, IpAddress::LocalHost ), Socket::Done );
	std::atomic<bool> succeeded{ false };
	std::thread worker( [&] {
		SocketSelector selector;
		selector.add( listener );
		if ( !selector.wait( Seconds( 2 ) ) )
			return;
		auto peer = std::make_unique<TcpSocket>();
		if ( listener.accept( *peer ) != Socket::Done )
			return;
		peer->setReceiveTimeout( Seconds( 2 ) );
		peer->setSendTimeout( Seconds( 2 ) );
		WebSocket websocket( std::move( peer ), WebSocket::Role::Server );
		std::string payload;
		WebSocket::Opcode opcode;
		if ( websocket.accept() != Socket::Done ||
			 websocket.receive( payload, opcode ) != Socket::Done || payload != "hello" ||
			 opcode != WebSocket::Opcode::Text ||
			 websocket.send( "probe", WebSocket::Opcode::Ping ) != Socket::Done ||
			 websocket.send( payload, WebSocket::Opcode::Binary ) != Socket::Done )
			return;
		succeeded = websocket.receive( payload, opcode ) == Socket::Disconnected;
	} );
	struct Join {
		std::thread& worker;

		~Join() {
			if ( worker.joinable() )
				worker.join();
		}
	} join{ worker };
	auto transport = std::make_unique<TcpSocket>();
	ASSERT_EQ( transport->connect( IpAddress::LocalHost, listener.getLocalPort(), Seconds( 2 ) ),
			   Socket::Done );
	transport->setSendTimeout( Seconds( 2 ) );
	transport->setReceiveTimeout( Seconds( 2 ) );
	WebSocket client( std::move( transport ), WebSocket::Role::Client );
	ASSERT_EQ( client.open( "localhost", "/test", "Authorization: Bearer test\r\n" ),
			   Socket::Done );
	ASSERT_EQ( client.send( "hello" ), Socket::Done );
	std::string received;
	WebSocket::Opcode opcode;
	ASSERT_EQ( client.receive( received, opcode ), Socket::Done );
	EXPECT_STREQ( received.c_str(), "hello" );
	EXPECT_EQ( opcode, WebSocket::Opcode::Binary );
	EXPECT_EQ( client.close(), Socket::Done );
	worker.join();
	// The guard normally joins on early assertions; do not join the completed worker twice.
	EXPECT_TRUE( succeeded.load() );
}

UTEST( WebSocket, exclusivelyOwnsAndDestroysItsPolymorphicTransport ) {
	bool destroyed = false;
	struct TrackedSocket : TcpSocket {
		bool& destroyed;

		explicit TrackedSocket( bool& destroyed ) : destroyed( destroyed ) {}

		~TrackedSocket() { destroyed = true; }
	};
	{
		auto transport = std::make_unique<TrackedSocket>( destroyed );
		WebSocket websocket( std::move( transport ), WebSocket::Role::Client );
		EXPECT_TRUE( transport == nullptr );
		EXPECT_FALSE( destroyed );
	}
	EXPECT_TRUE( destroyed );
	WebSocket empty( nullptr, WebSocket::Role::Client );
	EXPECT_EQ( empty.open( "localhost" ), Socket::Error );
}

UTEST( WebSocket, partialFrameSendPermanentlyClosesTheStream ) {
	struct PartialSocket : TcpSocket {
		std::string request{
			"GET / HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
			"Sec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n" };
		size_t offset{ 0 };
		unsigned sends{ 0 };
		bool disconnected{ false };

		Status receive( void* data, size_t size, size_t& count ) override {
			count = eemin( size, request.size() - offset );
			if ( !count )
				return Disconnected;
			std::memcpy( data, request.data() + offset, count );
			offset += count;
			return Done;
		}

		Status send( const void*, size_t ) override { return ++sends == 1 ? Done : Partial; }

		void disconnect() override { disconnected = true; }
	};
	auto transport = std::make_unique<PartialSocket>();
	auto* observed = transport.get();
	WebSocket websocket( std::move( transport ), WebSocket::Role::Server );
	ASSERT_EQ( websocket.accept(), Socket::Done );
	EXPECT_EQ( websocket.send( "message" ), Socket::Partial );
	EXPECT_TRUE( observed->disconnected );
	EXPECT_EQ( websocket.send( "next message" ), Socket::Error );
	EXPECT_EQ( observed->sends, 2u );
}

namespace {

// A server-side transport replaying a client handshake followed by scripted client frames.
struct ScriptedSocket : TcpSocket {
	std::string input{
		"GET / HTTP/1.1\r\nHost: localhost\r\nUpgrade: websocket\r\nConnection: Upgrade\r\n"
		"Sec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n" };
	size_t offset{ 0 };
	unsigned sends{ 0 };
	unsigned disconnects{ 0 };

	Status receive( void* data, size_t size, size_t& count ) override {
		count = eemin( size, input.size() - offset );
		if ( !count )
			return Disconnected;
		std::memcpy( data, input.data() + offset, count );
		offset += count;
		return Done;
	}

	Status send( const void*, size_t ) override {
		++sends;
		return Done;
	}

	void disconnect() override { ++disconnects; }
};

} // namespace

UTEST( WebSocket, receiveAfterLocalCloseWaitsForThePeersClose ) {
	auto transport = std::make_unique<ScriptedSocket>();
	auto* observed = transport.get();
	WebSocket websocket( std::move( transport ), WebSocket::Role::Server );
	ASSERT_EQ( websocket.accept(), Socket::Done );
	// Data still in flight from the peer, a ping, then the peer's Close.
	for ( const auto& frame :
		  { WebSocket::encode( "late", WebSocket::Opcode::Text, WebSocket::Role::Client ),
			WebSocket::encode( "probe", WebSocket::Opcode::Ping, WebSocket::Role::Client ),
			WebSocket::encode( std::string( "\x03\xe8", 2 ), WebSocket::Opcode::Close,
							   WebSocket::Role::Client ) } )
		observed->input += frame;
	ASSERT_EQ( websocket.close(), Socket::Done );
	std::string payload;
	WebSocket::Opcode opcode;
	EXPECT_EQ( websocket.receive( payload, opcode ), Socket::Disconnected );
	EXPECT_EQ( observed->offset, observed->input.size() );
	EXPECT_TRUE( payload.empty() );
	// The handshake response, our Close and the Pong: pings are answered until the peer's Close,
	// and its Close is not echoed after ours.
	EXPECT_EQ( observed->sends, 3u );
	// Both Close frames are exchanged, so TCP closes now.
	EXPECT_EQ( observed->disconnects, 1u );
	EXPECT_EQ( websocket.send( "after" ), Socket::Error );
}

UTEST( WebSocket, closeSentThroughSendEndsDataSending ) {
	WebSocket websocket( std::make_unique<ScriptedSocket>(), WebSocket::Role::Server );
	ASSERT_EQ( websocket.accept(), Socket::Done );
	ASSERT_EQ( websocket.send( std::string( "\x03\xe8", 2 ), WebSocket::Opcode::Close ),
			   Socket::Done );
	EXPECT_EQ( websocket.send( "data" ), Socket::Error );
	EXPECT_EQ( websocket.send( "pong", WebSocket::Opcode::Pong ), Socket::Done );
}

UTEST( WebSocket, protocolViolationPermanentlyFailsTheStream ) {
	auto transport = std::make_unique<ScriptedSocket>();
	auto* observed = transport.get();
	// A server requires masked client frames: this unmasked one violates the protocol.
	observed->input +=
		WebSocket::encode( "plain", WebSocket::Opcode::Text, WebSocket::Role::Server );
	WebSocket websocket( std::move( transport ), WebSocket::Role::Server );
	ASSERT_EQ( websocket.accept(), Socket::Done );
	std::string payload;
	WebSocket::Opcode opcode;
	EXPECT_EQ( websocket.receive( payload, opcode ), Socket::Error );
	EXPECT_EQ( observed->disconnects, 1u );
	EXPECT_EQ( websocket.send( "after" ), Socket::Error );
	EXPECT_EQ( websocket.receive( payload, opcode ), Socket::Error );
	EXPECT_EQ( observed->sends, 1u );
}

UTEST( WebSocket, passiveOversizedNotificationsDoNotPoisonLaterMessages ) {
	int messages = 0;
	WebSocket::Decoder decoder(
		WebSocket::Mask::Forbidden,
		[&]( WebSocket::Opcode, std::string_view data ) {
			EXPECT_TRUE( data == "ok" );
			++messages;
		},
		4, {}, true );
	for ( int pass = 0; pass < 2; ++pass ) {
		// The limit is crossed across fragments, not just by a single large frame.
		ASSERT_TRUE( decoder.feed( std::string( "\x01\x03"
												"abc",
												5 ) ) );
		EXPECT_FALSE( decoder.takeOversizedMessage() );
		ASSERT_TRUE( decoder.feed( std::string( "\x80\x03"
												"def",
												5 ) ) );
		EXPECT_TRUE( decoder.takeOversizedMessage() );
		EXPECT_FALSE( decoder.takeOversizedMessage() );
		ASSERT_TRUE( decoder.feed(
			WebSocket::encode( "ok", WebSocket::Opcode::Text, WebSocket::Role::Server ) ) );
	}
	EXPECT_EQ( messages, 2 );
}
