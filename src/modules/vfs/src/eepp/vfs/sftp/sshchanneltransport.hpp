#ifndef EE_VFS_SFTP_SSHCHANNELTRANSPORT_HPP
#define EE_VFS_SFTP_SSHCHANNELTRANSPORT_HPP

#include "sshoperation.hpp"
#include "sshtransport.hpp"
#include <memory>
#include <string>

namespace EE { namespace VFS { namespace sftp {

class SSHConnection;

/** Transport tunneled through a direct-tcpip channel of a parent connection (ProxyJump). */
class SSHChannelTransport : public SSHTransport {
  public:
	SSHChannelTransport( std::shared_ptr<SSHConnection> parent, std::string host, Uint16 port );

	~SSHChannelTransport() override;

	SocketHandle pollSocket() const override;

	short pollEvents( short directions ) const override;

	libssh2_socket_t handshakeSocket() const override;

	void install( LIBSSH2_SESSION* session ) override;

	bool alive() const override;

	/** Opens the channel; @return Done once open (check channel()), Again to retry. */
	Step open( VFSError& error );

	LIBSSH2_CHANNEL* channel() const { return mChannel; }

	static ssize_t sendCallback( libssh2_socket_t, const void* buffer, size_t length, int,
								 void** abstract );

	static ssize_t recvCallback( libssh2_socket_t, void* buffer, size_t length, int,
								 void** abstract );

  private:
	std::shared_ptr<SSHConnection> mParent;
	std::string mHost;
	Uint16 mPort;
	LIBSSH2_CHANNEL* mChannel{ nullptr };
};

}}} // namespace EE::VFS::sftp

#endif
