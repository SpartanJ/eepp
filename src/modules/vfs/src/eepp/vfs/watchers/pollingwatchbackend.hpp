#ifndef EE_VFS_WATCHERS_POLLINGWATCHBACKEND_HPP
#define EE_VFS_WATCHERS_POLLINGWATCHBACKEND_HPP

#include <eepp/vfs/watch.hpp>
#include <memory>

namespace EE { namespace VFS {

class VirtualFileSystem;

namespace detail {
class PollingWatchBackendImpl;
}

/** @brief Provider-agnostic watch backend that periodically lists the watched directory through
 * the VFS and diffs snapshots.
 *
 * Used for schemes without an event-driven backend, and as the remote fallback when no
 * efsw-agent is available. Polling runs on one dedicated thread. Watches report
 * WatchStatus::Polling; transient listing failures switch them to Reconnecting and the next
 * successful poll emits RescanRequired. Recursive polling is limited to MaxEntries entries per
 * watch; larger trees degrade to the top directories that fit.
 */
class PollingWatchBackend : public WatchBackend {
  public:
	static constexpr size_t MaxEntries = 100000;

	explicit PollingWatchBackend( VirtualFileSystem& vfs );

	~PollingWatchBackend() override;

	VFSTask<std::unique_ptr<WatchRegistration>> watch( const URI& root, const WatchOptions& options,
													   std::shared_ptr<WatchSink> sink ) override;

  private:
	std::shared_ptr<detail::PollingWatchBackendImpl> mImpl;
};

}} // namespace EE::VFS

#endif
