#ifndef EE_VFS_DIRECTORYHANDLE_HPP
#define EE_VFS_DIRECTORYHANDLE_HPP

#include <eepp/core/cancellationtoken.hpp>
#include <eepp/vfs/config.hpp>
#include <eepp/vfs/error.hpp>
#include <eepp/vfs/fileinfo.hpp>
#include <memory>
#include <vector>

namespace EE { namespace VFS {

/** One chunk of a directory enumeration.
 *
 * endOfDirectory is explicit: an empty batch does not imply EOF, and the final batch may carry
 * entries. "." and ".." are never reported.
 */
struct EE_VFS_API DirectoryBatch {
	std::vector<VFSFileInfo> entries;
	bool endOfDirectory{ false };
};

/** Provider implementation behind a DirectoryHandle. Implementations must tolerate close() while
 * a read is pending and must release native resources in their destructor if close() was never
 * called. Calls are serialized by the handle's single consumer; implementations need not support
 * concurrent read() calls. */
class EE_VFS_API DirectoryHandleImpl {
  public:
	virtual ~DirectoryHandleImpl() = default;

	virtual VFSTask<DirectoryBatch> read( size_t maxEntries, CancellationToken token ) = 0;

	virtual VFSTask<void> close() = 0;
};

/** @brief Incremental directory enumeration.
 *
 * Move-only. Destroying an open handle releases it without waiting. read() after EOF returns an
 * empty batch with endOfDirectory set.
 */
class EE_VFS_API DirectoryHandle {
  public:
	DirectoryHandle() = default;

	DirectoryHandle( URI uri, std::shared_ptr<DirectoryHandleImpl> impl ) :
		mUri( std::move( uri ) ), mImpl( std::move( impl ) ) {}

	DirectoryHandle( DirectoryHandle&& ) noexcept = default;

	DirectoryHandle& operator=( DirectoryHandle&& ) noexcept = default;

	DirectoryHandle( const DirectoryHandle& ) = delete;

	DirectoryHandle& operator=( const DirectoryHandle& ) = delete;

	VFSTask<DirectoryBatch> read( size_t maxEntries = 256, CancellationToken token = {} ) {
		if ( !mImpl ) {
			return readyError<DirectoryBatch>( VFSErrorCode::InvalidOperation, mUri,
											   "Directory handle is closed" );
		}
		return mImpl->read( maxEntries == 0 ? 1 : maxEntries, std::move( token ) );
	}

	VFSTask<void> close() {
		auto impl = std::move( mImpl );
		if ( !impl )
			return readyValue<void>();
		return impl->close();
	}

	const URI& uri() const noexcept { return mUri; }

	bool isOpen() const noexcept { return nullptr != mImpl; }

  private:
	URI mUri;
	std::shared_ptr<DirectoryHandleImpl> mImpl;
};

}} // namespace EE::VFS

#endif
