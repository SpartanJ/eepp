#ifndef EE_VFS_SFTP_SSHSOCKET_HPP
#define EE_VFS_SFTP_SSHSOCKET_HPP

#include <eepp/config.hpp>
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

/** Initializes the platform socket layer once (WSAStartup on Windows). */
void socketsInit();

void closeSocket( SocketHandle socket );

bool setNonBlocking( SocketHandle socket );

int pollSockets( PollFd* fds, size_t count, int timeoutMs );

/** @return The pending socket error (SO_ERROR), 0 when none. */
int socketError( SocketHandle socket );

std::string socketErrorString( int error );

/** Connects to host:port with a timeout (blocking for the caller, socket left non-blocking).
 * Tries every resolved address in order. @return InvalidSocket on failure with error filled. */
SocketHandle connectTcp( const std::string& host, Uint16 port, int timeoutMs, std::string& error );

/** Self-pipe used to wake a thread blocked in pollSockets(). On Windows a connected loopback
 * socket pair is used because WSAPoll() only accepts sockets. */
class WakeSignal {
  public:
	WakeSignal();

	~WakeSignal();

	WakeSignal( const WakeSignal& ) = delete;

	WakeSignal& operator=( const WakeSignal& ) = delete;

	bool valid() const { return mRead != InvalidSocket; }

	/** Socket/descriptor to poll for readability. */
	SocketHandle pollHandle() const { return mRead; }

	void signal();

	void drain();

  private:
	SocketHandle mRead{ InvalidSocket };
	SocketHandle mWrite{ InvalidSocket };
};

}}} // namespace EE::VFS::sftp

#endif
