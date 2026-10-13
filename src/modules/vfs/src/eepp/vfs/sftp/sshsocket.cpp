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

void socketsInit() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	static std::once_flag once;
	std::call_once( once, [] {
		WSADATA data;
		WSAStartup( MAKEWORD( 2, 2 ), &data );
	} );
#endif
}

void closeSocket( SocketHandle socket ) {
	if ( socket == InvalidSocket )
		return;
#if EE_PLATFORM == EE_PLATFORM_WIN
	closesocket( socket );
#else
	::close( socket );
#endif
}

bool setNonBlocking( SocketHandle socket ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	u_long mode = 1;
	return ioctlsocket( socket, FIONBIO, &mode ) == 0;
#else
	int flags = fcntl( socket, F_GETFL, 0 );
	return flags >= 0 && fcntl( socket, F_SETFL, flags | O_NONBLOCK ) == 0;
#endif
}

int pollSockets( PollFd* fds, size_t count, int timeoutMs ) {
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

int socketError( SocketHandle socket ) {
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

std::string socketErrorString( int error ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	char buffer[256] = { 0 };
	FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
					static_cast<DWORD>( error ), 0, buffer, sizeof( buffer ), nullptr );
	return buffer;
#else
	return std::strerror( error );
#endif
}

static int lastSocketError() {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return WSAGetLastError();
#else
	return errno;
#endif
}

static bool connectInProgress( int error ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
	return error == WSAEWOULDBLOCK || error == WSAEINPROGRESS;
#else
	return error == EINPROGRESS || error == EWOULDBLOCK;
#endif
}

SocketHandle connectTcp( const std::string& host, Uint16 port, int timeoutMs, std::string& error ) {
	socketsInit();
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
		int err = lastSocketError();
		if ( !connectInProgress( err ) ) {
			error = "connect to " + host + " port " + service + ": " + socketErrorString( err );
			closeSocket( sock );
			continue;
		}
		int remaining = static_cast<int>( std::chrono::duration_cast<std::chrono::milliseconds>(
											  deadline - std::chrono::steady_clock::now() )
											  .count() );
		if ( remaining <= 0 ) {
			error = "connect to " + host + " port " + service + ": Connection timed out";
			closeSocket( sock );
			break;
		}
		PollFd pfd{};
		pfd.fd = sock;
		pfd.events = POLLOUT;
		int ready = pollSockets( &pfd, 1, remaining );
		if ( ready <= 0 ) {
			error = "connect to " + host + " port " + service + ": Connection timed out";
			closeSocket( sock );
			continue;
		}
		err = socketError( sock );
		if ( err != 0 ) {
			error = "connect to " + host + " port " + service + ": " + socketErrorString( err );
			closeSocket( sock );
			continue;
		}
		result = sock;
	}
	freeaddrinfo( addresses );
	return result;
}

WakeSignal::WakeSignal() {
	socketsInit();
#if EE_PLATFORM == EE_PLATFORM_WIN
	SocketHandle listener = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if ( listener == InvalidSocket )
		return;
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl( INADDR_LOOPBACK );
	int len = sizeof( address );
	if ( ::bind( listener, reinterpret_cast<sockaddr*>( &address ), len ) != 0 ||
		 ::listen( listener, 1 ) != 0 ||
		 getsockname( listener, reinterpret_cast<sockaddr*>( &address ), &len ) != 0 ) {
		closeSocket( listener );
		return;
	}
	mWrite = ::socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if ( mWrite == InvalidSocket ||
		 ::connect( mWrite, reinterpret_cast<sockaddr*>( &address ), len ) != 0 ) {
		closeSocket( listener );
		closeSocket( mWrite );
		mWrite = InvalidSocket;
		return;
	}
	mRead = ::accept( listener, nullptr, nullptr );
	closeSocket( listener );
	if ( mRead == InvalidSocket ) {
		closeSocket( mWrite );
		mWrite = InvalidSocket;
		return;
	}
	setNonBlocking( mRead );
	setNonBlocking( mWrite );
#else
	int fds[2];
	if ( ::pipe( fds ) != 0 )
		return;
	mRead = fds[0];
	mWrite = fds[1];
	setNonBlocking( mRead );
	setNonBlocking( mWrite );
	fcntl( mRead, F_SETFD, FD_CLOEXEC );
	fcntl( mWrite, F_SETFD, FD_CLOEXEC );
#endif
}

WakeSignal::~WakeSignal() {
	closeSocket( mRead );
	closeSocket( mWrite );
}

void WakeSignal::signal() {
	char byte = 1;
#if EE_PLATFORM == EE_PLATFORM_WIN
	::send( mWrite, &byte, 1, 0 );
#else
	// A full pipe already guarantees a pending wake-up, so EAGAIN is fine.
	[[maybe_unused]] ssize_t written = ::write( mWrite, &byte, 1 );
#endif
}

void WakeSignal::drain() {
	char buffer[256];
#if EE_PLATFORM == EE_PLATFORM_WIN
	while ( ::recv( mRead, buffer, sizeof( buffer ), 0 ) > 0 ) {
	}
#else
	while ( ::read( mRead, buffer, sizeof( buffer ) ) > 0 ) {
	}
#endif
}

}}} // namespace EE::VFS::sftp
