#ifndef EE_VFS_PERMISSIONCHANGES_HPP
#define EE_VFS_PERMISSIONCHANGES_HPP

#include <eepp/vfs/config.hpp>
#include <optional>

namespace EE { namespace VFS {

/** Only masked permission bits change; all other bits and unspecified ownership IDs survive.
 * Recursive changes skip symbolic links and grant requested directory read/search additions
 * before traversal. Final directory modes and ownership are applied after their contents. */
struct EE_VFS_API PermissionChanges {
	std::optional<Uint32> uid;
	std::optional<Uint32> gid;
	Uint32 mask{ 0 };
	Uint32 value{ 0 };
	bool recursive{ false };
	/** For files, grant execute only where read is granted, or clear all execute bits.
	 * An unset value preserves the execute bits selected by mask/value. Directories are unaffected.
	 */
	std::optional<bool> executable;

	Uint32 modeFor( Uint32 mode, bool isDirectory ) const {
		mode = ( mode & ~mask ) | ( value & mask );
		if ( executable && !isDirectory )
			mode = ( mode & ~0111u ) | ( *executable ? ( mode & 0444u ) >> 2 : 0 );
		return mode;
	}
};

}} // namespace EE::VFS

#endif
