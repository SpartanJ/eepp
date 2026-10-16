#include "sshagent.hpp"

#if EE_PLATFORM != EE_PLATFORM_WIN
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#endif

namespace EE { namespace VFS { namespace sftp {

namespace {

// draft-miller-ssh-agent message numbers and flags.
constexpr Uint8 AgentFailure = 5;
constexpr Uint8 RequestIdentities = 11;
constexpr Uint8 IdentitiesAnswer = 12;
constexpr Uint8 SignRequest = 13;
constexpr Uint8 SignResponse = 14;
constexpr Uint32 RSASHA256 = 2;
constexpr Uint32 RSASHA512 = 4;

/** Agent replies larger than this are refused (OpenSSH uses 256 KiB). */
constexpr Uint32 MaxMessage = 256 * 1024;

void putU32( std::string& out, Uint32 value ) {
	out += static_cast<char>( ( value >> 24 ) & 0xFF );
	out += static_cast<char>( ( value >> 16 ) & 0xFF );
	out += static_cast<char>( ( value >> 8 ) & 0xFF );
	out += static_cast<char>( value & 0xFF );
}

void putString( std::string& out, std::string_view value ) {
	putU32( out, static_cast<Uint32>( value.size() ) );
	out.append( value );
}

/** Sequential reader of SSH wire data; any overrun makes it invalid. */
class WireReader {
  public:
	explicit WireReader( std::string_view data ) : mData( data ) {}

	bool valid() const { return mValid; }

	Uint8 u8() {
		if ( !need( 1 ) )
			return 0;
		return static_cast<Uint8>( mData[mPosition++] );
	}

	Uint32 u32() {
		if ( !need( 4 ) )
			return 0;
		Uint32 value = 0;
		for ( int i = 0; i < 4; ++i )
			value = ( value << 8 ) | static_cast<Uint8>( mData[mPosition++] );
		return value;
	}

	std::string_view string() {
		Uint32 length = u32();
		if ( !need( length ) )
			return {};
		std::string_view value = mData.substr( mPosition, length );
		mPosition += length;
		return value;
	}

  private:
	bool need( size_t bytes ) {
		if ( !mValid || mData.size() - mPosition < bytes )
			mValid = false;
		return mValid;
	}

	std::string_view mData;
	size_t mPosition{ 0 };
	bool mValid{ true };
};

VFSError agentError( std::string message ) {
	return VFSError( VFSErrorCode::AuthenticationFailed, URI(), "ssh-agent: " + message );
}

} // namespace

bool SSHAgent::supported() {
#if EE_PLATFORM != EE_PLATFORM_WIN
	return true;
#else
	return false;
#endif
}

SSHAgent::SSHAgent( std::string socketPath ) : mPath( std::move( socketPath ) ) {}

SSHAgent::~SSHAgent() {
	disconnect();
}

void SSHAgent::disconnect() {
	std::lock_guard<std::mutex> lock( mSocketMutex );
#if EE_PLATFORM != EE_PLATFORM_WIN
	if ( mSocket >= 0 )
		::close( mSocket );
#endif
	mSocket = -1;
}

void SSHAgent::cancel() {
	std::lock_guard<std::mutex> lock( mSocketMutex );
	mCancelled.store( true, std::memory_order_release );
#if EE_PLATFORM != EE_PLATFORM_WIN
	// Do not close here: the worker may still use this descriptor, and closing could let a
	// different thread reuse its number. shutdown wakes its blocking receive; it closes it.
	if ( mSocket >= 0 )
		::shutdown( mSocket, SHUT_RDWR );
#endif
}

VFSResult<std::string> SSHAgent::transact( std::string_view request ) {
#if EE_PLATFORM != EE_PLATFORM_WIN
	if ( mCancelled.load( std::memory_order_acquire ) )
		return Err( agentError( "request cancelled" ) );
	if ( mSocket < 0 ) {
		sockaddr_un address{};
		if ( mPath.empty() || mPath.size() >= sizeof( address.sun_path ) )
			return Err( agentError( "invalid socket path \"" + mPath + "\"" ) );
		address.sun_family = AF_UNIX;
		std::memcpy( address.sun_path, mPath.c_str(), mPath.size() + 1 );
		{
			std::lock_guard<std::mutex> lock( mSocketMutex );
			if ( mCancelled.load( std::memory_order_acquire ) )
				return Err( agentError( "request cancelled" ) );
			mSocket = ::socket( AF_UNIX, SOCK_STREAM, 0 );
		}
#ifdef SO_NOSIGPIPE
		// No MSG_NOSIGNAL on Apple platforms: an agent that went away must not kill the process.
		int noSigPipe = 1;
		if ( mSocket >= 0 )
			::setsockopt( mSocket, SOL_SOCKET, SO_NOSIGPIPE, &noSigPipe, sizeof( noSigPipe ) );
#endif
		if ( mSocket < 0 || ::connect( mSocket, reinterpret_cast<sockaddr*>( &address ),
									   sizeof( address ) ) != 0 ) {
			std::string error( std::strerror( errno ) );
			disconnect();
			return Err( agentError( "cannot connect to " + mPath + ": " + error ) );
		}
	}
	if ( mCancelled.load( std::memory_order_acquire ) ) {
		disconnect();
		return Err( agentError( "request cancelled" ) );
	}
	std::string message;
	message.reserve( 4 + request.size() );
	putString( message, request );
	size_t sent = 0;
	while ( sent < message.size() ) {
#ifdef MSG_NOSIGNAL
		constexpr int flags = MSG_NOSIGNAL;
#else
		constexpr int flags = 0;
#endif
		ssize_t rc = ::send( mSocket, message.data() + sent, message.size() - sent, flags );
		if ( rc < 0 && errno == EINTR )
			continue;
		if ( rc <= 0 ) {
			disconnect();
			return Err( agentError( "write failed" ) );
		}
		sent += static_cast<size_t>( rc );
	}
	auto receive = [this]( char* buffer, size_t size ) {
		size_t received = 0;
		while ( received < size ) {
			ssize_t rc = ::recv( mSocket, buffer + received, size - received, 0 );
			if ( rc < 0 && errno == EINTR )
				continue;
			if ( rc <= 0 )
				return false;
			received += static_cast<size_t>( rc );
		}
		return true;
	};
	char header[4];
	if ( !receive( header, sizeof( header ) ) ) {
		disconnect();
		return Err( agentError( "read failed" ) );
	}
	Uint32 length = WireReader( std::string_view( header, sizeof( header ) ) ).u32();
	if ( length == 0 || length > MaxMessage ) {
		disconnect();
		return Err( agentError( "invalid reply length" ) );
	}
	std::string reply( length, '\0' );
	if ( !receive( reply.data(), reply.size() ) ) {
		disconnect();
		return Err( agentError( "read failed" ) );
	}
	return Ok( std::move( reply ) );
#else
	(void)request;
	return Err( agentError( "not supported on this platform" ) );
#endif
}

VFSResult<std::vector<SSHAgent::Identity>> SSHAgent::listIdentities() {
	std::string request( 1, static_cast<char>( RequestIdentities ) );
	auto reply = transact( request );
	if ( !reply )
		return Err( std::move( reply ).error() );
	WireReader reader( reply.value() );
	if ( reader.u8() != IdentitiesAnswer )
		return Err( agentError( "the agent refused to list its keys" ) );
	Uint32 count = reader.u32();
	std::vector<Identity> identities;
	for ( Uint32 i = 0; i < count && reader.valid(); ++i ) {
		std::string_view blob = reader.string();
		std::string_view comment = reader.string();
		if ( reader.valid() )
			identities.push_back( { std::string( blob ), std::string( comment ) } );
	}
	if ( !reader.valid() )
		return Err( agentError( "malformed key list" ) );
	return Ok( std::move( identities ) );
}

VFSResult<std::string> SSHAgent::sign( std::string_view blob, std::string_view data,
									   std::string_view algorithm ) {
	Uint32 flags = algorithm == "rsa-sha2-512"	 ? RSASHA512
				   : algorithm == "rsa-sha2-256" ? RSASHA256
												 : 0;
	std::string request( 1, static_cast<char>( SignRequest ) );
	putString( request, blob );
	putString( request, data );
	putU32( request, flags );
	auto reply = transact( request );
	if ( !reply )
		return Err( std::move( reply ).error() );
	WireReader reader( reply.value() );
	Uint8 type = reader.u8();
	if ( type == AgentFailure )
		return Err( agentError( "signing was refused" ) );
	if ( type != SignResponse )
		return Err( agentError( "unexpected reply to a signing request" ) );
	WireReader signature( reader.string() );
	std::string_view signedWith = signature.string();
	std::string_view raw = signature.string();
	if ( !reader.valid() || !signature.valid() )
		return Err( agentError( "malformed signature" ) );
	// The server verifies the algorithm announced in the request.
	if ( !algorithm.empty() && signedWith != algorithm ) {
		return Err( agentError( "signed with " + std::string( signedWith ) + " instead of " +
								std::string( algorithm ) ) );
	}
	return Ok( std::string( raw ) );
}

std::string_view SSHAgent::algorithmOfSignedRequest( std::string_view data ) {
	// string session identifier, byte SSH_MSG_USERAUTH_REQUEST, string user, string service,
	// string "publickey", boolean TRUE, string algorithm, string public key.
	WireReader reader( data );
	reader.string();
	reader.u8();
	reader.string();
	reader.string();
	std::string_view method = reader.string();
	reader.u8();
	std::string_view algorithm = reader.string();
	if ( !reader.valid() || method != "publickey" )
		return {};
	return algorithm;
}

}}} // namespace EE::VFS::sftp
