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
};

}} // namespace EE::VFS

#endif
