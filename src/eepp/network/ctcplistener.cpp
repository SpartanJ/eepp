#include <eepp/network/ctcplistener.hpp>
#include <eepp/network/ctcpsocket.hpp>
#include <eepp/network/platform/platformimpl.hpp>

namespace EE { namespace Network {

cTcpListener::cTcpListener() :
	cSocket(Tcp)
{
}

unsigned short cTcpListener::GetLocalPort() const {
	if (GetHandle() != Private::cSocketImpl::InvalidSocket()) {
		// Retrieve informations about the local end of the socket
		sockaddr_in address;
		Private::cSocketImpl::AddrLength size = sizeof(address);

		if (getsockname(GetHandle(), reinterpret_cast<sockaddr*>(&address), &size) != -1) {
			return ntohs(address.sin_port);
		}
	}

	// We failed to retrieve the port
	return 0;
}

cSocket::Status cTcpListener::Listen(unsigned short port) {
	// Create the internal socket if it doesn't exist
	Create();

	// Bind the socket to the specified port
	sockaddr_in address = Private::cSocketImpl::CreateAddress(INADDR_ANY, port);
	if (bind(GetHandle(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
		// Not likely to happen, but...
		//err() << "Failed to bind listener socket to port " << port << std::endl;
		return Error;
	}

	// Listen to the bound port
	if (::listen(GetHandle(), 0) == -1) {
		// Oops, socket is deaf
		//err() << "Failed to Listen to port " << port << std::endl;
		return Error;
	}

	return Done;
}

void cTcpListener::Close() {
	// Simply close the socket
	cSocket::Close();
}

cSocket::Status cTcpListener::Accept(cTcpSocket& socket) {
	// Make sure that we're listening
	if (GetHandle() == Private::cSocketImpl::InvalidSocket()) {
		//err() << "Failed to accept a new connection, the socket is not listening" << std::endl;
		return Error;
	}

	// Accept a new connection
	sockaddr_in address;
	Private::cSocketImpl::AddrLength length = sizeof(address);
	SocketHandle remote = ::accept(GetHandle(), reinterpret_cast<sockaddr*>(&address), &length);

	// Check for errors
	if (remote == Private::cSocketImpl::InvalidSocket())
		return Private::cSocketImpl::GetErrorStatus();

	// Initialize the new connected socket
	socket.Close();
	socket.Create(remote);

	return Done;
}

}}
