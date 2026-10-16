#ifndef EE_VFS_SFTP_SSHWAKESIGNAL_HPP
#define EE_VFS_SFTP_SSHWAKESIGNAL_HPP

#include "sshsocket.hpp"

namespace EE { namespace VFS { namespace sftp {

/** Self-pipe used to wake a thread blocked in SSHSocket::poll(). On Windows a connected loopback
 * socket pair is used because WSAPoll() only accepts sockets. */
class SSHWakeSignal {
  public:
	SSHWakeSignal();

	~SSHWakeSignal();

	SSHWakeSignal( const SSHWakeSignal& ) = delete;

	SSHWakeSignal& operator=( const SSHWakeSignal& ) = delete;

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
