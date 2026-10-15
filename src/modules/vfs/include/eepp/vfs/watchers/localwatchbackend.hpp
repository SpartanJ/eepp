#ifndef EE_VFS_WATCHERS_LOCALWATCHBACKEND_HPP
#define EE_VFS_WATCHERS_LOCALWATCHBACKEND_HPP

#include <eepp/system/threadpool.hpp>
#include <eepp/vfs/watch.hpp>
#include <memory>

namespace EE { namespace VFS {

namespace detail {
class LocalWatchBackendImpl;
}

/** @brief `file://` watch backend built on efsw.
 *
 * Each watch first tries the native efsw watcher. When efsw reports the directory as remote
 * (FileRemote, for example NFS mounts) or the native watcher cannot represent it, the watch is
 * served by efsw's generic polling watcher and reports WatchStatus::Polling. Missed native events
 * (efsw handleMissedFileActions) become RescanRequired.
 *
 * Several logical watches on the same directory share one efsw watch.
 *
 * Registration and removal touch the filesystem (the generic watcher takes a snapshot of the
 * tree), so they run on the I/O pool, never on the caller's thread.
 */
class LocalWatchBackend : public WatchBackend {
  public:
	/** @param pool Where watches are registered and removed.
	 * @param genericPollingInterval Polling interval for the generic efsw watcher. */
	explicit LocalWatchBackend( std::shared_ptr<System::ThreadPool> pool,
								Time genericPollingInterval = System::Seconds( 1 ) );

	~LocalWatchBackend() override;

	VFSTask<std::unique_ptr<WatchRegistration>> watch( const URI& root, const WatchOptions& options,
													   std::shared_ptr<WatchSink> sink ) override;

  private:
	std::shared_ptr<System::ThreadPool> mPool;
	std::shared_ptr<detail::LocalWatchBackendImpl> mImpl;
};

}} // namespace EE::VFS

#endif
