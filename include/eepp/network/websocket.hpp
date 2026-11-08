#ifndef EE_NETWORK_WEBSOCKET_HPP
#define EE_NETWORK_WEBSOCKET_HPP

#include <array>
#include <eepp/core/noncopyable.hpp>
#include <eepp/network/tcpsocket.hpp>
#include <functional>
#include <memory>
#include <string_view>

namespace EE::Network {

/** RFC 6455 over an owned connected TcpSocket (including SSLSocket for wss).
 * Construction transfers exclusive ownership; destruction closes the transport. Handshakes and
 * send/receive require blocking mode; socket timeouts are controlled by the caller. No
 * extensions/subprotocols are negotiated. Decoder is independently usable for nonblocking stream
 * observers and protocol servers. */
class EE_API WebSocket : NonCopyable {
  public:
	enum class Role : Uint8 { Client, Server };

	enum class Opcode : Uint8 {
		Continuation = 0,
		Text = 1,
		Binary = 2,
		Close = 8,
		Ping = 9,
		Pong = 10
	};

	enum class Mask : Uint8 { Any, Required, Forbidden };

	using MessageFn = std::function<void( Opcode, std::string_view )>;
	using CaptureFn = std::function<bool()>;

	/** Incremental, bounded frame decoder. Callback views are borrowed until it returns.
	 * Control frames are delivered between fragments. capture can skip data-message buffering
	 * at a message boundary; control frames are always delivered. Skipped text is not validated.
	 * skipOversized is intended for passive observers: oversized messages are discarded while
	 * parsing continues; endpoints normally fail instead. Feed false means protocol failure. */
	class EE_API Decoder {
	  public:
		Decoder( Mask mask, MessageFn onMessage, size_t maxMessageSize = 8 * 1024 * 1024,
				 CaptureFn capture = {}, bool skipOversized = false );

		bool feed( std::string_view bytes );

		/** Optionally stop after one complete message/control frame, reporting consumed bytes. */
		bool feed( std::string_view bytes, size_t& consumed, bool stopAfterMessage );

		/** Report and clear an oversized-message notification from passive skip mode. The
		 * notification is raised once at the first oversized fragment, without buffering it. */
		bool takeOversizedMessage();

	  private:
		MessageFn mOnMessage;
		CaptureFn mCapture;
		std::string mMessage;
		std::string mControl;
		Uint64 mRemaining{ 0 };
		Uint64 mOffset{ 0 };
		size_t mLimit;
		size_t mUsed{ 0 };
		size_t mNeeded{ 2 };
		std::array<Uint8, 14> mHeader{};
		Opcode mOpcode{ Opcode::Continuation };
		Opcode mMessageOpcode{ Opcode::Continuation };
		Mask mMask;
		bool mFinal{ false };
		bool mDiscard{ false };
		bool mSkipOversized;
		bool mFailed{ false };
		bool mOversized{ false };
	};

	WebSocket( std::unique_ptr<TcpSocket> transport, Role role,
			   size_t maxMessageSize = 8 * 1024 * 1024 );

	/** Client/server opening handshakes. extraHeaders is raw HTTP fields ending in CRLF;
	 * reserved WebSocket/HTTP framing headers cannot be overridden. Server validates the
	 * request before responding. Applications needing origin/auth policy should read/validate
	 * the request themselves and use serverResponse(), rather than accept(). */
	Socket::Status open( const std::string& host, const std::string& target = "/",
						 const std::string& extraHeaders = "" );

	Socket::Status accept();

	Socket::Status send( std::string_view payload, Opcode opcode = Opcode::Text );

	/** Receive one application message, answering ping and close frames automatically. After
	 * close(), it discards data (still answering pings) until the peer's Close completes the
	 * handshake. Once both Close frames are exchanged the transport is disconnected and this
	 * reports Disconnected. */
	Socket::Status receive( std::string& payload, Opcode& opcode );

	/** Send a Close frame. Call receive() to wait for the peer's Close before disconnecting. */
	Socket::Status close( Uint16 code = 1000, std::string_view reason = {} );

	/** Encode one final frame, masking with fresh OS randomness for a client. Empty means
	 * invalid input or unavailable randomness. Control payloads must be <=125 bytes. */
	static std::string encode( std::string_view payload, Opcode opcode, Role sender );

	static std::string acceptKey( std::string_view clientKey );

	static std::string serverResponse( std::string_view request );

	static std::string headerValue( std::string_view header, std::string_view name );

  private:
	Decoder mDecoder;
	std::unique_ptr<TcpSocket> mTransport;
	std::string mInput;
	std::string mReceived;
	Opcode mReceivedOpcode{ Opcode::Continuation };
	Role mRole;
	bool mOpen{ false };
	// Our Close frame was sent: only Pong may follow it.
	bool mCloseSent{ false };
	// The peer's Close frame was received.
	bool mPeerClosed{ false };
	// A partial or failed send left the stream unusable.
	bool mFailed{ false };
	bool mHasMessage{ false };
	bool mControlFailed{ false };

	Socket::Status readHeader( std::string& header );
};

} // namespace EE::Network

#endif
