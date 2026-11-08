#include <eepp/network/websocket.hpp>
#include <utility>

#include <cstring>
#include <eepp/core/string.hpp>
#include <eepp/network/http.hpp>
#include <eepp/system/base64.hpp>
#include <eepp/system/sha1.hpp>
#include <eepp/system/sys.hpp>
#include <sstream>
#include <thirdparty/simdutf/simdutf.h>

using namespace EE::System;

namespace EE::Network {

namespace {

bool validOpcode( WebSocket::Opcode opcode ) {
	return opcode == WebSocket::Opcode::Continuation || opcode == WebSocket::Opcode::Text ||
		   opcode == WebSocket::Opcode::Binary || opcode == WebSocket::Opcode::Close ||
		   opcode == WebSocket::Opcode::Ping || opcode == WebSocket::Opcode::Pong;
}

bool validClose( std::string_view payload ) {
	if ( payload.empty() )
		return true;
	if ( payload.size() == 1 )
		return false;
	const auto code = ( static_cast<Uint8>( payload[0] ) << 8 ) | static_cast<Uint8>( payload[1] );
	if ( !( ( code >= 1000 && code <= 1014 && code != 1004 && code != 1005 && code != 1006 ) ||
			( code >= 3000 && code < 5000 ) ) )
		return false;
	return simdutf::validate_utf8( payload.data() + 2, payload.size() - 2 );
}

bool headerToken( std::string_view header, std::string_view field, std::string_view token ) {
	const auto value = WebSocket::headerValue( header, field );
	std::string_view remaining = value;
	while ( !remaining.empty() ) {
		const auto comma = remaining.find( ',' );
		if ( String::iequals( String::trim( remaining.substr( 0, comma ), " \t\r" ), token ) )
			return true;
		if ( comma == std::string_view::npos )
			break;
		remaining.remove_prefix( comma + 1 );
	}
	return false;
}

bool validTarget( std::string_view value ) {
	return !value.empty() && value.find_first_of( "\r\n\0", 0, 3 ) == std::string::npos;
}

} // namespace

WebSocket::Decoder::Decoder( Mask mask, MessageFn onMessage, size_t maxMessageSize,
							 CaptureFn capture, bool skipOversized ) :
	mOnMessage( std::move( onMessage ) ),
	mCapture( std::move( capture ) ),
	mLimit( maxMessageSize ),
	mMask( mask ),
	mSkipOversized( skipOversized ) {}

bool WebSocket::Decoder::takeOversizedMessage() {
	return std::exchange( mOversized, false );
}

bool WebSocket::Decoder::feed( std::string_view data ) {
	size_t consumed = 0;
	return feed( data, consumed, false );
}

bool WebSocket::Decoder::feed( std::string_view data, size_t& consumed, bool stopAfterMessage ) {
	struct Consumption {
		std::string_view& remaining;
		size_t initial;
		size_t& consumed;

		~Consumption() { consumed = initial - remaining.size(); }
	} consumption{ data, data.size(), consumed };
	while ( !data.empty() && !mFailed ) {
		if ( !mRemaining ) {
			const auto count = std::min( mNeeded - mUsed, data.size() );
			memcpy( mHeader.data() + mUsed, data.data(), count );
			mUsed += count;
			data.remove_prefix( count );
			if ( mUsed < mNeeded )
				return true;
			if ( mNeeded == 2 ) {
				const auto size = mHeader[1] & 127;
				mNeeded += ( size == 126 ? 2 : size == 127 ? 8 : 0 ) + ( mHeader[1] & 128 ? 4 : 0 );
				if ( mUsed < mNeeded )
					continue;
			}
			mOpcode = static_cast<Opcode>( mHeader[0] & 15 );
			mFinal = ( mHeader[0] & 128 ) != 0;
			const bool masked = ( mHeader[1] & 128 ) != 0;
			mRemaining = mHeader[1] & 127;
			const auto indicator = mRemaining;
			if ( indicator >= 126 ) {
				mRemaining = 0;
				for ( size_t i = 0; i < ( indicator == 126 ? 2u : 8u ); ++i )
					mRemaining = ( mRemaining << 8 ) | mHeader[2 + i];
			}
			const bool control = static_cast<Uint8>( mOpcode ) >= 8;
			if ( ( mHeader[0] & 0x70 ) || !validOpcode( mOpcode ) ||
				 ( mMask == Mask::Required && !masked ) || ( mMask == Mask::Forbidden && masked ) ||
				 ( indicator == 126 && mRemaining < 126 ) ||
				 ( indicator == 127 && ( mRemaining < 65536 || ( mHeader[2] & 128 ) ) ) ||
				 ( control && ( !mFinal || mRemaining > 125 ) ) ) {
				mFailed = true;
				return false;
			}
			mOffset = 0;
			if ( control ) {
				mControl.clear();
			} else {
				if ( mOpcode == Opcode::Continuation ) {
					if ( mMessageOpcode == Opcode::Continuation ) {
						mFailed = true;
						return false;
					}
				} else {
					if ( mMessageOpcode != Opcode::Continuation ) {
						mFailed = true;
						return false;
					}
					mMessageOpcode = mOpcode;
					mMessage.clear();
					mDiscard = mCapture && !mCapture();
				}
				if ( !mDiscard &&
					 ( mRemaining > mLimit || mRemaining + mMessage.size() > mLimit ) ) {
					if ( !mSkipOversized ) {
						mFailed = true;
						return false;
					}
					mOversized = true;
					mDiscard = true;
					mMessage.clear();
				}
			}
		}
		const bool control = static_cast<Uint8>( mOpcode ) >= 8;
		const auto count = static_cast<size_t>( std::min<Uint64>( data.size(), mRemaining ) );
		if ( control || !mDiscard ) {
			auto& output = control ? mControl : mMessage;
			const auto start = output.size();
			output.append( data.data(), count );
			if ( mHeader[1] & 128 ) {
				for ( size_t i = 0; i < count; ++i )
					output[start + i] ^= mHeader[mNeeded - 4 + ( ( mOffset + i ) % 4 )];
			}
		}
		data.remove_prefix( count );
		mOffset += count;
		mRemaining -= count;
		if ( mRemaining )
			continue;
		bool delivered = false;
		if ( control ) {
			delivered = true;
			if ( mOpcode == Opcode::Close && !validClose( mControl ) ) {
				mFailed = true;
				return false;
			}
			if ( mOnMessage )
				mOnMessage( mOpcode, mControl );
		} else if ( mFinal ) {
			if ( !mDiscard ) {
				delivered = true;
				if ( mMessageOpcode == Opcode::Text &&
					 !simdutf::validate_utf8( mMessage.data(), mMessage.size() ) ) {
					mFailed = true;
					return false;
				}
				if ( mOnMessage )
					mOnMessage( mMessageOpcode, mMessage );
			}
			mMessageOpcode = Opcode::Continuation;
			mMessage.clear();
			if ( mMessage.capacity() > 65536 )
				std::string{}.swap( mMessage );
		}
		mUsed = 0;
		mNeeded = 2;
		if ( stopAfterMessage && delivered )
			return true;
	}
	return !mFailed;
}

std::string WebSocket::encode( std::string_view payload, Opcode opcode, Role sender ) {
	const auto size = payload.size();
	if ( !validOpcode( opcode ) || opcode == Opcode::Continuation ||
		 ( static_cast<Uint8>( opcode ) >= 8 && size > 125 ) ||
		 ( opcode == Opcode::Text && !simdutf::validate_utf8( payload.data(), size ) ) ||
		 ( opcode == Opcode::Close && !validClose( payload ) ) )
		return {};
	std::array<Uint8, 4> mask{};
	const bool masked = sender == Role::Client;
	if ( masked && !Sys::getRandomBytes( mask.data(), mask.size() ) )
		return {};
	std::string frame;
	frame.reserve( size + 14 );
	frame += static_cast<char>( 128 | static_cast<Uint8>( opcode ) );
	frame += static_cast<char>( ( masked ? 128 : 0 ) | ( size < 126		 ? size
														 : size <= 65535 ? 126
																		 : 127 ) );
	if ( size >= 126 ) {
		for ( int shift = size <= 65535 ? 8 : 56; shift >= 0; shift -= 8 )
			frame += static_cast<char>( ( static_cast<Uint64>( size ) >> shift ) & 255 );
	}
	if ( masked )
		frame.append( reinterpret_cast<const char*>( mask.data() ), mask.size() );
	const auto start = frame.size();
	frame.append( payload );
	if ( masked ) {
		for ( size_t i = 0; i < size; ++i )
			frame[start + i] ^= mask[i % 4];
	}
	return frame;
}

std::string WebSocket::headerValue( std::string_view header, std::string_view name ) {
	const auto start = header.find( "\r\n" );
	if ( start == std::string_view::npos )
		return {};
	std::istringstream fields( std::string( header.substr( start + 2 ) ) );
	Http::Response parsed;
	return parsed.parseFields( fields ) ? parsed.getField( std::string( name ) ) : std::string{};
}

std::string WebSocket::acceptKey( std::string_view clientKey ) {
	const auto hash =
		SHA1::fromString( std::string( clientKey ) + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11" );
	std::string result;
	Base64::encode(
		std::string_view( reinterpret_cast<const char*>( hash.digest.data() ), hash.digest.size() ),
		result );
	return result;
}

std::string WebSocket::serverResponse( std::string_view request ) {
	const auto lineEnd = request.find( "\r\n" );
	const auto first = request.substr( 0, lineEnd );
	std::string decoded;
	const auto key = headerValue( request, "sec-websocket-key" );
	if ( request.size() > 65536 || first.substr( 0, 4 ) != "GET " || first.size() < 14 ||
		 first.substr( first.size() - 9 ) != " HTTP/1.1" ||
		 headerValue( request, "host" ).empty() ||
		 headerValue( request, "sec-websocket-version" ) != "13" ||
		 Base64::decode( key, decoded, Base64::DecodeMode::NoWhitespaceStrict ) != 16 ||
		 !headerToken( request, "connection", "upgrade" ) ||
		 !headerToken( request, "upgrade", "websocket" ) )
		return {};
	return "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: "
		   "Upgrade\r\nSec-WebSocket-Accept: " +
		   acceptKey( key ) + "\r\n\r\n";
}

WebSocket::WebSocket( std::unique_ptr<TcpSocket> transport, Role role, size_t maxMessageSize ) :
	mDecoder(
		role == Role::Server ? Mask::Required : Mask::Forbidden,
		[this]( Opcode opcode, std::string_view data ) {
			if ( opcode == Opcode::Ping ) {
				// Pings are answered until the peer's Close, even after ours (RFC 6455 5.5.2).
				if ( !mPeerClosed )
					mControlFailed = send( data, Opcode::Pong ) != Socket::Done;
			} else if ( opcode == Opcode::Close ) {
				if ( !mCloseSent )
					mControlFailed = send( data, Opcode::Close ) != Socket::Done;
				mPeerClosed = true;
			} else if ( ( opcode == Opcode::Text || opcode == Opcode::Binary ) && !mCloseSent ) {
				mReceived.assign( data );
				mReceivedOpcode = opcode;
				mHasMessage = true;
			}
		},
		maxMessageSize ),
	mTransport( std::move( transport ) ),
	mRole( role ) {}

Socket::Status WebSocket::readHeader( std::string& header ) {
	while ( true ) {
		const auto end = mInput.find( "\r\n\r\n" );
		if ( end != std::string::npos ) {
			if ( end + 4 > 65536 )
				return Socket::Error;
			header = mInput.substr( 0, end + 4 );
			mInput.erase( 0, end + 4 );
			return Socket::Done;
		}
		if ( mInput.size() > 65536 )
			return Socket::Error;
		std::array<char, 4096> buffer;
		size_t received = 0;
		const auto status = mTransport->receive( buffer.data(), buffer.size(), received );
		if ( status != Socket::Done )
			return status;
		mInput.append( buffer.data(), received );
	}
}

Socket::Status WebSocket::open( const std::string& host, const std::string& target,
								const std::string& extraHeaders ) {
	if ( !mTransport || mRole != Role::Client || mOpen || !mTransport->isBlocking() ||
		 !validTarget( host ) || !validTarget( target ) || target.front() != '/' ||
		 extraHeaders.size() > 32768 )
		return Socket::Error;
	// Each caller field must be a single HTTP line, never a second request or an override of
	// required framing/negotiation fields. Protocols/extensions need an explicit future API.
	if ( !extraHeaders.empty() && !String::endsWith( extraHeaders, "\r\n" ) )
		return Socket::Error;
	std::string_view fields = extraHeaders;
	while ( !fields.empty() ) {
		const auto end = fields.find( "\r\n" );
		if ( end == std::string_view::npos )
			return Socket::Error;
		const auto line = fields.substr( 0, end );
		const auto colon = line.find( ':' );
		const auto name = line.substr( 0, colon );
		if ( colon == std::string::npos || name.empty() ||
			 name.find_first_of( " \t\r\n" ) != std::string::npos ||
			 String::iequals( name, "host" ) || String::iequals( name, "connection" ) ||
			 String::iequals( name, "upgrade" ) || String::iequals( name, "content-length" ) ||
			 String::iequals( name, "transfer-encoding" ) ||
			 String::iequals( name.substr( 0, 14 ), "sec-websocket-" ) ||
			 line.find_first_of( "\r\n\0", 0, 3 ) != std::string::npos )
			return Socket::Error;
		fields.remove_prefix( end + 2 );
	}
	std::array<char, 16> random;
	if ( !Sys::getRandomBytes( random.data(), random.size() ) )
		return Socket::Error;
	std::string key;
	Base64::encode( std::string_view( random.data(), random.size() ), key );
	const auto request = "GET " + target + " HTTP/1.1\r\nHost: " + host +
						 "\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Version: "
						 "13\r\nSec-WebSocket-Key: " +
						 key + "\r\n" + extraHeaders + "\r\n";
	if ( mTransport->send( request.data(), request.size() ) != Socket::Done )
		return Socket::Error;
	std::string header;
	const auto status = readHeader( header );
	if ( status != Socket::Done )
		return status;
	const auto statusLine = header.substr( 0, header.find( "\r\n" ) );
	if ( !( statusLine == "HTTP/1.1 101" || String::startsWith( statusLine, "HTTP/1.1 101 " ) ) ||
		 !headerToken( header, "connection", "upgrade" ) ||
		 !headerToken( header, "upgrade", "websocket" ) ||
		 headerValue( header, "sec-websocket-accept" ) != acceptKey( key ) ||
		 !headerValue( header, "sec-websocket-extensions" ).empty() ||
		 !headerValue( header, "sec-websocket-protocol" ).empty() )
		return Socket::Error;
	mOpen = true;
	return Socket::Done;
}

Socket::Status WebSocket::accept() {
	if ( !mTransport || mRole != Role::Server || mOpen || !mTransport->isBlocking() )
		return Socket::Error;
	std::string header;
	const auto status = readHeader( header );
	if ( status != Socket::Done )
		return status;
	const auto response = serverResponse( header );
	if ( response.empty() || mTransport->send( response.data(), response.size() ) != Socket::Done )
		return Socket::Error;
	mOpen = true;
	return Socket::Done;
}

Socket::Status WebSocket::send( std::string_view payload, Opcode opcode ) {
	// After our Close only a Pong may follow it (RFC 6455 5.5.1).
	if ( !mTransport || !mOpen || mFailed || ( mCloseSent && opcode != Opcode::Pong ) ||
		 !mTransport->isBlocking() )
		return Socket::Error;
	const auto frame = encode( payload, opcode, mRole );
	if ( frame.empty() )
		return Socket::Error;
	const auto status = mTransport->send( frame.data(), frame.size() );
	if ( status != Socket::Done ) {
		// A partially emitted frame cannot be retried as a new frame on this stream.
		mFailed = true;
		mTransport->disconnect();
	} else if ( opcode == Opcode::Close ) {
		// However the Close was sent, nothing but a Pong may follow it.
		mCloseSent = true;
	}
	return status;
}

Socket::Status WebSocket::receive( std::string& payload, Opcode& opcode ) {
	if ( !mTransport || !mOpen || mFailed || !mTransport->isBlocking() )
		return Socket::Error;

	// After a local close, data is discarded until the peer's Close completes the handshake.
	while ( !mHasMessage && !mPeerClosed && !mControlFailed ) {
		if ( mInput.empty() ) {
			std::array<char, 4096> buffer;
			size_t count = 0;
			const auto status = mTransport->receive( buffer.data(), buffer.size(), count );
			if ( status != Socket::Done )
				return status;
			mInput.assign( buffer.data(), count );
		}
		size_t consumed = 0;
		if ( !mDecoder.feed( mInput, consumed, true ) ) {
			// The decoder cannot resynchronize after a protocol violation: the stream is done.
			mFailed = true;
			mTransport->disconnect();
			return Socket::Error;
		}
		mInput.erase( 0, consumed );
	}
	if ( mControlFailed )
		return Socket::Error;
	if ( mHasMessage ) {
		payload = std::move( mReceived );
		opcode = mReceivedOpcode;
		mHasMessage = false;
		return Socket::Done;
	}
	// Both Close frames are exchanged: close TCP now rather than leaving the peer waiting
	// (RFC 6455 5.5.1; a client may close at any time after the exchange).
	if ( mCloseSent && mPeerClosed )
		mTransport->disconnect();
	return Socket::Disconnected;
}

Socket::Status WebSocket::close( Uint16 code, std::string_view reason ) {
	std::string payload;
	payload += static_cast<char>( code >> 8 );
	payload += static_cast<char>( code & 255 );
	payload.append( reason );
	return send( payload, Opcode::Close );
}

} // namespace EE::Network
