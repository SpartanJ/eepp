#include <eepp/network/platform/win/csocketimpl.hpp>
#include <cstring>

#if EE_PLATFORM == EE_PLATFORM_WIN

namespace EE { namespace Network { namespace Private {

sockaddr_in cSocketImpl::CreateAddress(Uint32 address, unsigned short port) {
	sockaddr_in addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_addr.s_addr	= htonl(address);
	addr.sin_family			= AF_INET;
	addr.sin_port			= htons(port);

	return addr;
}

SocketHandle cSocketImpl::InvalidSocket() {
	return INVALID_SOCKET;
}

void cSocketImpl::Close(SocketHandle sock) {
	closesocket(sock);
}

void cSocketImpl::SetBlocking(SocketHandle sock, bool block) {
	u_long blocking = block ? 0 : 1;
	ioctlsocket(sock, FIONBIO, &blocking);
}

cSocket::Status cSocketImpl::GetErrorStatus() {
	switch (WSAGetLastError()) {
		case WSAEWOULDBLOCK:	return cSocket::NotReady;
		case WSAEALREADY:		return cSocket::NotReady;
		case WSAECONNABORTED:	return cSocket::Disconnected;
		case WSAECONNRESET:		return cSocket::Disconnected;
		case WSAETIMEDOUT:		return cSocket::Disconnected;
		case WSAENETRESET:		return cSocket::Disconnected;
		case WSAENOTCONN:		return cSocket::Disconnected;
		case WSAEISCONN:		return cSocket::Done; // when connecting a non-blocking socket
		default:				return cSocket::Error;
	}
}

/** Windows needs some initialization and cleanup to get
**  sockets working properly... so let's create a class that will do it automatically */
struct SocketInitializer
{
	SocketInitializer()
	{
		WSADATA init;
		WSAStartup(MAKEWORD(2, 2), &init);
	}

	~SocketInitializer()
	{
		WSACleanup();
	}
};

SocketInitializer globalInitializer;

}}}

#endif
