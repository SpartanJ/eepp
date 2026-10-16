#ifndef EE_VFS_SFTP_SSHSOCKETTRANSPORT_HPP
#define EE_VFS_SFTP_SSHSOCKETTRANSPORT_HPP

#include "sshtransport.hpp"

namespace EE { namespace VFS { namespace sftp {

/** TCP transport. */
class SSHSocketTransport : public SSHTransport {
  public:
	explicit SSHSocketTransport( SocketHandle socket ) : mSocket( socket ) {}

	~SSHSocketTransport() override { SSHSocket::close( mSocket ); }

	SocketHandle pollSocket() const override { return mSocket; }

	short pollEvents( short directions ) const override { return directions; }

	libssh2_socket_t handshakeSocket() const override {
		return static_cast<libssh2_socket_t>( mSocket );
	}

  private:
	SocketHandle mSocket;
};

}}} // namespace EE::VFS::sftp

#endif
