#ifndef EE_NETWORKCTCPLISTENER_HPP
#define EE_NETWORKCTCPLISTENER_HPP

#include <eepp/network/csocket.hpp>

namespace EE { namespace Network {

class cTcpSocket;

/** @brief Socket that listens to new TCP connections */
class EE_API cTcpListener : public cSocket
{
public :

	/** @brief Default constructor */
	cTcpListener();

	/** @brief Get the port to which the socket is bound locally
	**  If the socket is not listening to a port, this function
	**  returns 0.
	**  @return Port to which the socket is bound
	**  @see Listen */
	unsigned short GetLocalPort() const;

	/** @brief Start listening for connections
	**  This functions makes the socket listen to the specified
	**  port, waiting for new connections.
	**  If the socket was previously listening to another port,
	**  it will be stopped first and bound to the new port.
	**  @param port Port to listen for new connections
	**  @return Status code
	**  @see Accept, Close */
	Status Listen(unsigned short port);

	/** @brief Stop listening and close the socket
	**  This function gracefully stops the listener. If the
	**  socket is not listening, this function has no effect.
	**  @see Listen */
	void Close();

	/** @brief Accept a new connection
	**  If the socket is in blocking mode, this function will
	**  not return until a connection is actually received.
	**  @param socket Socket that will hold the new connection
	**  @return Status code
	**  @see Listen */
	Status Accept(cTcpSocket& socket);
};

}}

#endif // EE_NETWORKCTCPLISTENER_HPP

/**
@class cTcpListener
@ingroup Network

A listener socket is a special type of socket that listens to
a given port and waits for connections on that port.
This is all it can do.

When a new connection is received, you must call accept and
the listener returns a new instance of cTcpSocket that
is properly initialized and can be used to communicate with
the new client.

Listener sockets are specific to the TCP protocol,
UDP sockets are connectionless and can therefore communicate
directly. As a consequence, a listener socket will always
return the new connections as cTcpSocket instances.

A listener is automatically closed on destruction, like all
other types of socket. However if you want to stop listening
before the socket is destroyed, you can call its close()
function.

Usage example:
@code
// Create a listener socket and make it wait for new
// connections on port 55001
cTcpListener listener;
listener.Listen(55001);

// Endless loop that waits for new connections
while (running)
{
	 cTcpSocket client;
	 if (listener.Accept(client) == cSocket::Done)
	 {
		 // A new client just connected!
		 std::cout << "New connection received from " << client.GetRemoteAddress() << std::endl;
		 doSomethingWith(client);
	 }
}
@endcode

@see cTcpSocket, cSocket
*/
