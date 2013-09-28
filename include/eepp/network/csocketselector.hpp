#ifndef EE_NETWORKCSOCKETSELECTOR_HPP
#define EE_NETWORKCSOCKETSELECTOR_HPP

#include <eepp/network/base.hpp>
#include <eepp/system/ctime.hpp>
using namespace EE::System;

namespace EE { namespace Network {

class cSocket;

/** @brief Multiplexer that allows to read from multiple sockets */
class EE_API cSocketSelector
{
	public :
		/** @brief Default constructor */
		cSocketSelector();

		/** @brief Copy constructor
		**  @param copy Instance to copy */
		cSocketSelector(const cSocketSelector& copy);

		/** @brief Destructor */
		~cSocketSelector();

		/** @brief Add a new socket to the selector
		**  This function keeps a weak reference to the socket,
		**  so you have to make sure that the socket is not destroyed
		**  while it is stored in the selector.
		**  This function does nothing if the socket is not valid.
		**  @param socket Reference to the socket to add
		**  @see Remove, Clear */
		void Add(cSocket& socket);

		/** @brief Remove a socket from the selector
		**  This function doesn't destroy the socket, it simply
		**  removes the reference that the selector has to it.
		**  @param socket Reference to the socket to remove
		**  @see Add, Clear */
		void Remove(cSocket& socket);

		/** @brief Remove all the sockets stored in the selector
		**  This function doesn't destroy any instance, it simply
		**  removes all the references that the selector has to
		**  external sockets.
		**  @see Add, Remove */
		void Clear();

		/** @brief Wait until one or more sockets are ready to receive
		**  This function returns as soon as at least one socket has
		**  some data available to be received. To know which sockets are
		**  ready, use the isReady function.
		**  If you use a timeout and no socket is ready before the timeout
		**  is over, the function returns false.
		**  @param timeout Maximum time to wait, (use cTime::Zero for infinity)
		**  @return True if there are sockets ready, false otherwise
		**  @see IsReady */
		bool Wait(cTime timeout = cTime::Zero);

		/** @brief Test a socket to know if it is ready to receive data
		**  This function must be used after a call to Wait, to know
		**  which sockets are ready to receive data. If a socket is
		**  ready, a call to receive will never block because we know
		**  that there is data available to read.
		**  Note that if this function returns true for a cTcpListener,
		**  this means that it is ready to accept a new connection.
		**  @param socket cSocket to test
		**  @return True if the socket is ready to read, false otherwise
		**  @see IsReady */
		bool IsReady(cSocket& socket) const;

		/** @brief Overload of assignment operator
		**  @param right Instance to assign
		**  @return Reference to self */
		cSocketSelector& operator =(const cSocketSelector& right);
	private :
		struct cSocketSelectorImpl;

		// Member data
		cSocketSelectorImpl* mImpl; ///< Opaque pointer to the implementation (which requires OS-specific types)
};

}}

#endif // EE_NETWORKCSOCKETSELECTOR_HPP

/**
@class cSocketSelector
@ingroup Network

cSocket selectors provide a way to wait until some data is
available on a set of sockets, instead of just one. This
is convenient when you have multiple sockets that may
possibly receive data, but you don't know which one will
be ready first. In particular, it avoids to use a thread
for each socket; with selectors, a single thread can handle
all the sockets.

All types of sockets can be used in a selector:
@li cTcpListener
@li cTcpSocket
@li cUdpSocket

A selector doesn't store its own copies of the sockets
(socket classes are not copyable anyway), it simply keeps
a reference to the original sockets that you pass to the
"add" function. Therefore, you can't use the selector as a
socket container, you must store them oustide and make sure
that they are alive as long as they are used in the selector.

Using a selector is simple:
@li populate the selector with all the sockets that you want to observe
@li make it wait until there is data available on any of the sockets
@li test each socket to find out which ones are ready

Usage example:
@code
// Create a socket to listen to new connections
cTcpListener listener;
listener.Listen(55001);

// Create a list to store the future clients
std::list<cTcpSocket*> clients;

// Create a selector
cSocketSelector selector;

// Add the listener to the selector
selector.Add(listener);

// Endless loop that waits for new connections
while (running)
{
	 // Make the selector wait for data on any socket
	 if (selector.Wait())
	 {
		 // Test the listener
		 if (selector.IsReady(listener))
		 {
			 // The listener is ready: there is a pending connection
			 cTcpSocket* client = new cTcpSocket;
			 if (listener.Accept(*client) == cSocket::Done)
			 {
				 // Add the new client to the clients list
				 clients.push_back(client);

				 // Add the new client to the selector so that we will
				 // be notified when he sends something
				 selector.Add(*client);
			 }
			 else
			 {
				 // Error, we won't get a new connection, delete the socket
				 delete client;
			 }
		 }
		 else
		 {
			 // The listener socket is not ready, test all other sockets (the clients)
			 for (std::list<cTcpSocket*>::iterator it = clients.begin(); it != clients.end(); ++it)
			 {
				 cTcpSocket& client = **it;
				 if (selector.IsReady(client))
				 {
					 // The client has sent some data, we can receive it
					 cPacket packet;
					 if (client.Receive(packet) == cSocket::Done)
					 {
						 ...
					 }
				 }
			 }
		 }
	 }
}
@endcode

@see cSocket
*/
