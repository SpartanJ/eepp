#ifndef EE_VFS_SFTP_SFTPPROVIDER_HPP
#define EE_VFS_SFTP_SFTPPROVIDER_HPP

#include <eepp/core/task.hpp>
#include <eepp/vfs/provider.hpp>
#include <eepp/vfs/ssh/sshconfig.hpp>
#include <eepp/vfs/watch.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace EE { namespace VFS {

class VirtualFileSystem;

/** Unknown host key presented for a user decision. Changed (mismatching) host keys are never
 * offered: they always fail with VFSErrorCode::HostKeyRejected. */
struct SSHHostKeyInfo {
	std::string host;
	Uint16 port{ 22 };
	/** Key algorithm, for example "ecdsa-sha2-nistp256". */
	std::string keyType;
	/** OpenSSH style fingerprint: "SHA256:" followed by unpadded base64. */
	std::string fingerprint;
	/** known_hosts file the key will be stored in on AcceptAndSave. */
	std::string knownHostsFile;
};

enum class SSHHostKeyDecision : Uint8 {
	Reject,
	/** Trust for this connection only. */
	AcceptOnce,
	/** Trust and append to the user known_hosts file. */
	AcceptAndSave,
};

/** Application policy for unknown host keys (for example a UI dialog). */
class SSHHostKeyVerifier {
  public:
	virtual ~SSHHostKeyVerifier() = default;

	/** May resolve on any thread; the SSH I/O thread is never blocked while waiting. */
	virtual Task<SSHHostKeyDecision> verify( const SSHHostKeyInfo& info ) = 0;
};

struct SSHPrompt {
	std::string text;
	bool echo{ false };
};

struct SSHAuthenticationRequest {
	enum class Kind : Uint8 {
		Password,
		/** Passphrase of an encrypted identity file. */
		Passphrase,
		KeyboardInteractive,
	};

	Kind kind{ Kind::Password };
	std::string user;
	std::string host;
	/** Passphrase requests: the identity file. */
	std::string identityFile;
	/** Keyboard-interactive name and instruction. */
	std::string name;
	std::string instruction;
	/** One entry per expected answer (a single prompt for passwords and passphrases). */
	std::vector<SSHPrompt> prompts;
	/** 1-based attempt number, bounded by NumberOfPasswordPrompts. */
	Uint32 attempt{ 1 };
};

/** Supplies secrets on demand. Secrets are used for one authentication attempt and never stored
 * by the VFS (and never appear in URIs). */
class SSHAuthenticationHandler {
  public:
	virtual ~SSHAuthenticationHandler() = default;

	/** @return One answer per prompt, or nullopt to give up on this method. */
	virtual Task<std::optional<std::vector<std::string>>>
	request( const SSHAuthenticationRequest& request ) = 0;
};

struct SFTPProviderConfig {
	std::shared_ptr<SSHHostKeyVerifier> hostKeyVerifier;
	std::shared_ptr<SSHAuthenticationHandler> authenticationHandler;
	/** Builds the OpenSSH configuration context for a host. Defaults to
	 * SSHConfigContext::system(). URI user and port are applied as overrides afterwards. */
	std::function<SSHConfigContext( const std::string& host )> configContextFactory;
	/** Connect timeout when ConnectTimeout is not configured. */
	Time defaultConnectTimeout{ System::Seconds( 30 ) };
	/** Idle connections are closed after this long. */
	Time idleTimeout{ System::Minutes( 5 ) };
	/** Remote command used for the efsw notification agent (run over an exec channel). Empty
	 * disables remote agent watching (polling is used instead). */
	std::string watchAgentCommand{ "efsw-agent --stdio" };
};

namespace sftp {
class SFTPProviderImpl;
}

/** @brief `sftp://` provider using libssh2.
 *
 * URIs have the form sftp://[user@]host[:port]/absolute/path. The host is resolved through the
 * user's OpenSSH configuration (Host/Match/Include, HostName, Port, User, IdentityFile,
 * ProxyJump, ProxyCommand, known hosts and authentication options), so `sftp://makyo/` behaves
 * like `ssh makyo`. Passwords are never accepted in URIs.
 *
 * Connections are pooled by effective configuration (not by path) and shared by every operation
 * for that destination. All SSH sessions are driven by one I/O thread in non-blocking mode;
 * provider methods never block the caller.
 */
class SFTPProvider : public VFSProvider {
  public:
	explicit SFTPProvider( SFTPProviderConfig config = {} );

	~SFTPProvider() override;

	std::string_view scheme() const override { return "sftp"; }

	ProviderCapabilities capabilities( const URI& uri ) const override;

	VFSTask<FileInfo> stat( const URI& uri, const StatOptions& options,
							CancellationToken token ) override;

	VFSTask<DirectoryHandle> openDirectory( const URI& uri, const DirectoryOpenOptions& options,
											CancellationToken token ) override;

	VFSTask<FileHandle> open( const URI& uri, const OpenOptions& options,
							  CancellationToken token ) override;

	VFSTask<void> mkdir( const URI& uri, const MkDirOptions& options,
						 CancellationToken token ) override;

	VFSTask<void> remove( const URI& uri, const RemoveOptions& options,
						  CancellationToken token ) override;

	VFSTask<void> rename( const URI& from, const URI& to, const RenameOptions& options,
						  CancellationToken token ) override;

	VFSTask<URI> canonicalize( const URI& uri, CancellationToken token ) override;

	VFSTask<void> setPermissions( const URI& uri, Uint32 permissions,
								  CancellationToken token ) override;

	VFSTask<SpaceInfo> spaceInfo( const URI& uri, CancellationToken token ) override;

	void setHostKeyVerifier( std::shared_ptr<SSHHostKeyVerifier> verifier );

	void setAuthenticationHandler( std::shared_ptr<SSHAuthenticationHandler> handler );

	/** Resolves the effective OpenSSH configuration for a URI (no connection is made). */
	VFSTask<SSHResolvedConfig> resolveConfig( const URI& uri );

	/** Establishes (or reuses) the connection for a URI. */
	VFSTask<void> connect( const URI& uri );

	/** Closes every pooled connection. In-flight operations fail with ConnectionLost. */
	void disconnectAll();

	size_t connectionCount() const;

	/** Watch backend for sftp:// URIs: remote efsw-agent over the same SSH connection, with
	 * polling through the VFS as fallback. */
	std::shared_ptr<WatchBackend> createWatchBackend( VirtualFileSystem& vfs );

	/** Internal access for the remote watcher. */
	const std::shared_ptr<sftp::SFTPProviderImpl>& impl() const { return mImpl; }

  private:
	std::shared_ptr<sftp::SFTPProviderImpl> mImpl;
};

}} // namespace EE::VFS

#endif
