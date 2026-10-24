#include "permissionsengine.hpp"
#include <eepp/core/small_vector.hpp>
#include <eepp/vfs/detail/await.hpp>
#include <eepp/vfs/virtualfilesystem.hpp>

namespace EE { namespace VFS {

using detail::await;

VFSResult<void> PermissionsEngine::run( VirtualFileSystem& vfs, const std::vector<URI>& roots,
										const PermissionChanges& changes,
										CancellationToken token ) {
	struct Frame {
		DirectoryHandle directory;
		DirectoryBatch batch;
		size_t index{ 0 };
		Uint32 mode;
	};
	SmallVector<Frame, 4> stack;
	auto apply = [&]( const URI& uri, Uint32 mode ) -> VFSResult<void> {
		if ( token.isCancellationRequested() )
			return Err( cancelledError( uri ) );
		if ( changes.uid || changes.gid ) {
			auto result = await( vfs.setOwnership( uri, changes.uid, changes.gid, token ) );
			if ( !result )
				return result;
		}
		// chown may clear set-ID bits. Apply the requested mode afterwards.
		if ( changes.mask ) {
			return await( vfs.setPermissions(
				uri, ( mode & ~changes.mask ) | ( changes.value & changes.mask ), token ) );
		}
		return Ok();
	};
	auto visit = [&]( const URI& uri ) -> VFSResult<void> {
		if ( token.isCancellationRequested() )
			return Err( cancelledError( uri ) );
		auto info = await( vfs.stat( uri, StatOptions{ false }, token ) );
		if ( !info )
			return Err( std::move( info ).error() );
		if ( info.value().isSymlink() )
			return Ok();
		if ( changes.mask && !info.value().permissions() ) {
			return Err(
				VFSError( VFSErrorCode::Unsupported, uri, "Permission metadata is unavailable" ) );
		}
		const Uint32 mode = info.value().permissions().value_or( 0 );
		if ( changes.recursive && info.value().type == FileType::Directory ) {
			// Grant only requested read/search additions before opening the directory. Keep
			// existing access and ownership until its contents are processed, so a change
			// can both unlock unreadable directories and remove access without blocking itself.
			const Uint32 traversalMode = mode | ( changes.mask & changes.value & 0555 );
			if ( traversalMode != mode ) {
				if ( token.isCancellationRequested() )
					return Err( cancelledError( uri ) );
				auto granted = await( vfs.setPermissions( uri, traversalMode, token ) );
				if ( !granted )
					return granted;
			}
			auto opened = await( vfs.openDirectory(
				uri, DirectoryOpenOptions{ DirectoryMetadata::NamesAndTypes, false }, token ) );
			if ( !opened )
				return Err( std::move( opened ).error() );
			stack.push_back( { std::move( opened ).value(), {}, 0, mode } );
			return Ok();
		}
		return apply( uri, mode );
	};
	for ( const auto& root : roots ) {
		auto result = visit( root );
		if ( !result )
			return result;
		while ( !stack.empty() ) {
			if ( token.isCancellationRequested() )
				return Err( cancelledError( root ) );
			auto& frame = stack.back();
			if ( frame.index < frame.batch.entries.size() ) {
				const auto& entry = frame.batch.entries[frame.index++];
				if ( entry.isSymlink() )
					continue;
				const URI child = entry.uriIn( frame.directory.uri() );
				// Re-stat before applying: listing metadata can be stale. Observed links
				// are skipped, but this pathname traversal is not an atomic tree snapshot.
				result = visit( child );
				if ( !result )
					return result;
			} else if ( frame.batch.endOfDirectory ) {
				auto finished = std::move( frame );
				stack.pop_back();
				result = await( finished.directory.close() );
				if ( !result )
					return result;
				result = apply( finished.directory.uri(), finished.mode );
				if ( !result )
					return result;
			} else {
				auto batch = await( frame.directory.read( 256, token ) );
				if ( !batch )
					return Err( std::move( batch ).error() );
				frame.batch = std::move( batch ).value();
				frame.index = 0;
			}
		}
	}
	return Ok();
}

}} // namespace EE::VFS
