#include <cerrno>
#include <cstring>
#include <eepp/network/platform/platformimpl.hpp>
#include <eepp/network/socket.hpp>
#include <eepp/network/socketselector.hpp>
#include <eepp/system/log.hpp>
#include <limits>
#if EE_PLATFORM == EE_PLATFORM_WIN
#include <afunix.h>
#else
#include <sys/un.h>
#endif
#if EE_PLATFORM != EE_PLATFORM_WIN
#include <fcntl.h>
#endif

namespace EE { namespace Network {

Socket::Socket( Type type ) :
	mType( type ), mSocket( Private::SocketImpl::invalidSocket() ), mIsBlocking( true ) {}

Socket::~Socket() {
	// Close the socket before it gets destructed
	close();
}

void Socket::setBlocking( bool blocking ) {
	// Apply if the socket is already created
	if ( mSocket != Private::SocketImpl::invalidSocket() )
		Private::SocketImpl::setBlocking( mSocket, blocking );

	mIsBlocking = blocking;
}

bool Socket::isBlocking() const {
	return mIsBlocking;
}

SocketHandle Socket::getHandle() const {
	return mSocket;
}

bool Socket::create() {
	// Don't create the socket if it already exists
	if ( mSocket == Private::SocketImpl::invalidSocket() ) {
		int type = mType == Udp ? SOCK_DGRAM : SOCK_STREAM;
#if EE_PLATFORM != EE_PLATFORM_WIN
#if EE_SOCKET_CLOEXEC
		// Atomic: a concurrent fork/exec cannot inherit the descriptor before it is marked.
		type |= SOCK_CLOEXEC;
#endif
#endif
		SocketHandle handle = socket( mType == Local ? AF_UNIX : PF_INET, type, 0 );
		if ( handle == Private::SocketImpl::invalidSocket() ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
			Log::error( "Failed to create %s socket (Winsock error %d)",
						mType == Tcp   ? "TCP"
						: mType == Udp ? "UDP"
									   : "local",
						WSAGetLastError() );
#else
			const int errorCode = errno;
			Log::error( "Failed to create %s socket: %s (errno %d)",
						mType == Tcp   ? "TCP"
						: mType == Udp ? "UDP"
									   : "local",
						std::strerror( errorCode ), errorCode );
#endif
			return false;
		}
		create( handle );
	}
	return true;
}

void Socket::create( SocketHandle handle ) {
	// Don't create the socket if it already exists
	if ( mSocket == Private::SocketImpl::invalidSocket() ) {
		// Assign the new handle
		mSocket = handle;
#if EE_PLATFORM == EE_PLATFORM_WIN
		SetHandleInformation( reinterpret_cast<HANDLE>( mSocket ), HANDLE_FLAG_INHERIT, 0 );
#else
		fcntl( mSocket, F_SETFD, FD_CLOEXEC );
#endif

		// Set the current blocking state
		setBlocking( mIsBlocking );

		if ( mType != Udp ) {
			// Disable the Nagle algorithm (ie. removes buffering of TCP packets)
			int yes = 1;

			if ( mType == Tcp &&
				 setsockopt( mSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>( &yes ),
							 sizeof( yes ) ) == -1 ) {
				Log::error(
					"Failed to set socket option \"TCP_NODELAY\" ; all your TCP packets will "
					"be buffered" );
			}

// On Mac OS X, disable the SIGPIPE signal on disconnection
#if defined( SO_NOSIGPIPE )
			if ( setsockopt( mSocket, SOL_SOCKET, SO_NOSIGPIPE, reinterpret_cast<char*>( &yes ),
							 sizeof( yes ) ) == -1 ) {
				Log::error( "Failed to set socket option \"SO_NOSIGPIPE\"" );
			}
#endif
		} else {
			// Enable broadcast by default for UDP sockets
			int yes = 1;

			if ( setsockopt( mSocket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>( &yes ),
							 sizeof( yes ) ) == -1 ) {
				Log::error( "Failed to enable broadcast on UDP socket" );
			}
		}
	}
}

void Socket::close() {
	// Close the socket
	if ( mSocket != Private::SocketImpl::invalidSocket() ) {
		Private::SocketImpl::close( mSocket );
		mSocket = Private::SocketImpl::invalidSocket();
	}
}

namespace {

#if defined( MSG_NOSIGNAL )
constexpr int flags = MSG_NOSIGNAL;
#else
constexpr int flags = 0;
#endif

} // namespace

bool Socket::makeLocalAddress( const std::string& path, void* storage, size_t size ) {
#if EE_PLATFORM == EE_PLATFORM_EMSCRIPTEN
	(void)path;
	(void)storage;
	(void)size;
	return false;
#else
	if ( size != sizeof( sockaddr_un ) )
		return false;
	auto& address = *static_cast<sockaddr_un*>( storage );
	if ( path.empty() || path.size() >= sizeof( address.sun_path ) ||
		 path.find( '\0' ) != std::string::npos )
		return false;
	address.sun_family = AF_UNIX;
	std::memcpy( address.sun_path, path.c_str(), path.size() + 1 );
	return true;
#endif
}

Socket::Status Socket::connectAddress( const void* address, size_t size, Time timeout ) {
	const bool blocking = isBlocking();
	if ( blocking && timeout > Time::Zero )
		setBlocking( false );
	const int result = ::connect( getHandle(), reinterpret_cast<const sockaddr*>( address ),
								  static_cast<Private::SocketImpl::AddrLength>( size ) );
	Status status = result == 0 ? Done : Private::SocketImpl::getErrorStatus();
#if EE_PLATFORM != EE_PLATFORM_WIN
	// An interrupted connect continues asynchronously and must not be retried: a second call
	// reports EALREADY or EISCONN. Wait for it as for a nonblocking connect.
	const bool interrupted = result != 0 && errno == EINTR;
	if ( interrupted ) {
		status = NotReady;
	} else if ( mType == Local && status == NotReady && errno != EINPROGRESS ) {
		// AF_UNIX EAGAIN means its listen queue is full: no connection was started. Polling
		// writability/SO_ERROR here would incorrectly report success on an unconnected socket.
		status = Error;
	}
#else
	const bool interrupted = false;
#endif
	if ( status == NotReady && ( interrupted || ( blocking && timeout > Time::Zero ) ) ) {
		SocketSelector selector;
		selector.add( *this, SocketSelector::Event::Write );
		// Interruption only happens on blocking sockets without a timeout: wait without one.
		bool ready;
		do {
			ready = selector.wait( interrupted ? Time::Zero : timeout );
		} while ( !ready && interrupted && errno == EINTR );
		if ( ready )
			status = connectionStatus();
		else if ( interrupted )
			status = Error;
	}
#if EE_PLATFORM == EE_PLATFORM_WIN
	const auto error = WSAGetLastError();
#else
	const int error = errno;
#endif
	if ( blocking )
		setBlocking( true );
#if EE_PLATFORM == EE_PLATFORM_WIN
	WSASetLastError( error );
#else
	errno = error;
#endif
	return status;
}

Socket::Status Socket::connectionStatus() const {
	int error = 0;
	Private::SocketImpl::AddrLength length = sizeof( error );
	if ( getsockopt( getHandle(), SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>( &error ),
					 &length ) != 0 )
		return Private::SocketImpl::getErrorStatus();
	if ( error ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
		WSASetLastError( error );
#else
		errno = error;
#endif
		return Private::SocketImpl::getErrorStatus();
	}
	// SO_ERROR == 0 also describes a connection which has not completed yet.
	sockaddr_storage peer{};
	length = sizeof( peer );
	if ( getpeername( getHandle(), reinterpret_cast<sockaddr*>( &peer ), &length ) == 0 )
		return Done;
#if EE_PLATFORM == EE_PLATFORM_WIN
	if ( WSAGetLastError() == WSAENOTCONN )
#else
	if ( errno == ENOTCONN )
#endif
		return NotReady;
	return Private::SocketImpl::getErrorStatus();
}

Socket::Status Socket::sendStream( const void* data, std::size_t size, std::size_t& sent ) {
	sent = 0;
	// Check the parameters
	if ( !data || ( size == 0 ) ) {
		Log::error( "Cannot send data over the network (no data to send)" );
		return Error;
	}

	// Loop until every byte has been sent
	int result = 0;
	for ( sent = 0; sent < size; sent += result ) {
		// Send a chunk of data
		result = ::send(
			getHandle(), static_cast<const char*>( data ) + sent,
			static_cast<int>( eemin<size_t>( size - sent, std::numeric_limits<int>::max() ) ),
			flags );

		// Check for errors
		if ( result == 0 )
			return Disconnected;
		if ( result < 0 ) {
#if EE_PLATFORM != EE_PLATFORM_WIN
			if ( errno == EINTR ) {
				result = 0;
				continue;
			}
#endif
			Status status = Private::SocketImpl::getErrorStatus();

			if ( ( status == NotReady ) && sent ) {
				return Partial;
			}

			return status;
		}
	}

	return Done;
}

Socket::Status Socket::receiveStream( void* data, std::size_t size, std::size_t& received ) {
	// First clear the variables to fill
	received = 0;

	// Check the destination buffer
	if ( !data ) {
		Log::error( "Cannot receive data from the network (the destination buffer is invalid)" );
		return Error;
	}

	// Receive a chunk of bytes
	int sizeReceived;
	do {
		sizeReceived =
			recv( getHandle(), static_cast<char*>( data ),
				  static_cast<int>( eemin<size_t>( size, std::numeric_limits<int>::max() ) ), 0 );
#if EE_PLATFORM != EE_PLATFORM_WIN
	} while ( sizeReceived < 0 && errno == EINTR );
#else
	} while ( sizeReceived < 0 && WSAGetLastError() == WSAEINTR );
#endif

	// Check the number of bytes received
	if ( sizeReceived > 0 ) {
		received = static_cast<std::size_t>( sizeReceived );
		return Done;
	} else if ( sizeReceived == 0 ) {
		return Socket::Disconnected;
	} else {
		return Private::SocketImpl::getErrorStatus();
	}
}

}} // namespace EE::Network
