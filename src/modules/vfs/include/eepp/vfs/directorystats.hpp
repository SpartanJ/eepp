#ifndef EE_VFS_DIRECTORYSTATS_HPP
#define EE_VFS_DIRECTORYSTATS_HPP

#include <eepp/core/small_unique_function.hpp>
#include <eepp/vfs/error.hpp>
#include <memory>
#include <optional>

namespace EE { namespace VFS {

/** Recursive totals. Hidden entries are included; symbolic links are counted as files but
 * contribute no bytes and their targets are never traversed. Directory metadata contributes
 * only to allocatedSize, including the selected root directories. Hard links contribute bytes
 * once when the provider reports file identities. */
struct EE_VFS_API DirectoryStats {
	Uint64 size{ 0 };
	/** Unavailable if any non-link entry lacks allocation metadata. Zero is a valid result. */
	std::optional<Uint64> allocatedSize{ 0 };
	Uint64 files{ 0 };
	/** Descendant directories; selected roots are excluded. */
	Uint64 directories{ 0 };
	Uint64 unreadableEntries{ 0 };
	Uint64 unknownSizes{ 0 };
	/** First traversal error; later errors increment unreadableEntries. Shared immutable storage
	 * keeps ordinary progress snapshots small and allocates diagnostics only on failure. */
	std::shared_ptr<const VFSError> firstError;

	bool incomplete() const { return unreadableEntries != 0 || unknownSizes != 0; }
};

/** Runs on the worker, at most every 200 ms plus the final snapshot (also on cancellation).
 * Dispatch owned snapshots to a UI executor before touching widgets. */
using DirectoryStatsProgressCallback = SmallUniqueFunction<void( const DirectoryStats& ), 48>;

}} // namespace EE::VFS

#endif
