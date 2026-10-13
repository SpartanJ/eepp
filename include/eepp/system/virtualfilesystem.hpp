#ifndef EE_VIRTUALFILESYSTEM_HPP
#define EE_VIRTUALFILESYSTEM_HPP

#include <eepp/system/packfileindex.hpp>

namespace EE { namespace System {

/** @deprecated The pack path index was renamed to PackFileIndex. The URI-based virtual file system
 * lives in the eepp-vfs module (EE::VFS::VirtualFileSystem), where packs are served by the
 * `res://` scheme. This alias is kept temporarily for source compatibility. */
using VirtualFileSystem [[deprecated( "Use EE::System::PackFileIndex or the eepp-vfs res:// "
									  "provider" )]] = PackFileIndex;

/** @deprecated Use PackFileIndex::instance(). */
class [[deprecated( "Use EE::System::PackFileIndex::instance()" )]] VFS {
  public:
	static PackFileIndex* instance() { return PackFileIndex::instance(); }
};

}} // namespace EE::System

#endif
