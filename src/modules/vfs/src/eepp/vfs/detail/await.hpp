#ifndef EE_VFS_DETAIL_AWAIT_HPP
#define EE_VFS_DETAIL_AWAIT_HPP

#include <eepp/vfs/error.hpp>

namespace EE { namespace VFS { namespace detail {

/** Blocks on a VFS task and returns its result, mapping abandonment to an error.
 *
 * For VFS orchestration jobs and synchronous IOStreamVFS calls on independent workers. Never call
 * it on the UI thread or on a thread that the awaited provider needs to make progress.
 */
template <typename T> VFSResult<T> await( VFSTask<T>&& task ) {
	auto value = std::move( task ).get();
	if ( !value )
		return Err( VFSError( VFSErrorCode::Unknown, URI(), "Operation was abandoned" ) );
	return std::move( *value );
}

}}} // namespace EE::VFS::detail

#endif
