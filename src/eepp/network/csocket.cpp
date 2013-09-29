#include <eepp/network/csocket.hpp>
#include <eepp/network/platform/platformimpl.hpp>

namespace EE { namespace Network {

cSocket::cSocket(Type type) :
	mType(type),
	mSocket	(Private::cSocketImpl::InvalidSocket()),
	mIsBlocking(true)
{
}

cSocket::~cSocket() {
	// Close the socket before it gets destructed
	Close();
}


void cSocket::SetBlocking(bool blocking) {
	// Apply if the socket is already created
	if (mSocket != Private::cSocketImpl::InvalidSocket())
		Private::cSocketImpl::SetBlocking(mSocket, blocking);

	mIsBlocking = blocking;
}

bool cSocket::IsBlocking() const {
	return mIsBlocking;
}

SocketHandle cSocket::GetHandle() const {
	return mSocket;
}

void cSocket::Create() {
	// Don't create the socket if it already exists
	if (mSocket == Private::cSocketImpl::InvalidSocket()) {
		SocketHandle handle = socket(PF_INET, mType == Tcp ? SOCK_STREAM : SOCK_DGRAM, 0);
		Create(handle);
	}
}

void cSocket::Create(SocketHandle handle) {
	// Don't create the socket if it already exists
	if (mSocket == Private::cSocketImpl::InvalidSocket()) {
		// Assign the new handle
		mSocket = handle;

		// Set the current blocking state
		SetBlocking(mIsBlocking);

		if (mType == Tcp) {
			// Disable the Nagle algorithm (ie. removes buffering of TCP packets)
			int yes = 1;

			if (setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char*>(&yes), sizeof(yes)) == -1) {
				eePRINTL( "Failed to set socket option \"TCP_NODELAY\" ; all your TCP packets will be buffered" );
			}

			// On Mac OS X, disable the SIGPIPE signal on disconnection
			#if EE_PLATFORM == EE_PLATFORM_MACOSX
				if (setsockopt(mSocket, SOL_SOCKET, SO_NOSIGPIPE, reinterpret_cast<char*>(&yes), sizeof(yes)) == -1) {
					eePRINTL( "Failed to set socket option \"SO_NOSIGPIPE\"" );
				}
			#endif
		} else {
			// Enable broadcast by default for UDP sockets
			int yes = 1;

			if (setsockopt(mSocket, SOL_SOCKET, SO_BROADCAST, reinterpret_cast<char*>(&yes), sizeof(yes)) == -1) {
				eePRINTL( "Failed to enable broadcast on UDP socket" );
			}
		}
	}
}

void cSocket::Close() {
	// Close the socket
	if (mSocket != Private::cSocketImpl::InvalidSocket()) {
		Private::cSocketImpl::Close(mSocket);
		mSocket = Private::cSocketImpl::InvalidSocket();
	}
}

}}
