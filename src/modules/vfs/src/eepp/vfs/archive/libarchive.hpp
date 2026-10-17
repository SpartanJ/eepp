#ifndef EE_VFS_ARCHIVE_LIBARCHIVE_HPP
#define EE_VFS_ARCHIVE_LIBARCHIVE_HPP

#include "archiveinput.hpp"
#include <array>
#include <cstdint>
#include <ctime>
#include <eepp/vfs/providers/archiveprovider.hpp>
#if EE_PLATFORM != EE_PLATFORM_WIN
#include <sys/types.h>
#endif

struct archive;
struct archive_entry;

namespace EE { namespace VFS { namespace detail {

/** Private subset of libarchive 3.x's C ABI. No struct layout is imported. Reader callbacks use
 * eepp's UTF-8 file I/O and cancellation; all native resources die before the loader reference. */
class EE_VFS_API LibArchive {
  public:
#if EE_PLATFORM == EE_PLATFORM_WIN && !defined( _WIN64 )
	using Count = long;
#else
	using Count = std::intptr_t;
#endif
	using Offset = std::int64_t;
#if EE_PLATFORM == EE_PLATFORM_WIN
	using Mode = unsigned short;
#else
	using Mode = mode_t;
#endif
	using Enable = int ( * )( archive* );
	using Read = Count ( * )( archive*, void*, const void** );
	using Skip = Offset ( * )( archive*, void*, Offset );
	using Seek = Offset ( * )( archive*, void*, Offset, int );

	static VFSResult<std::shared_ptr<LibArchive>> load( const std::string& path, const URI& uri );

	LibArchive() = default;

	~LibArchive();

	LibArchive( const LibArchive& ) = delete;

	LibArchive& operator=( const LibArchive& ) = delete;

	ArchiveSupport support() const;

	VFSResult<void> configure( archive* reader, const URI& uri ) const;

	VFSError error( archive* reader, const URI& uri ) const;

	archive* ( *readNew )() = nullptr;
	int ( *readFree )( archive* ) = nullptr;
	int ( *readOpen )( archive*, void*, int ( * )( archive*, void* ), Read, Skip,
					   int ( * )( archive*, void* ) ) = nullptr;
	int ( *setSeek )( archive*, Seek ) = nullptr;
	int ( *nextHeader )( archive*, archive_entry** ) = nullptr;
	Count ( *readData )( archive*, void*, size_t ) = nullptr;
	const char* ( *errorString )( archive* ) = nullptr;
	int ( *errorNumber )( archive* ) = nullptr;
	int ( *formatCode )( archive* ) = nullptr;
	int ( *filterCount )( archive* ) = nullptr;
	int ( *filterCode )( archive*, int ) = nullptr;
	const char* ( *entryPath )( archive_entry* ) = nullptr;
	const char* ( *entryLink )( archive_entry* ) = nullptr;
	const char* ( *entryHardLink )( archive_entry* ) = nullptr;
	Offset ( *entrySize )( archive_entry* ) = nullptr;
	int ( *entrySizeSet )( archive_entry* ) = nullptr;
	Mode ( *entryType )( archive_entry* ) = nullptr;
	Mode ( *entryPermissions )( archive_entry* ) = nullptr;
	std::time_t ( *entryTime )( archive_entry* ) = nullptr;
	int ( *entryTimeSet )( archive_entry* ) = nullptr;

  private:
	bool bind();

	void* mLibrary = nullptr;
	int ( *mVersion )() = nullptr;
	const char* ( *mVersionString )() = nullptr;
	std::array<Enable, 11> mFormats{};
	std::array<Enable, 11> mFilters{};
};

class EE_VFS_API LibArchiveReader {
  public:
	LibArchiveReader( std::shared_ptr<LibArchive> library, const std::string& path, URI uri );

	~LibArchiveReader();

	LibArchiveReader( const LibArchiveReader& ) = delete;

	LibArchiveReader& operator=( const LibArchiveReader& ) = delete;

	VFSResult<void> open( CancellationToken token );

	VFSResult<bool> next( CancellationToken token );

	VFSResult<size_t> read( std::span<Uint8> data, CancellationToken token );

	void setURI( const URI& uri ) { mUri = uri; }

	/** Only retain the bounded decoders for plain/gzip TAR after a file closes. */
	bool canReuse() const;

	bool suspend() { return mFile.suspend(); }

	VFSResult<void> resume( CancellationToken token );

	const char* path() const { return mLibrary->entryPath( mEntry ); }

	const char* hardLink() const { return mLibrary->entryHardLink( mEntry ); }

	VFSResult<VFSFileInfo> info() const;

  private:
	static LibArchive::Count readCallback( archive*, void* self, const void** buffer );

	static LibArchive::Offset skipCallback( archive*, void* self, LibArchive::Offset amount );

	static LibArchive::Offset seekCallback( archive*, void* self, LibArchive::Offset offset,
											int whence );

	VFSError error() const;

	std::shared_ptr<LibArchive> mLibrary;
	ArchiveInput mFile;
	URI mUri;
	CancellationToken mToken;
	archive* mReader = nullptr;
	archive_entry* mEntry = nullptr;
	std::array<char, 16384> mBuffer;
};

}}} // namespace EE::VFS::detail
#endif
