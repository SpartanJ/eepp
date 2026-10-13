#ifndef EE_VFS_IOSTREAMVFS_HPP
#define EE_VFS_IOSTREAMVFS_HPP

#include <eepp/system/iostream.hpp>
#include <eepp/vfs/virtualfilesystem.hpp>
#include <optional>

namespace EE { namespace VFS {

/** @brief Synchronous System::IOStream over a VFS file, for existing eepp loaders and decoders.
 *
 * Every call blocks until the provider answers, so this adapter is only for worker threads:
 * never use it on the UI thread or on a thread a provider needs to make progress (for local files
 * that is the VFS I/O pool, for sftp:// the SSH I/O thread).
 *
 * Reads go through a small read-ahead buffer so byte-oriented decoders do not pay one provider
 * round trip per call.
 */
class IOStreamVFS : public System::IOStream {
  public:
	/** Opens uri (blocking). Check isOpen() / error() afterwards. */
	IOStreamVFS( VirtualFileSystem& vfs, const URI& uri, OpenFlags flags = OpenFlags::Read,
				 size_t readAheadSize = 64 * 1024 );

	~IOStreamVFS() override;

	ios_size read( char* data, ios_size size ) override;

	ios_size write( const char* data, ios_size size ) override;

	ios_size seek( ios_size position ) override;

	ios_size tell() override { return mPosition; }

	ios_size getSize() override;

	bool isOpen() override { return mHandle.isOpen(); }

	/** The last error, if any operation failed. */
	const std::optional<VFSError>& error() const { return mError; }

	/** Closes the handle (blocking). */
	void close();

  private:
	FileHandle mHandle;
	std::vector<Uint8> mBuffer;
	std::optional<VFSError> mError;
	std::optional<Uint64> mSize;
	Uint64 mPosition{ 0 };
	/** File offset of mBuffer[0]. */
	Uint64 mBufferOffset{ 0 };
	size_t mBufferLength{ 0 };
	size_t mReadAhead;
};

}} // namespace EE::VFS

#endif
