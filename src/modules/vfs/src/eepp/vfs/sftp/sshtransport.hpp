#ifndef EE_VFS_SFTP_SSHTRANSPORT_HPP
#define EE_VFS_SFTP_SSHTRANSPORT_HPP

#include "sshsocket.hpp"
#include <libssh2.h>

namespace EE { namespace VFS { namespace sftp {

/** Byte stream under an SSH session: a TCP socket, a direct-tcpip channel of a parent session
 * (ProxyJump) or a ProxyCommand process. */
class SSHTransport {
  public:
	virtual ~SSHTransport() = default;

	/** Final connected TCP endpoint, when visible. A jump channel or proxy process cannot
	 * expose the final peer's address, so its identity remains unknown here. */
	virtual std::optional<SSHSocket::Endpoint> peerEndpoint() const { return std::nullopt; }

	/** Socket to poll, or InvalidSocket when readiness is signaled through the wake pipe. */
	virtual SocketHandle pollSocket() const = 0;

	virtual short pollEvents( short sessionDirections ) const = 0;

	/** libssh2 socket argument for the handshake. */
	virtual libssh2_socket_t handshakeSocket() const = 0;

	/** Installs send/recv callbacks for non-socket transports. */
	virtual void install( LIBSSH2_SESSION* session ) { (void)session; }

	/** Called on the I/O thread before stepping; @return False when the transport died. */
	virtual bool alive() const { return true; }
};

}}} // namespace EE::VFS::sftp

#endif
