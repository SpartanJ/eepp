#include <eepp/network/platform/platformimpl.hpp>
#include <eepp/network/socket.hpp>
#include <eepp/system/log.hpp>

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

void Socket::create() {
	// Don't create the socket if it already exists
	if ( mSocket == Private::SocketImpl::invalidSocket() ) {
		SocketHandle handle = socket( PF_INET, mType == Tcp ? SOCK_STREAM : SOCK_DGRAM, 0 );
		create( handle );
	}
}

void Socket::create( SocketHandle handle ) {
	// Don't create the socket if it already exists
	if ( mSocket == Private::SocketImpl::invalidSocket() ) {
		// Assign the new handle
		mSocket = handle;

		// Set the current blocking state
		setBlocking( mIsBlocking );

		if ( mType == Tcp ) {
			// Disable the Nagle algorithm (ie. removes buffering of TCP packets)
			int yes = 1;

			if ( setsockopt( mSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>( &yes ),
							 sizeof( yes ) ) == -1 ) {
				Log::error(
					"Failed to set socket option \"TCP_NODELAY\" ; all your TCP packets will "
					"be buffered" );
			}

// On Mac OS X, disable the SIGPIPE signal on disconnection
#if EE_PLATFORM == EE_PLATFORM_MACOSX
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

}} // namespace EE::Network
