#ifndef EE_VFS_ARCHIVE_ARCHIVEINPUT_HPP
#define EE_VFS_ARCHIVE_ARCHIVEINPUT_HPP

#include <cstdint>
#include <cstdio>
#include <eepp/vfs/config.hpp>
#include <string>

namespace EE { namespace VFS { namespace detail {

/** Archive input needs checked 64-bit seeks and, on Windows, sharing that allows replacement
 * while an active or idle reader keeps the old file open. It never exposes a CRT file to
 * libarchive. */
class EE_VFS_API ArchiveInput {
  public:
	explicit ArchiveInput( const std::string& path );

	~ArchiveInput();

	ArchiveInput( const ArchiveInput& ) = delete;

	ArchiveInput& operator=( const ArchiveInput& ) = delete;

	bool isOpen() const { return mFile != nullptr; }

	std::intptr_t read( char* data, size_t size );

	Int64 seek( Int64 position );

	Int64 tell() const;

	Int64 size() const { return mSize; }

	bool suspend();

	bool resume();

  private:
	bool openFile();

	std::string mPath;
	std::FILE* mFile = nullptr;
	Int64 mSize = 0;
	Int64 mPosition = 0;
};

}}} // namespace EE::VFS::detail
#endif
