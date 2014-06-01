#ifndef EE_NETWORKCTCPSOCKET_HPP
#define EE_NETWORKCTCPSOCKET_HPP

#include <eepp/network/csocket.hpp>
#include <eepp/system/ctime.hpp>
using namespace EE::System;

namespace EE { namespace Network {

class cTcpListener;
class cIpAddress;
class cPacket;

/** @brief Specialized socket using the TCP protocol */
class EE_API cTcpSocket : public cSocket {
	public:

	/** @brief Default constructor */
	cTcpSocket();

	/** @brief Get the port to which the socket is bound locally
	**  If the socket is not connected, this function returns 0.
	**  @return Port to which the socket is bound
	**  @see Connect, GetRemotePort */
	unsigned short GetLocalPort() const;

	/** @brief Get the address of the connected peer
	**  It the socket is not connected, this function returns
	**  cIpAddress::None.
	**  @return Address of the remote peer
	**  @see GetRemotePort */
	cIpAddress GetRemoteAddress() const;

	/** @brief Get the port of the connected peer to which
			the socket is connected
	**  If the socket is not connected, this function returns 0.
	**  @return Remote port to which the socket is connected
	**  @see GetRemoteAddress */
	unsigned short GetRemotePort() const;

	/** @brief Connect the socket to a remote peer
	**  In blocking mode, this function may take a while, especially
	**  if the remote peer is not reachable. The last parameter allows
	**  you to stop trying to connect after a given timeout.
	**  If the socket was previously connected, it is first disconnected.
	**  @param remoteAddress Address of the remote peer
	**  @param remotePort	Port of the remote peer
	**  @param timeout	   Optional maximum time to wait
	**  @return Status code
	**  @see Disconnect */
	virtual Status Connect(const cIpAddress& remoteAddress, unsigned short remotePort, cTime timeout = cTime::Zero);

	/** @brief Disconnect the socket from its remote peer
	**  This function gracefully closes the connection. If the
	**  socket is not connected, this function has no effect.
	**  @see Connect */
	virtual void Disconnect();

	/** @brief Send raw data to the remote peer
	**  This function will fail if the socket is not connected.
	**  @param data Pointer to the sequence of bytes to send
	**  @param size Number of bytes to send
	**  @return Status code
	**  @see Receive */
	virtual Status Send(const void* data, std::size_t size);

	/** @brief Receive raw data from the remote peer
	**  In blocking mode, this function will wait until some
	**  bytes are actually received.
	**  This function will fail if the socket is not connected.
	**  @param data	 Pointer to the array to fill with the received bytes
	**  @param size	 Maximum number of bytes that can be received
	**  @param received This variable is filled with the actual number of bytes received
	**  @return Status code
	**  @see Send */
	virtual Status Receive(void* data, std::size_t size, std::size_t& received);

	/** @brief Send a formatted packet of data to the remote peer
	**  This function will fail if the socket is not connected.
	**  @param packet cPacket to send
	**  @return Status code
	**  @see Receive */
	virtual Status Send(cPacket& packet);

	/** @brief Receive a formatted packet of data from the remote peer
	**  In blocking mode, this function will wait until the whole packet
	**  has been received.
	**  This function will fail if the socket is not connected.
	**  @param packet cPacket to fill with the received data
	**  @return Status code
	**  @see Send */
	virtual Status Receive(cPacket& packet);

	private:

	friend class cTcpListener;

	/** @brief Structure holding the data of a pending packet */
	struct PendingPacket {
		PendingPacket();

		Uint32			Size;		 ///< Data of packet size
		std::size_t		SizeReceived; ///< Number of size bytes received so far
		std::vector<char> Data;		 ///< Data of the packet
	};

	// Member data
	PendingPacket mPendingPacket; ///< Temporary data of the packet currently being received
};

}}

#endif // EE_NETWORKCTCPSOCKET_HPP

/**
@class cTcpSocket
@ingroup Network

TCP is a connected protocol, which means that a TCP
socket can only communicate with the host it is connected
to. It can't send or receive anything if it is not connected.

The TCP protocol is reliable but adds a slight overhead.
It ensures that your data will always be received in order
and without errors (no data corrupted, lost or duplicated).

When a socket is connected to a remote host, you can
retrieve informations about this host with the
GetRemoteAddress and GetRemotePort functions. You can
also get the local port to which the socket is bound
(which is automatically chosen when the socket is connected),
with the GetLocalPort function.

Sending and receiving data can use either the low-level
or the high-level functions. The low-level functions
process a raw sequence of bytes, and cannot ensure that
one call to Send will exactly match one call to Receive
at the other end of the socket.

The high-level interface uses packets (see cPacket),
which are easier to use and provide more safety regarding
the data that is exchanged. You can look at the cPacket
class to get more details about how they work.

The socket is automatically disconnected when it is destroyed,
but if you want to explicitely close the connection while
the socket instance is still alive, you can call disconnect.

Usage example:
@code
// ----- The client -----

// Create a socket and connect it to 192.168.1.50 on port 55001
cTcpSocket socket;
socket.Connect("192.168.1.50", 55001);

// Send a message to the connected host
std::string message = "Hi, I am a client";
socket.Send(message.c_str(), message.size() + 1);

// Receive an answer from the server
char buffer[1024];
std::size_t received = 0;
socket.Receive(buffer, sizeof(buffer), received);
std::cout << "The server said: " << buffer << std::endl;

// ----- The server -----

// Create a listener to wait for incoming connections on port 55001
cTcpListener listener;
listener.Listen(55001);

// Wait for a connection
cTcpSocket socket;
listener.Accept(socket);
std::cout << "New client connected: " << socket.GetRemoteAddress() << std::endl;

// Receive a message from the client
char buffer[1024];
std::size_t received = 0;
socket.Receive(buffer, sizeof(buffer), received);
std::cout << "The client said: " << buffer << std::endl;

// Send an answer
std::string message = "Welcome, client";
socket.Send(message.c_str(), message.size() + 1);
@endcode

@see cSocket, cUdpSocket, cPacket
*/
