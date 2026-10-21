#ifndef EE_VFS_DIRECTORYSTATSENGINE_HPP
#define EE_VFS_DIRECTORYSTATSENGINE_HPP

#include <eepp/core/cancellationtoken.hpp>
#include <eepp/vfs/directorystats.hpp>
#include <vector>

namespace EE { namespace VFS {

class VirtualFileSystem;

/** Bounded-batch traversal on the orchestration pool, never the caller's UI thread. */
class DirectoryStatsEngine {
  public:
	static VFSResult<DirectoryStats> run( VirtualFileSystem& vfs, const std::vector<URI>& roots,
										  DirectoryStatsProgressCallback onProgress,
										  CancellationToken token );
};

}} // namespace EE::VFS

#endif
