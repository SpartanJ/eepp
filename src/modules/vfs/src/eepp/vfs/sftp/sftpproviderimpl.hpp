#ifndef EE_VFS_SFTP_SFTPPROVIDERIMPL_HPP
#define EE_VFS_SFTP_SFTPPROVIDERIMPL_HPP

#include "sshconnection.hpp"
#include <eepp/system/threadpool.hpp>
#include <eepp/vfs/sftp/sftpprovider.hpp>
#include <map>
#include <memory>
#include <mutex>

namespace EE { namespace VFS { namespace sftp {

/** Shared state of SFTPProvider: configuration, the SSH I/O context and the connection pool.
 * Also used by the remote watch backend to reuse pooled connections. */
class SFTPProviderImpl : public std::enable_shared_from_this<SFTPProviderImpl> {
  public:
	/** Marks the current thread as a helper pool thread while a helper job runs, so the provider
	 * knows when its last reference is released on one of its own threads. */
	class HelperThreadScope {
	  public:
		HelperThreadScope();

		~HelperThreadScope();

		static bool active();
	};

	explicit SFTPProviderImpl( SFTPProviderConfig config );

	~SFTPProviderImpl();

	/** @return The pooled, ready connection for the URI authority (created on demand). */
	VFSTask<std::shared_ptr<SSHConnection>> connection( const URI& uri );

	ProviderCapabilities capabilities( const URI& uri ) const;

	void disconnectAll();

	size_t connectionCount() const;

	void setHostKeyVerifier( std::shared_ptr<SSHHostKeyVerifier> verifier );

	void setAuthenticationHandler( std::shared_ptr<SSHAuthenticationHandler> handler );

	VFSResult<SSHResolvedConfig> resolveForURI( const URI& uri ) const;

	const std::shared_ptr<System::ThreadPool>& helperPool() const { return mHelperPool; }

	const std::shared_ptr<SSHIOContext>& ioContext() const { return mContext; }

	const SFTPProviderConfig& config() const { return mConfig; }

	/** Pool key: the URI authority exactly as written (user@host:port). Connections are found
	 * by this key first, then by their effective configuration (SSHResolvedConfig::
	 * connectionKey()). */
	static std::string authorityKey( const URI& uri );

  private:
	struct PoolEntry {
		/** The I/O context owns live sessions; the pool must not retain their resources after
		 * the context closes them. An expired entry with waiters is still being built. */
		std::weak_ptr<SSHConnection> connection;
		std::vector<TaskResolver<VFSResult<std::shared_ptr<SSHConnection>>>> waiters;
	};

	VFSResult<SSHResolvedConfig> resolveSync( const std::string& host,
											  std::optional<std::string> user,
											  std::optional<Uint16> port ) const;

	struct Build;

	/** Connects to config through its ProxyJump chain or ProxyCommand. The task resolves on the
	 * I/O thread once the session is ready (authenticated, SFTP started). */
	VFSTask<std::shared_ptr<SSHConnection>> buildConnection( SSHResolvedConfig config );

	/** Sets up the next hop of build on a helper thread and continues when it is ready. */
	void buildHop( std::shared_ptr<Build> build );

	/** Creates the transport and connection of build's current hop (helper thread). */
	VFSResult<std::shared_ptr<SSHConnection>> startHop( Build& build );

	VFSTask<void> probeExtensions( const std::shared_ptr<SSHConnection>& connection );

	/** Hands the connection (or the failure) to every request waiting on the pool key. */
	void finishConnection( const std::string& key, const URI& uri, Uint64 generation,
						   VFSResult<std::shared_ptr<SSHConnection>>&& result );

	SSHConnectionHooks hooks() const;

	Time connectTimeout( const SSHResolvedConfig& config ) const;

	/** Closes idle and dead pooled connections (pool lock held). */
	void pruneIdle();

	SFTPProviderConfig mConfig;
	mutable std::mutex mMutex;
	std::shared_ptr<SSHIOContext> mContext;
	std::shared_ptr<System::ThreadPool> mHelperPool;
	mutable std::mutex mPoolMutex;
	std::map<std::string, PoolEntry> mPool;
	/** Invalidates connection builds still in flight when disconnectAll() is called. */
	Uint64 mPoolGeneration{ 0 };
};

}}} // namespace EE::VFS::sftp

#endif
