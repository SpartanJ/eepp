#ifndef EE_VFS_SFTP_SFTPPROVIDERIMPL_HPP
#define EE_VFS_SFTP_SFTPPROVIDERIMPL_HPP

#include "sshsession.hpp"
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

	/** Pool key: the URI authority exactly as written (user@host:port). */
	static std::string authorityKey( const URI& uri );

  private:
	struct PoolEntry {
		std::shared_ptr<SSHConnection> connection;
		std::vector<TaskSource<VFSResult<std::shared_ptr<SSHConnection>>>> waiters;
	};

	VFSResult<SSHResolvedConfig> resolveSync( const std::string& host,
											  std::optional<std::string> user,
											  std::optional<Uint16> port ) const;

	VFSResult<std::shared_ptr<SSHConnection>> buildConnection( const SSHResolvedConfig& config );

	void probeExtensions( const std::shared_ptr<SSHConnection>& connection );

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
};

}}} // namespace EE::VFS::sftp

#endif
