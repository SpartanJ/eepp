#include <eepp/network/udpsocket.hpp>
#include <eepp/network/ipaddress.hpp>
#include <eepp/network/packet.hpp>
#include <eepp/network/platform/platformimpl.hpp>
#include <algorithm>

namespace EE { namespace Network {

UdpSocket::UdpSocket() :
	Socket  (Udp),
	mBuffer(MaxDatagramSize)
{
}

unsigned short UdpSocket::GetLocalPort() const {
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

Socket::Status UdpSocket::Bind(unsigned short port) {
	// Create the internal socket if it doesn't exist
	Create();

	// Bind the socket
	sockaddr_in address = Private::SocketImpl::CreateAddress(INADDR_ANY, port);
	if (::bind(GetHandle(), reinterpret_cast<sockaddr*>(&address), sizeof(address)) == -1) {
		eePRINTL( "Failed to bind socket to port %d", port );
		return Error;
	}

	return Done;
}

void UdpSocket::Unbind() {
	// Simply close the socket
	Close();
}

Socket::Status UdpSocket::Send(const void* data, std::size_t size, const IpAddress& remoteAddress, unsigned short remotePort) {
	// Create the internal socket if it doesn't exist
	Create();

	// Make sure that all the data will fit in one datagram
	if (size > MaxDatagramSize)
	{
		eePRINTL( "Cannot send data over the network (the number of bytes to send is greater than UdpSocket::MaxDatagramSize)" );
		return Error;
	}

	// Build the target address
	sockaddr_in address = Private::SocketImpl::CreateAddress(remoteAddress.ToInteger(), remotePort);

	// Send the data (unlike TCP, all the data is always sent in one call)
	int sent = sendto(GetHandle(), static_cast<const char*>(data), static_cast<int>(size), 0, reinterpret_cast<sockaddr*>(&address), sizeof(address));

	// Check for errors
	if (sent < 0)
		return Private::SocketImpl::GetErrorStatus();

	return Done;
}

Socket::Status UdpSocket::Receive(void* data, std::size_t size, std::size_t& received, IpAddress& remoteAddress, unsigned short& remotePort) {
	// First clear the variables to fill
	received	  = 0;
	remoteAddress = IpAddress();
	remotePort	= 0;

	// Check the destination buffer
	if (!data) {
		eePRINTL( "Cannot receive data from the network (the destination buffer is invalid)" );
		return Error;
	}

	// Data that will be filled with the other computer's address
	sockaddr_in address = Private::SocketImpl::CreateAddress(INADDR_ANY, 0);

	// Receive a chunk of bytes
	Private::SocketImpl::AddrLength addressSize = sizeof(address);
	int sizeReceived = recvfrom(GetHandle(), static_cast<char*>(data), static_cast<int>(size), 0, reinterpret_cast<sockaddr*>(&address), &addressSize);

	// Check for errors
	if (sizeReceived < 0)
		return Private::SocketImpl::GetErrorStatus();

	// Fill the sender informations
	received	  = static_cast<std::size_t>(sizeReceived);
	remoteAddress = IpAddress(ntohl(address.sin_addr.s_addr));
	remotePort	= ntohs(address.sin_port);

	return Done;
}

Socket::Status UdpSocket::Send(Packet& packet, const IpAddress& remoteAddress, unsigned short remotePort) {
	// UDP is a datagram-oriented protocol (as opposed to TCP which is a stream protocol).
	// Sending one datagram is almost safe: it may be lost but if it's received, then its data
	// is guaranteed to be ok. However, splitting a packet into multiple datagrams would be highly
	// unreliable, since datagrams may be reordered, dropped or mixed between different sources.
	// That's why eepp imposes a limit on packet size so that they can be sent in a single datagram.
	// This also removes the overhead associated to packets -- there's no size to send in addition
	// to the packet's data.

	// Get the data to send from the packet
	std::size_t size = 0;
	const void* data = packet.OnSend(size);

	// Send it
	return Send(data, size, remoteAddress, remotePort);
}

Socket::Status UdpSocket::Receive(Packet& packet, IpAddress& remoteAddress, unsigned short& remotePort) {
	// See the detailed comment in Send(Packet) above.

	// Receive the datagram
	std::size_t received = 0;
	Status status = Receive(&mBuffer[0], mBuffer.size(), received, remoteAddress, remotePort);

	// If we received valid data, we can copy it to the user packet
	packet.Clear();
	if ((status == Done) && (received > 0))
		packet.OnReceive(&mBuffer[0], received);

	return status;
}


}}
