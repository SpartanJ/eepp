#ifndef EE_NETWORKCSOCKET_HPP
#define EE_NETWORKCSOCKET_HPP

#include <eepp/config.hpp>
#include <eepp/core/noncopyable.hpp>
#include <eepp/network/sockethandle.hpp>
#include <eepp/system/time.hpp>
#include <string>

namespace EE { namespace Network {
class SocketSelector;

/** @brief Base class for all the socket types */
class EE_API Socket : NonCopyable {
  public:
	/** @brief Status codes that may be returned by socket functions */
	enum Status {
		Done,		  ///< The socket has sent / received the data
		NotReady,	  ///< The socket is not ready to send / receive data yet
		Partial,	  ///< The socket sent a part of the data
		Disconnected, ///< The TCP socket has been disconnected
		Error		  ///< An unexpected error happened
	};

	/** @brief Some special values used by sockets */
	enum {
		AnyPort = 0 ///< Special value that tells the system to pick any available port
	};

	/**  @brief Destructor */
	virtual ~Socket();

	/** @brief Set the blocking state of the socket
	**  In blocking mode, calls will not return until they have
	**  completed their task. For example, a call to Receive in
	**  blocking mode won't return until some data was actually
	**  received.
	**  In non-blocking mode, calls will always return immediately,
	**  using the return code to signal whether there was data
	**  available or not.
	**  By default, all sockets are blocking.
	**  @param blocking True to set the socket as blocking, false for non-blocking
	**  @see IsBlocking */
	void setBlocking( bool blocking );

	/** @brief Tell whether the socket is in blocking or non-blocking mode
	**  @return True if the socket is blocking, false otherwise
	**  @see SetBlocking */
	bool isBlocking() const;

	/** Borrow the native socket handle for OS polling/interoperability. It can be invalid before
	 * creation or after close. Ownership stays with this socket: never close the handle, and
	 * finish all native operations before disconnecting or destroying the socket. */
	SocketHandle getHandle() const;

  protected:
	/** @brief Types of protocols that the socket can use */
	enum Type {
		Tcp,  ///< TCP protocol
		Udp,  ///< UDP protocol
		Local ///< Local stream protocol
	};

	/** @brief Default constructor
	**  This constructor can only be accessed by derived classes.
	**  @param type Type of the socket (TCP or UDP) */
	Socket( Type type );

	/** @brief Create the internal representation of the socket
	///
	**  This function can only be accessed by derived classes.
	**  @return True when a valid socket exists, false when socket creation failed. */
	bool create();

	/** @brief Create the internal representation of the socket from a socket handle
	**  This function can only be accessed by derived classes.
	**  @param handle OS-specific handle of the socket to wrap */
	void create( SocketHandle handle );

	static bool makeLocalAddress( const std::string& path, void* address, size_t size );

	Status connectAddress( const void* address, size_t size, System::Time timeout );

	Status connectionStatus() const;

	Status sendStream( const void* data, size_t size, size_t& sent );

	Status receiveStream( void* data, size_t size, size_t& received );

	/** @brief Close the socket gracefully
	**  This function can only be accessed by derived classes. */
	void close();

  protected:
	friend class SocketSelector;
	// Member data
	Type mType;			  ///< Type of the socket (TCP or UDP)
	SocketHandle mSocket; ///< Socket descriptor
	bool mIsBlocking;	  ///< Current blocking mode of the socket
};

}} // namespace EE::Network

#endif // EE_NETWORKCSOCKET_HPP

/**
@class EE::Network::Socket

This class mainly defines internal stuff to be used by
derived classes.

The only public features that it defines, and which
is therefore common to all the socket classes, is the
blocking state. All sockets can be set as blocking or
non-blocking.

In blocking mode, socket functions will hang until
the operation completes, which means that the entire
program (well, in fact the current thread if you use
multiple ones) will be stuck waiting for your socket
operation to complete.

In non-blocking mode, all the socket functions will
return immediately. If the socket is not ready to complete
the requested operation, the function simply returns
the proper status code (Socket::NotReady).
The default mode, which is blocking, is the one that is
generally used, in combination with threads or selectors.

The non-blocking mode is rather used in real-time
applications that run an endless loop that can poll
the socket often enough, and cannot afford blocking
this loop.

@see EE::Network::TcpListener, EE::Network::TcpSocket, EE::Network::UdpSocket
*/
