#ifndef EE_VFS_PERMISSIONSENGINE_HPP
#define EE_VFS_PERMISSIONSENGINE_HPP

#include <eepp/vfs/permissionchanges.hpp>
#include <eepp/vfs/provider.hpp>
#include <vector>

namespace EE { namespace VFS {

class VirtualFileSystem;

/** Streaming worker-side traversal for permission and ownership edits. */
class PermissionsEngine {
  public:
	static VFSResult<void> run( VirtualFileSystem& vfs, const std::vector<URI>& roots,
								const PermissionChanges& changes, CancellationToken token );
};

}} // namespace EE::VFS

#endif
