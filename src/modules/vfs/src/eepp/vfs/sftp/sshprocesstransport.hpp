#ifndef EE_VFS_SFTP_SSHPROCESSTRANSPORT_HPP
#define EE_VFS_SFTP_SSHPROCESSTRANSPORT_HPP

#include "sshtransport.hpp"
#include <memory>
#include <mutex>
#include <string>

namespace EE { namespace System {
class Process;
}} // namespace EE::System

namespace EE { namespace VFS { namespace sftp {

class SSHIOContext;

/** Transport through a ProxyCommand child process (stdin/stdout). */
class SSHProcessTransport : public SSHTransport {
  public:
	SSHProcessTransport( std::shared_ptr<SSHIOContext> context, const std::string& command );

	~SSHProcessTransport() override;

	bool started() const;

	SocketHandle pollSocket() const override { return InvalidSocket; }

	short pollEvents( short ) const override { return 0; }

	libssh2_socket_t handshakeSocket() const override;

	void install( LIBSSH2_SESSION* session ) override;

	bool alive() const override;

	static ssize_t sendCallback( libssh2_socket_t, const void* buffer, size_t length, int,
								 void** abstract );

	static ssize_t recvCallback( libssh2_socket_t, void* buffer, size_t length, int,
								 void** abstract );

  private:
	std::shared_ptr<SSHIOContext> mContext;
	std::unique_ptr<System::Process> mProcess;
	/** libssh2 requires a valid socket value for the handshake even with custom send/recv
	 * callbacks; this unconnected socket only satisfies that check. */
	SocketHandle mPlaceholder{ InvalidSocket };
	mutable std::mutex mMutex;
	std::string mBuffer;
	bool mEOF{ false };
};

}}} // namespace EE::VFS::sftp

#endif
