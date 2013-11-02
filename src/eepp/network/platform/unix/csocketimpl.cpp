#include <eepp/network/platform/unix/csocketimpl.hpp>

#if defined( EE_PLATFORM_POSIX )

#include <errno.h>
#include <fcntl.h>
#include <cstring>

namespace EE { namespace Network { namespace Private {

sockaddr_in cSocketImpl::CreateAddress(Uint32 address, unsigned short port) {
	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr = htonl(address);
	addr.sin_family		= AF_INET;
	addr.sin_port		= htons(port);

#if EE_PLATFORM == EE_PLATFORM_MACOSX || EE_PLATFORM == EE_PLATFORM_IOS
	addr.sin_len = sizeof(addr);
#endif

	return addr;
}

SocketHandle cSocketImpl::InvalidSocket() {
	return -1;
}

void cSocketImpl::Close(SocketHandle sock) {
	::close(sock);
}

void cSocketImpl::SetBlocking(SocketHandle sock, bool block) {
	int status = fcntl(sock, F_GETFL);
	if (block)
		fcntl(sock, F_SETFL, status & ~O_NONBLOCK);
	else
		fcntl(sock, F_SETFL, status | O_NONBLOCK);
}

cSocket::Status cSocketImpl::GetErrorStatus() {
	// The followings are sometimes equal to EWOULDBLOCK,
	// so we have to make a special case for them in order
	// to avoid having double values in the switch case
	if ((errno == EAGAIN) || (errno == EINPROGRESS))
		return cSocket::NotReady;

	switch (errno) {
		case EWOULDBLOCK:	return cSocket::NotReady;
		case ECONNABORTED:	return cSocket::Disconnected;
		case ECONNRESET:	return cSocket::Disconnected;
		case ETIMEDOUT:		return cSocket::Disconnected;
		case ENETRESET:		return cSocket::Disconnected;
		case ENOTCONN:		return cSocket::Disconnected;
		case EPIPE:			return cSocket::Disconnected;
		default:			return cSocket::Error;
	}
}

}}}

#endif
