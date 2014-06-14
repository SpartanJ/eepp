#include <eepp/network/tcplistener.hpp>
#include <eepp/network/tcpsocket.hpp>
#include <eepp/network/platform/platformimpl.hpp>

namespace EE { namespace Network {

TcpListener::TcpListener() :
	Socket(Tcp)
{
}

unsigned short TcpListener::GetLocalPort() const {
	if (GetHandle() != Private::SocketImpl::InvalidSocket()) {
		// Retrieve informations about the local end of the socket
		sockaddr_in address;
		Private::SocketImpl::AddrLength size = sizeof(address);

		if (getsockname(GetHandle(), reinterpret_cast<sockaddr*>(&address), &size) != -1) {
			return ntohs(address.sin_port);
		}
	}

	// We failed to retrieve the port
	return 0;
}

Socket::Status TcpListener::Listen(unsigned short port) {
	// Create the internal socket if it doesn't exist
	Create();

	// Bind the socket to the specified port
	sockaddr_in address = Private::SocketImpl::CreateAddress(INADDR_ANY, port);
	if (bind(GetHandle(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
		// Not likely to happen, but...
		eePRINTL( "Failed to bind listener socket to port %d", port );
		return Error;
	}

	// Listen to the bound port
	if (::listen(GetHandle(), 0) == -1) {
		// Oops, socket is deaf
		eePRINTL( "Failed to Listen to port %d", port );
		return Error;
	}

	return Done;
}

void TcpListener::Close() {
	// Simply close the socket
	Socket::Close();
}

Socket::Status TcpListener::Accept(TcpSocket& socket) {
	// Make sure that we're listening
	if (GetHandle() == Private::SocketImpl::InvalidSocket()) {
		eePRINTL( "Failed to accept a new connection, the socket is not listening" );
		return Error;
	}

	// Accept a new connection
	sockaddr_in address;
	Private::SocketImpl::AddrLength length = sizeof(address);
	SocketHandle remote = ::accept(GetHandle(), reinterpret_cast<sockaddr*>(&address), &length);

	// Check for errors
	if (remote == Private::SocketImpl::InvalidSocket())
		return Private::SocketImpl::GetErrorStatus();

	// Initialize the new connected socket
	socket.Close();
	socket.Create(remote);

	return Done;
}

}}
