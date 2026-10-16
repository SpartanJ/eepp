#ifndef EE_VFS_SFTP_SSHCHANNELPUMP_HPP
#define EE_VFS_SFTP_SSHCHANNELPUMP_HPP

#include <eepp/vfs/error.hpp>

namespace EE { namespace VFS { namespace sftp {

class SSHConnection;

/** Long-lived channel consumer (for example the remote watch agent) serviced on every pass of
 * the I/O loop, independently of the serialized operation queue. Pumps must only touch their own
 * channel, so they never interleave with the session-level state of queued operations. */
class SSHChannelPump {
  public:
	virtual ~SSHChannelPump() = default;

	/** Non-blocking read/write of the pump's channel. @return False to unregister. */
	virtual bool pump( SSHConnection& connection ) = 0;

	/** The connection failed or closed; the pump is unregistered afterwards. */
	virtual void connectionLost( const VFSError& error ) = 0;
};

}}} // namespace EE::VFS::sftp

#endif
