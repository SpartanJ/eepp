#ifndef EE_VFS_SFTP_SSHAGENT_HPP
#define EE_VFS_SFTP_SSHAGENT_HPP

#include <atomic>
#include <eepp/config.hpp>
#include <eepp/vfs/error.hpp>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace EE { namespace VFS { namespace sftp {

/** @brief Minimal ssh-agent client: lists the agent's keys and signs with them.
 *
 * Every call blocks on the agent, and signing may wait for the user (a hardware key touch, a
 * confirmation dialog): use it from a worker thread, never from the SSH I/O thread. One call at a
 * time. Speaks the agent protocol over Unix-domain sockets; elsewhere (Windows named pipes,
 * Pageant) supported() is false and libssh2's own agent support is used.
 */
class SSHAgent {
  public:
	struct Identity {
		/** Public key blob (SSH wire format). */
		std::string blob;
		std::string comment;
	};

	/** @return True where agents are reached through Unix-domain sockets, which this client
	 * speaks: every POSIX platform that builds SFTP (Linux, BSD, macOS, iOS). It does not mean an
	 * agent runs: without one, connecting fails and authentication goes on with identity files.
	 * Windows agents are named pipes or Pageant, left to libssh2. */
	static bool supported();

	explicit SSHAgent( std::string socketPath );

	~SSHAgent();

	/** Interrupts a pending request and prevents further requests. Safe from any thread. */
	void cancel();

	SSHAgent( const SSHAgent& ) = delete;

	SSHAgent& operator=( const SSHAgent& ) = delete;

	VFSResult<std::vector<Identity>> listIdentities();

	/** Signs data with the key of blob for the signature algorithm (for example "rsa-sha2-512",
	 * which selects the agent's RSA SHA-2 flags). @return The raw signature, without its
	 * algorithm name (as libssh2 sign callbacks return it). */
	VFSResult<std::string> sign( std::string_view blob, std::string_view data,
								 std::string_view algorithm );

	/** @return The signature algorithm of a SSH_MSG_USERAUTH_REQUEST "publickey" signing payload
	 * (session identifier followed by the request), empty when it cannot be parsed. */
	static std::string_view algorithmOfSignedRequest( std::string_view data );

  private:
	VFSResult<std::string> transact( std::string_view request );

	void disconnect();

	std::string mPath;
	std::mutex mSocketMutex;
	int mSocket{ -1 };
	std::atomic<bool> mCancelled{ false };
};

}}} // namespace EE::VFS::sftp

#endif
