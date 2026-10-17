#ifndef EE_VFS_OPTIONS_HPP
#define EE_VFS_OPTIONS_HPP

#include <eepp/config.hpp>
#include <eepp/vfs/fileinfo.hpp>
#include <optional>

namespace EE { namespace VFS {

struct StatOptions {
	/** When false the entry itself is described (lstat semantics). */
	bool followSymlinks{ true };
};

/** How much metadata a directory listing must produce. */
enum class DirectoryMetadata : Uint8 {
	/** Name and type only. Avoids any per-entry stat on providers where the type is free. */
	NamesAndTypes,
	/** Everything the provider can report for each entry. */
	Full,
};

struct DirectoryOpenOptions {
	DirectoryMetadata metadata{ DirectoryMetadata::Full };
	/** Resolve the target type of symlinks (VFSFileInfo::targetType). May cost a stat per link. */
	bool resolveSymlinkTargets{ true };
};

/** Open mode flags for files. */
enum class OpenFlags : Uint8 {
	None = 0,
	Read = 1 << 0,
	Write = 1 << 1,
	/** Create the file if it does not exist. */
	Create = 1 << 2,
	/** Fail with AlreadyExists if the file exists (with Create). */
	Exclusive = 1 << 3,
	/** Truncate an existing file to zero length. */
	Truncate = 1 << 4,
	ReadWrite = Read | Write,
};

constexpr OpenFlags operator|( OpenFlags a, OpenFlags b ) noexcept {
	return static_cast<OpenFlags>( static_cast<Uint8>( a ) | static_cast<Uint8>( b ) );
}

constexpr OpenFlags operator&( OpenFlags a, OpenFlags b ) noexcept {
	return static_cast<OpenFlags>( static_cast<Uint8>( a ) & static_cast<Uint8>( b ) );
}

constexpr bool hasFlag( OpenFlags flags, OpenFlags flag ) noexcept {
	return ( static_cast<Uint8>( flags ) & static_cast<Uint8>( flag ) ) != 0;
}

struct OpenOptions {
	OpenFlags flags{ OpenFlags::Read };
	/** Permissions for newly created files; provider default (0666 & ~umask) when unset. */
	std::optional<Uint32> createPermissions;

	OpenOptions() = default;

	OpenOptions( OpenFlags flags ) : flags( flags ) {}
};

struct MkDirOptions {
	std::optional<Uint32> permissions;
};

struct RemoveOptions {
	/** VFS-level only: remove directory contents recursively. Providers receive false. */
	bool recursive{ false };
};

struct RenameOptions {
	/** Replace an existing destination. Atomic when the provider reports AtomicReplace. */
	bool overwrite{ false };
};

struct WriteOptions {
	/** Write a sibling temporary file and atomically rename it over the destination. */
	bool atomic{ true };
	/** When atomic replacement is unsupported, fall back to an in-place write instead of failing
	 * with Unsupported. */
	bool allowNonAtomicFallback{ false };
	/** Flush to stable storage before the final rename, where supported. */
	bool fsync{ true };
	/** Create missing parent directories. */
	bool createParents{ false };
	/** Fail with ChangedExternally when the current destination revision differs. An expected
	 * revision on a missing destination also fails. */
	std::optional<FileRevision> expectedRevision;
	/** Permissions for a newly created destination. Existing destinations keep their mode. */
	std::optional<Uint32> permissions;
};

enum class OverwritePolicy : Uint8 {
	/** Fail with AlreadyExists. */
	Fail,
	/** Replace existing files (directories are merged). */
	Overwrite,
	/** Leave existing destination files untouched and continue. */
	Skip,
};

struct CopyOptions {
	OverwritePolicy overwrite{ OverwritePolicy::Fail };
	bool recursive{ true };
	/** Copy permission bits when both sides support them. */
	bool preservePermissions{ true };
	/** Chunk size used by the generic cross-provider copy loop. */
	Uint32 chunkSize{ 256 * 1024 };
};

}} // namespace EE::VFS

#endif
