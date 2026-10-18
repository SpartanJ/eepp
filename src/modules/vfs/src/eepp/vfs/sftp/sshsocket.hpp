#ifndef EE_VFS_SFTP_SSHSOCKET_HPP
#define EE_VFS_SFTP_SSHSOCKET_HPP

#include <array>
#include <eepp/config.hpp>
#include <optional>
#include <string>

#if EE_PLATFORM == EE_PLATFORM_WIN
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <poll.h>
#endif

namespace EE { namespace VFS { namespace sftp {

#if EE_PLATFORM == EE_PLATFORM_WIN
using SocketHandle = SOCKET;
constexpr SocketHandle InvalidSocket = INVALID_SOCKET;
using PollFd = WSAPOLLFD;
#else
using SocketHandle = int;
constexpr SocketHandle InvalidSocket = -1;
using PollFd = struct pollfd;
#endif

/** Non-blocking TCP sockets and polling for the SSH I/O thread, over the platform socket API
 * (the descriptors are handed to libssh2). */
class SSHSocket {
  public:
	/** Numeric connected peer, independent of hostname spelling. IPv4-mapped IPv6 addresses
	 * use the IPv4 representation so the same peer compares equal across address families. */
	struct Endpoint {
		std::array<Uint8, 16> address{};
		Uint32 scope{ 0 };
		Uint16 port{ 0 };
		Uint8 family{ 0 };

		bool operator==( const Endpoint& ) const = default;
	};

	/** Returns the actual TCP peer without another DNS lookup; unknown on error. */
	static std::optional<Endpoint> peerEndpoint( SocketHandle socket );

	/** Initializes the platform socket layer once (WSAStartup on Windows). */
	static void init();

	static void close( SocketHandle socket );

	static bool setNonBlocking( SocketHandle socket );

	/** poll() / WSAPoll(), retried on EINTR. */
	static int poll( PollFd* fds, size_t count, int timeoutMs );

	/** @return The pending socket error (SO_ERROR), 0 when none. */
	static int pendingError( SocketHandle socket );

	static std::string errorString( int error );

	/** Connects to host:port with a timeout (blocking for the caller, socket left non-blocking).
	 * Tries every resolved address in order. @return InvalidSocket on failure with error filled.
	 */
	static SocketHandle connect( const std::string& host, Uint16 port, int timeoutMs,
								 std::string& error );

  private:
	static int lastError();

	static bool connectInProgress( int error );
};

}}} // namespace EE::VFS::sftp

#endif
