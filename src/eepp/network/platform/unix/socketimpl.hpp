#ifndef EE_NETWORKCSOCKETIMPL_UNIX_HPP
#define EE_NETWORKCSOCKETIMPL_UNIX_HPP

#include <eepp/config.hpp>

#if defined( EE_PLATFORM_POSIX )

#include <arpa/inet.h>
#include <eepp/network/socket.hpp>
#include <eepp/system/time.hpp>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

using namespace EE::System;

namespace EE { namespace Network { namespace Private {

/** @brief Helper class implementing all the non-portable socket stuff; this is the Unix version */
class SocketImpl {
  public:
	// Types
	typedef socklen_t AddrLength;

	/** @brief  Create an internal sockaddr_in address
	**  @param address Target address
	**  @param port	Target port
	**  @return sockaddr_in ready to be used by socket functions */
	static sockaddr_in createAddress( Uint32 address, unsigned short port );

	/** @brief  Return the value of the invalid socket
	**  @return Special value of the invalid socket */
	static SocketHandle invalidSocket();

	/** @brief  Close and destroy a socket
	**  @param sock Handle of the socket to close */
	static void close( SocketHandle sock );

	/** @brief  Set a socket as blocking or non-blocking
	**  @param sock  Handle of the socket
	**  @param block New blocking state of the socket */
	static void setBlocking( SocketHandle sock, bool block );

	/** Get the last socket error status
	**  @return Status corresponding to the last socket error */
	static Socket::Status getErrorStatus();

	/** Set the send timeout */
	static void setSendTimeout( SocketHandle sock, const Time& timeout );

	/** Set the receive timeout */
	static void setReceiveTimeout( SocketHandle sock, const Time& timeout );
};

}}} // namespace EE::Network::Private

#endif

#endif // EE_NETWORKCSOCKETIMPL_UNIX_HPP
