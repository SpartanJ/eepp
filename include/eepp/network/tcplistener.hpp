#ifndef EE_NETWORKCTCPLISTENER_HPP
#define EE_NETWORKCTCPLISTENER_HPP

#include <eepp/network/socket.hpp>
#include <eepp/network/ipaddress.hpp>

namespace EE { namespace Network {

class TcpSocket;

/** @brief Socket that listens to new TCP connections */
class EE_API TcpListener : public Socket {
public :

	/** @brief Default constructor */
	TcpListener();

	/** @brief Get the port to which the socket is bound locally
	**  If the socket is not listening to a port, this function
	**  returns 0.
	**  @return Port to which the socket is bound
	**  @see Listen */
	unsigned short getLocalPort() const;

	/** @brief Start listening for connections
	**  This functions makes the socket listen to the specified
	**  port, waiting for new connections.
	**  If the socket was previously listening to another port,
	**  it will be stopped first and bound to the new port.
	**  @param port Port to listen for new connections
	**  @param address Address of the interface to listen on
	**  @return Status code
	**  @see Accept, Close */
	Status listen(unsigned short port, const IpAddress& address = IpAddress::Any);

	/** @brief Stop listening and close the socket
	**  This function gracefully stops the listener. If the
	**  socket is not listening, this function has no effect.
	**  @see Listen */
	void close();

	/** @brief Accept a new connection
	**  If the socket is in blocking mode, this function will
	**  not return until a connection is actually received.
	**  @param socket Socket that will hold the new connection
	**  @return Status code
	**  @see Listen */
	Status accept(TcpSocket& socket);
};

}}

#endif // EE_NETWORKCTCPLISTENER_HPP

/**
@class EE::Network::TcpListener

A listener socket is a special type of socket that listens to
a given port and waits for connections on that port.
This is all it can do.

When a new connection is received, you must call accept and
the listener returns a new instance of TcpSocket that
is properly initialized and can be used to communicate with
the new client.

Listener sockets are specific to the TCP protocol,
UDP sockets are connectionless and can therefore communicate
directly. As a consequence, a listener socket will always
return the new connections as TcpSocket instances.

A listener is automatically closed on destruction, like all
other types of socket. However if you want to stop listening
before the socket is destroyed, you can call its close()
function.

Usage example:
@code
// Create a listener socket and make it wait for new
// connections on port 55001
TcpListener listener;
listener.listen(55001);

// Endless loop that waits for new connections
while (running) {
	 TcpSocket client;
	 if (listener.accept(client) == Socket::Done) {
		 // A new client just connected!
		 std::cout << "New connection received from " << client.getRemoteAddress() << std::endl;
		 doSomethingWith(client);
	 }
}
@endcode

@see EE::Network::TcpSocket, EE::Network::Socket
*/
