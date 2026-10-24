#ifndef EE_VFS_IOSTREAMVFS_HPP
#define EE_VFS_IOSTREAMVFS_HPP

#include <eepp/system/iostream.hpp>
#include <eepp/vfs/filehandle.hpp>
#include <eepp/vfs/options.hpp>
#include <limits>
#include <optional>
#include <vector>

namespace EE { namespace VFS {

/** An owned FileHandle exposed as a synchronous IOStream for existing loaders.
 *
 * read(), write(), flush(), close(), and create() may wait for provider I/O. Run them on a
 * loader/application worker, never the UI thread or a worker needed by that provider. Opening
 * through VirtualFileSystem::openStream() performs the initial work asynchronously.
 *
 * Small reads use a lazily allocated 64 KiB cache; large reads/writes use bounded 256 KiB provider
 * requests. No whole-file buffer is allocated. Reads fill the requested range up to the size
 * reported at open, or return a partial count on failure. Failure is sticky: isOpen() becomes false
 * and error() retains the cause. EOF and failures both return zero when no bytes were read.
 *
 * A stream has one mutable position and is not thread-safe. Transfer ownership between loaders;
 * do not use it concurrently. Destruction schedules handle cleanup without waiting for it.
 */
class EE_VFS_API IOStreamVFS : public System::IOStream {
  public:
	static constexpr Uint64 Unlimited = static_cast<Uint64>( std::numeric_limits<ios_size>::max() );

	/** Adapts an already opened handle. Requires a known, representable length; maxSize checks
	 * the initial length. Cancellation is checked even when a read can be served from the cache. */
	static VFSResult<std::unique_ptr<IOStreamVFS>> create( FileHandle handle,
														   OpenFlags flags = OpenFlags::Read,
														   CancellationToken token = {},
														   Uint64 maxSize = Unlimited );

	~IOStreamVFS();

	ios_size read( char* data, ios_size size );

	ios_size write( const char* data, ios_size size );

	ios_size seek( ios_size position );

	ios_size tell();

	/** Cached length at open, extended by this stream's writes. */
	ios_size getSize();

	bool isOpen();

	const std::optional<VFSError>& error() const { return mError; }

	VFSResult<void> flush();

	VFSResult<void> close();

  private:
	IOStreamVFS( FileHandle handle, ios_size size, OpenFlags flags, CancellationToken token );

	bool check();

	void fail( VFSError error );

	FileHandle mHandle;
	CancellationToken mToken;
	std::optional<VFSError> mError;
	std::vector<Uint8> mBuffer;
	ios_size mSize;
	ios_size mPosition{ 0 };
	ios_size mBufferOffset{ 0 };
	size_t mBufferValid{ 0 };
	OpenFlags mFlags;
};

}} // namespace EE::VFS

#endif
