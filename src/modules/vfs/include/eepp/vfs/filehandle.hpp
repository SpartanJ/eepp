#ifndef EE_VFS_FILEHANDLE_HPP
#define EE_VFS_FILEHANDLE_HPP

#include <eepp/core/cancellationtoken.hpp>
#include <eepp/vfs/capabilities.hpp>
#include <eepp/vfs/error.hpp>
#include <eepp/vfs/fileinfo.hpp>
#include <memory>
#include <span>

namespace EE { namespace VFS {

/** Provider implementation behind a FileHandle.
 *
 * Buffers passed to read()/write() are owned by the caller and must stay valid until the returned
 * task completes. Implementations must release native resources in their destructor if close()
 * was never called.
 */
class FileHandleImpl {
  public:
	virtual ~FileHandleImpl() = default;

	virtual VFSTask<size_t> read( Uint64 offset, std::span<Uint8> data,
								  CancellationToken token ) = 0;

	virtual VFSTask<size_t> write( Uint64 offset, std::span<const Uint8> data,
								   CancellationToken token ) = 0;

	virtual VFSTask<void> truncate( Uint64 size ) = 0;

	/** Flushes buffered data and, where supported (VFSCapability::Fsync), syncs it to stable
	 * storage. */
	virtual VFSTask<void> flush() = 0;

	virtual VFSTask<FileInfo> stat() = 0;

	virtual VFSTask<void> close() = 0;

	virtual ProviderCapabilities capabilities() const = 0;
};

/** @brief Open file with explicit-offset I/O.
 *
 * Offset-based operations avoid a shared mutable seek position, so independent reads can be
 * pipelined. read() returning 0 bytes for a non-empty buffer means end of file.
 */
class FileHandle {
  public:
	FileHandle() = default;

	FileHandle( URI uri, std::shared_ptr<FileHandleImpl> impl ) :
		mUri( std::move( uri ) ), mImpl( std::move( impl ) ) {}

	FileHandle( FileHandle&& ) noexcept = default;

	FileHandle& operator=( FileHandle&& ) noexcept = default;

	FileHandle( const FileHandle& ) = delete;

	FileHandle& operator=( const FileHandle& ) = delete;

	VFSTask<size_t> read( Uint64 offset, std::span<Uint8> data, CancellationToken token = {} ) {
		if ( !mImpl )
			return closedError<size_t>();
		return mImpl->read( offset, data, std::move( token ) );
	}

	VFSTask<size_t> write( Uint64 offset, std::span<const Uint8> data,
						   CancellationToken token = {} ) {
		if ( !mImpl )
			return closedError<size_t>();
		return mImpl->write( offset, data, std::move( token ) );
	}

	VFSTask<void> truncate( Uint64 size ) {
		if ( !mImpl )
			return closedError<void>();
		return mImpl->truncate( size );
	}

	VFSTask<void> flush() {
		if ( !mImpl )
			return closedError<void>();
		return mImpl->flush();
	}

	VFSTask<FileInfo> stat() {
		if ( !mImpl )
			return closedError<FileInfo>();
		return mImpl->stat();
	}

	VFSTask<void> close() {
		auto impl = std::move( mImpl );
		if ( !impl )
			return readyValue<void>();
		return impl->close();
	}

	ProviderCapabilities capabilities() const {
		return mImpl ? mImpl->capabilities() : ProviderCapabilities();
	}

	const URI& uri() const noexcept { return mUri; }

	bool isOpen() const noexcept { return nullptr != mImpl; }

  private:
	template <typename T> VFSTask<T> closedError() const {
		return readyError<T>( VFSErrorCode::InvalidOperation, mUri, "File handle is closed" );
	}

	URI mUri;
	std::shared_ptr<FileHandleImpl> mImpl;
};

}} // namespace EE::VFS

#endif
