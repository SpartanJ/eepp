#include "sshsocket.hpp"
#include <chrono>
#include <cstring>
#include <eepp/core/string.hpp>
#include <mutex>

#if EE_PLATFORM != EE_PLATFORM_WIN
#include <cerrno>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#endif

namespace EE { namespace VFS { namespace sftp {

void SSHSocket::init() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	static std::once_flag once;
	std::call_once( once, [] {
		WSADATA data;
		WSAStartup( MAKEWORD( 2, 2 ), &data );
	} );
#endif
}

void SSHSocket::close( SocketHandle socket ) {
	if ( socket == InvalidSocket )
		return;
#if EE_PLATFORM == EE_PLATFORM_WIN
	closesocket( socket );
#else
	::close( socket );
#endif
}

bool SSHSocket::setNonBlocking( SocketHandle socket ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	u_long mode = 1;
	return ioctlsocket( socket, FIONBIO, &mode ) == 0;
#else
	int flags = fcntl( socket, F_GETFL, 0 );
	return flags >= 0 && fcntl( socket, F_SETFL, flags | O_NONBLOCK ) == 0;
#endif
}

int SSHSocket::poll( PollFd* fds, size_t count, int timeoutMs ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return WSAPoll( fds, static_cast<ULONG>( count ), timeoutMs );
#else
	int result;
	do {
		result = ::poll( fds, static_cast<nfds_t>( count ), timeoutMs );
	} while ( result < 0 && errno == EINTR );
	return result;
#endif
}

int SSHSocket::pendingError( SocketHandle socket ) {
	int error = 0;
#if EE_PLATFORM == EE_PLATFORM_WIN
	int len = sizeof( error );
	getsockopt( socket, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>( &error ), &len );
#else
	socklen_t len = sizeof( error );
	getsockopt( socket, SOL_SOCKET, SO_ERROR, &error, &len );
#endif
	return error;
}

std::string SSHSocket::errorString( int error ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	char buffer[256] = { 0 };
	FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
					static_cast<DWORD>( error ), 0, buffer, sizeof( buffer ), nullptr );
	return buffer;
#else
	return std::strerror( error );
#endif
}

int SSHSocket::lastError() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return WSAGetLastError();
#else
	return errno;
#endif
}

bool SSHSocket::connectInProgress( int error ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS;
#else
	return error == EINPROGRESS || error == EWOULDBLOCK;
#endif
}

SocketHandle SSHSocket::connect( const std::string& host, Uint16 port, int timeoutMs,
								 std::string& error ) {
	init();
	struct addrinfo hints = {};
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	struct addrinfo* addresses = nullptr;
	std::string service = String::toString( port );
	int gai = getaddrinfo( host.c_str(), service.c_str(), &hints, &addresses );
	if ( gai != 0 || !addresses ) {
		error = "Could not resolve hostname " + host + ": " + gai_strerror( gai );
		return InvalidSocket;
	}
	auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds( timeoutMs );
	SocketHandle result = InvalidSocket;
	for ( auto* ai = addresses; ai && result == InvalidSocket; ai = ai->ai_next ) {
		SocketHandle sock = ::socket( ai->ai_family, ai->ai_socktype, ai->ai_protocol );
		if ( sock == InvalidSocket )
			continue;
		setNonBlocking( sock );
		int nodelay = 1;
		setsockopt( sock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<const char*>( &nodelay ),
					sizeof( nodelay ) );
		if ( ::connect( sock, ai->ai_addr, static_cast<int>( ai->ai_addrlen ) ) == 0 ) {
			result = sock;
			break;
		}
		int err = lastError();
		if ( !connectInProgress( err ) ) {
			error = "connect to " + host + " port " + service + ": " + errorString( err );
			close( sock );
			continue;
		}
		int remaining = static_cast<int>( std::chrono::duration_cast<std::chrono::milliseconds>(
											  deadline - std::chrono::steady_clock::now() )
											  .count() );
		if ( remaining <= 0 ) {
			error = "connect to " + host + " port " + service + ": Connection timed out";
			close( sock );
			break;
		}
		PollFd pfd{};
		pfd.fd = sock;
		pfd.events = POLLOUT;
		int ready = poll( &pfd, 1, remaining );
		if ( ready <= 0 ) {
			error = "connect to " + host + " port " + service + ": Connection timed out";
			close( sock );
			continue;
		}
		err = pendingError( sock );
		if ( err != 0 ) {
			error = "connect to " + host + " port " + service + ": " + errorString( err );
			close( sock );
			continue;
		}
		result = sock;
	}
	freeaddrinfo( addresses );
	return result;
}

}}} // namespace EE::VFS::sftp
