#ifndef EE_VFS_BATCHRENAMEENGINE_HPP
#define EE_VFS_BATCHRENAMEENGINE_HPP

#include <eepp/core/cancellationtoken.hpp>
#include <eepp/vfs/batchrename.hpp>

namespace EE { namespace VFS {

class VirtualFileSystem;

/** Plans and preflights a sibling batch before executing provider renames on a worker. */
class BatchRenameEngine {
  public:
	static BatchRenameResult run( VirtualFileSystem& vfs, const URI& directory,
								  std::vector<BatchRenameItem> items,
								  const BatchRenamePattern& pattern, CancellationToken token );
};

}} // namespace EE::VFS

#endif
