#ifndef EE_VFS_CAPABILITIES_HPP
#define EE_VFS_CAPABILITIES_HPP

#include <eepp/vfs/config.hpp>

namespace EE { namespace VFS {

/** Individual provider capabilities. Watching is intentionally absent: it is a WatchManager
 * service, not a provider operation. */
enum class VFSCapability : Uint32 {
	Read = 1u << 0,
	Write = 1u << 1,
	List = 1u << 2,
	Create = 1u << 3,
	Remove = 1u << 4,
	Rename = 1u << 5,
	/** remove(recursive=true) traverses opened directories without following swapped links. */
	RecursiveRemove = 1u << 6,

	Seek = 1u << 7,
	RandomAccess = 1u << 8,
	Truncate = 1u << 9,

	Symlinks = 1u << 10,
	HardLinks = 1u << 11,
	Permissions = 1u << 12,
	Ownership = 1u << 13,

	AtomicRename = 1u << 14,
	/** rename() with overwrite replaces the destination atomically. */
	AtomicReplace = 1u << 15,
	Fsync = 1u << 16,

	FreeSpace = 1u << 17,
	/** Paths differ only by case are distinct entries. */
	CaseSensitive = 1u << 18,
};

/** Set of VFSCapability flags. Capabilities may vary per URI (for example per SFTP server), so
 * providers report them per URI. */
class EE_VFS_API ProviderCapabilities {
  public:
	constexpr ProviderCapabilities() noexcept = default;

	constexpr ProviderCapabilities( VFSCapability capability ) noexcept :
		mFlags( static_cast<Uint32>( capability ) ) {}

	constexpr explicit ProviderCapabilities( Uint32 flags ) noexcept : mFlags( flags ) {}

	constexpr bool has( VFSCapability capability ) const noexcept {
		return ( mFlags & static_cast<Uint32>( capability ) ) != 0;
	}

	constexpr bool hasAll( ProviderCapabilities other ) const noexcept {
		return ( mFlags & other.mFlags ) == other.mFlags;
	}

	constexpr ProviderCapabilities& set( VFSCapability capability, bool enabled = true ) noexcept {
		if ( enabled )
			mFlags |= static_cast<Uint32>( capability );
		else
			mFlags &= ~static_cast<Uint32>( capability );
		return *this;
	}

	constexpr Uint32 flags() const noexcept { return mFlags; }

	constexpr ProviderCapabilities operator|( ProviderCapabilities other ) const noexcept {
		return ProviderCapabilities( mFlags | other.mFlags );
	}

	constexpr ProviderCapabilities operator&( ProviderCapabilities other ) const noexcept {
		return ProviderCapabilities( mFlags & other.mFlags );
	}

	constexpr bool operator==( const ProviderCapabilities& other ) const noexcept = default;

  private:
	Uint32 mFlags{ 0 };
};

constexpr ProviderCapabilities operator|( VFSCapability a, VFSCapability b ) noexcept {
	return ProviderCapabilities( a ) | ProviderCapabilities( b );
}

}} // namespace EE::VFS

#endif
