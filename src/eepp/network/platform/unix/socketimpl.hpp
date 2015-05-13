#ifndef EE_NETWORKCSOCKETIMPL_UNIX_HPP
#define EE_NETWORKCSOCKETIMPL_UNIX_HPP

#include <eepp/config.hpp>

#if defined( EE_PLATFORM_POSIX )

#include <eepp/network/socket.hpp>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

namespace EE { namespace Network { namespace Private {

/** @brief Helper class implementing all the non-portable socket stuff; this is the Unix version */
class SocketImpl {
	public :
		// Types
		typedef socklen_t AddrLength;

		/** @brief  Create an internal sockaddr_in address
		**  @param address Target address
		**  @param port	Target port
		**  @return sockaddr_in ready to be used by socket functions */
		static sockaddr_in CreateAddress(Uint32 address, unsigned short port);

		/** @brief  Return the value of the invalid socket
		**  @return Special value of the invalid socket */
		static SocketHandle InvalidSocket();

		/** @brief  Close and destroy a socket
		**  @param sock Handle of the socket to close */
		static void Close(SocketHandle sock);

		/** @brief  Set a socket as blocking or non-blocking
		**  @param sock  Handle of the socket
		**  @param block New blocking state of the socket */
		static void SetBlocking(SocketHandle sock, bool block);

		/** Get the last socket error status
		**  @return Status corresponding to the last socket error */
		static Socket::Status GetErrorStatus();
};

}}}

#endif

#endif // EE_NETWORKCSOCKETIMPL_UNIX_HPP
