#include <eepp/network/platform/unix/socketimpl.hpp>

#if defined( EE_PLATFORM_POSIX )

#include <errno.h>
#include <fcntl.h>
#include <cstring>
#ifdef EE_PLATFORM_HAIKU
#include <posix/sys/time.h>
#endif

namespace EE { namespace Network { namespace Private {

sockaddr_in SocketImpl::createAddress(Uint32 address, unsigned short port) {
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

SocketHandle SocketImpl::invalidSocket() {
	return -1;
}

void SocketImpl::close(SocketHandle sock) {
	::close(sock);
}

void SocketImpl::setBlocking(SocketHandle sock, bool block) {
	int status = fcntl(sock, F_GETFL);
	if (block)
		fcntl(sock, F_SETFL, status & ~O_NONBLOCK);
	else
		fcntl(sock, F_SETFL, status | O_NONBLOCK);
}

Socket::Status SocketImpl::getErrorStatus() {
	// The followings are sometimes equal to EWOULDBLOCK,
	// so we have to make a special case for them in order
	// to avoid having double values in the switch case
	if ((errno == EAGAIN) || (errno == EINPROGRESS))
		return Socket::NotReady;

	switch (errno) {
		case EWOULDBLOCK:	return Socket::NotReady;
		case ECONNABORTED:	return Socket::Disconnected;
		case ECONNRESET:	return Socket::Disconnected;
		case ETIMEDOUT:		return Socket::Disconnected;
		case ENETRESET:		return Socket::Disconnected;
		case ENOTCONN:		return Socket::Disconnected;
		case EPIPE:			return Socket::Disconnected;
		default:			return Socket::Error;
	}
}

void SocketImpl::setSendTimeout(SocketHandle sock, const Time& timeout) {
	struct timeval time;
	time.tv_sec  = static_cast<long>(timeout.asMicroseconds() / 1000000);
	time.tv_usec = static_cast<long>(timeout.asMicroseconds() % 1000000);
	setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&time, sizeof time);
}

void SocketImpl::setReceiveTimeout(SocketHandle sock, const Time & timeout) {
	struct timeval time;
	time.tv_sec  = static_cast<long>(timeout.asMicroseconds() / 1000000);
	time.tv_usec = static_cast<long>(timeout.asMicroseconds() % 1000000);
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&time, sizeof time);
}

}}}

#endif
