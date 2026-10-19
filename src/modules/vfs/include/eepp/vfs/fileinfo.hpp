#ifndef EE_VFS_FILEINFO_HPP
#define EE_VFS_FILEINFO_HPP

#include <array>
#include <cstring>
#include <eepp/core/uri.hpp>
#include <eepp/vfs/config.hpp>
#include <initializer_list>
#include <optional>
#include <string>

namespace EE { namespace VFS {

enum class FileType : Uint8 {
	Unknown,
	Regular,
	Directory,
	Symlink,
	BlockDevice,
	CharacterDevice,
	FIFO,
	Socket,
};

EE_VFS_API const char* toString( FileType type );

/** @brief Opaque, provider-defined revision token.
 *
 * Used to detect external modification between reading and writing a file. Providers derive it
 * from whatever stable metadata they have (device/inode/mtime/size locally, mtime/size over SFTP).
 * Two revisions are only comparable when produced by the same provider. An empty revision means
 * "unknown" and never matches.
 *
 * Stored inline (no heap) because it travels inside every VFSFileInfo.
 */
class EE_VFS_API FileRevision {
  public:
	static constexpr size_t Capacity = 32;

	FileRevision() noexcept = default;

	/** Builds a revision from a list of 64-bit components (little-endian packed). Components
	 * beyond the capacity are ignored. */
	static FileRevision fromValues( std::initializer_list<Uint64> values ) noexcept {
		FileRevision rev;
		for ( Uint64 value : values ) {
			if ( rev.mSize + sizeof( Uint64 ) > Capacity )
				break;
			for ( size_t i = 0; i < sizeof( Uint64 ); ++i )
				rev.mData[rev.mSize++] = static_cast<Uint8>( value >> ( i * 8 ) );
		}
		return rev;
	}

	/** Builds a revision from raw bytes, truncated to the capacity. */
	static FileRevision fromBytes( const void* data, size_t size ) noexcept {
		FileRevision rev;
		rev.mSize = static_cast<Uint8>( size > Capacity ? Capacity : size );
		std::memcpy( rev.mData.data(), data, rev.mSize );
		return rev;
	}

	bool empty() const noexcept { return mSize == 0; }

	size_t size() const noexcept { return mSize; }

	const Uint8* data() const noexcept { return mData.data(); }

	/** @return True only for two non-empty, identical revisions. */
	bool matches( const FileRevision& other ) const noexcept { return !empty() && *this == other; }

	bool operator==( const FileRevision& other ) const noexcept {
		return mSize == other.mSize && std::memcmp( mData.data(), other.mData.data(), mSize ) == 0;
	}

	bool operator!=( const FileRevision& other ) const noexcept { return !( *this == other ); }

	/** @return Lowercase hexadecimal representation, for diagnostics and persistence. */
	std::string toHexString() const;

  private:
	std::array<Uint8, Capacity> mData{};
	Uint8 mSize{ 0 };
};

/** @brief Metadata snapshot of one filesystem entry. A pure value: constructing or copying it
 * never performs I/O.
 *
 * Optional metadata is tracked with a presence mask so that "unknown" stays distinguishable from a
 * valid zero, without paying std::optional padding for every field.
 *
 * The entry does not store its URI: listings hold thousands of entries and the caller always
 * knows the URI it listed or stat'ed. Use uriIn( directory ) for an entry of a listing.
 */
class EE_VFS_API VFSFileInfo {
  public:
	/** Last path segment (verbatim, not percent-encoded). */
	std::string name;
	/** Raw symlink target as stored by the filesystem (may be relative). Empty when not a link or
	 * unknown. Use symlinkTargetURI( uri ) for a resolved URI. */
	std::string symlinkTarget;
	std::string owner;
	std::string group;
	FileRevision revision;
	FileType type{ FileType::Unknown };
	/** For symlinks: the type of the final target when known, otherwise Unknown. */
	FileType targetType{ FileType::Unknown };
	bool hidden{ false };

	bool isDirectory() const noexcept {
		return type == FileType::Directory ||
			   ( type == FileType::Symlink && targetType == FileType::Directory );
	}

	bool isRegularFile() const noexcept {
		return type == FileType::Regular ||
			   ( type == FileType::Symlink && targetType == FileType::Regular );
	}

	bool isSymlink() const noexcept { return type == FileType::Symlink; }

	std::optional<Uint64> size() const { return get( Field::Size, mSize ); }

	std::optional<Int64> modifiedTime() const { return get( Field::Modified, mModified ); }

	std::optional<Int64> createdTime() const { return get( Field::Created, mCreated ); }

	std::optional<Int64> accessedTime() const { return get( Field::Accessed, mAccessed ); }

	std::optional<Uint32> permissions() const { return get( Field::Permissions, mPermissions ); }

	std::optional<Uint32> uid() const { return get( Field::Uid, mUid ); }

	std::optional<Uint32> gid() const { return get( Field::Gid, mGid ); }

	VFSFileInfo& setSize( Uint64 value ) { return set( Field::Size, mSize, value ); }

	/** Times are Unix timestamps in seconds. */
	VFSFileInfo& setModifiedTime( Int64 value ) { return set( Field::Modified, mModified, value ); }

	VFSFileInfo& setCreatedTime( Int64 value ) { return set( Field::Created, mCreated, value ); }

	VFSFileInfo& setAccessedTime( Int64 value ) { return set( Field::Accessed, mAccessed, value ); }

	/** POSIX permission bits (07777 range). */
	VFSFileInfo& setPermissions( Uint32 value ) {
		return set( Field::Permissions, mPermissions, value );
	}

	VFSFileInfo& setUid( Uint32 value ) { return set( Field::Uid, mUid, value ); }

	VFSFileInfo& setGid( Uint32 value ) { return set( Field::Gid, mGid, value ); }

	/** @return The URI of this entry inside the listed directory. */
	URI uriIn( const URI& directory ) const { return directory.withAppendedSegment( name ); }

	/** @return The symlink target resolved against the parent of linkURI (the URI of this
	 * entry), or nullopt when this is not a link with a known target. */
	std::optional<URI> symlinkTargetURI( const URI& linkURI ) const;

  private:
	enum class Field : Uint8 {
		Size = 1 << 0,
		Modified = 1 << 1,
		Created = 1 << 2,
		Accessed = 1 << 3,
		Permissions = 1 << 4,
		Uid = 1 << 5,
		Gid = 1 << 6,
	};

	template <typename T> std::optional<T> get( Field field, const T& value ) const {
		if ( mPresent & static_cast<Uint8>( field ) )
			return value;
		return std::nullopt;
	}

	template <typename T> VFSFileInfo& set( Field field, T& member, T value ) {
		member = value;
		mPresent |= static_cast<Uint8>( field );
		return *this;
	}

	Uint64 mSize{ 0 };
	Int64 mModified{ 0 };
	Int64 mCreated{ 0 };
	Int64 mAccessed{ 0 };
	Uint32 mPermissions{ 0 };
	Uint32 mUid{ 0 };
	Uint32 mGid{ 0 };
	Uint8 mPresent{ 0 };
};

}} // namespace EE::VFS

#endif
