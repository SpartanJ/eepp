#ifndef EE_VFS_SFTP_REMOTEWATCHBACKEND_HPP
#define EE_VFS_SFTP_REMOTEWATCHBACKEND_HPP

#include <eepp/vfs/watch.hpp>
#include <memory>

namespace EE { namespace VFS {

class VirtualFileSystem;

namespace sftp {
class SFTPProviderImpl;
class RemoteWatchBackendImpl;
} // namespace sftp

/** @brief `sftp://` watch backend using efsw-agent over the pooled SSH connection.
 *
 * One agent process per SSH connection is started through an exec channel
 * (SFTPProviderConfig::watchAgentCommand) and multiplexes every watch of that connection. The
 * agent uses the native watcher of the remote host, or efsw's generic polling watcher there for
 * network mounts, so remote polling never travels over SSH.
 *
 * Logical watches outlive their agent: when the agent dies or the SSH connection drops they report
 * WatchStatus::Reconnecting, are re-registered on a new agent and then emit RescanRequired. When
 * the agent cannot be started (not installed) watches fall back to polling through the VFS and
 * report WatchStatus::Polling. SFTP operations never depend on the agent.
 */
class RemoteWatchBackend : public WatchBackend {
  public:
	RemoteWatchBackend( std::shared_ptr<sftp::SFTPProviderImpl> provider, VirtualFileSystem& vfs );

	RemoteWatchBackend( std::shared_ptr<sftp::SFTPProviderImpl> provider, VirtualFileSystem& vfs,
						Time establishmentTimeout );

	~RemoteWatchBackend() override;

	VFSTask<std::unique_ptr<WatchRegistration>> watch( const URI& root, const WatchOptions& options,
													   std::shared_ptr<WatchSink> sink ) override;

  private:
	std::shared_ptr<sftp::RemoteWatchBackendImpl> mImpl;
};

}} // namespace EE::VFS

#endif
