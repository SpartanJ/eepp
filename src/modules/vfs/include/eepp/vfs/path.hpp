#ifndef EE_VFS_PATH_HPP
#define EE_VFS_PATH_HPP

#include <eepp/core/uri.hpp>
#include <eepp/vfs/config.hpp>
#include <string_view>

namespace EE { namespace VFS {

/** Validation at provider and user-input boundaries. These checks perform no I/O. */
class EE_VFS_API VFSPath {
  public:
	/** Reject embedded NULs in decoded paths, hosts and user information before native calls. */
	static bool validURI( const URI& uri );

	/** A single, nonempty basename, excluding dot entries, separators and NULs. Local Windows
	 * destinations additionally reject reserved characters, device names and trailing dots/spaces.
	 * Remote and resource names retain POSIX rules even on a Windows client. */
	static bool validName( std::string_view name, const URI& directory );
};

}} // namespace EE::VFS

#endif
