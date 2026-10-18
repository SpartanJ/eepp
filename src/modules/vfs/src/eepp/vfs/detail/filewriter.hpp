#ifndef EE_VFS_DETAIL_FILEWRITER_HPP
#define EE_VFS_DETAIL_FILEWRITER_HPP

#include <eepp/vfs/detail/await.hpp>
#include <eepp/vfs/filehandle.hpp>
#include <eepp/vfs/provider.hpp>
#include <optional>
#include <span>
#include <string>

namespace EE { namespace VFS { namespace detail {

/** Blocking file-writing steps shared by the orchestrated VFS operations and the transfer engine.
 * Only for jobs that may block (see await()). */
class FileWriter {
  public:
	/** Chunk used by readAll()/writeAll() and the generic copy loop. */
	static constexpr size_t IOChunkSize = 256 * 1024;

	struct ReplaceOptions {
		/** Mode of a newly created file (the provider filters it through the umask). */
		std::optional<Uint32> createPermissions;
		/** Exact mode to give the new version (for example the replaced file's), applied after it
		 * is written: creation modes are filtered through the umask. */
		std::optional<Uint32> exactPermissions;
		bool fsync{ false };
		/** Replace through a temporary sibling. When false, target is rewritten in place. */
		bool atomic{ true };
		/** Without atomic replacement support, rewrite target in place instead of failing. */
		bool allowInPlace{ false };
		/** Fail if another entry appeared at target while the temporary was being written. */
		bool overwrite{ true };
	};

	/** Writes every byte of data through handle, starting at offset. */
	static VFSResult<void> writeFully( FileHandle& handle, std::span<const Uint8> data,
									   const CancellationToken& token, Uint64 offset = 0 );

	/** Creates uri and every missing parent. Orchestration jobs call it directly: waiting on a
	 * VirtualFileSystem::makeDirs() task from the orchestration pool would wait on a job queued
	 * behind the waiting one. */
	static VFSResult<void> makeDirs( VFSProvider& provider, const URI& uri,
									 const MkDirOptions& options, const CancellationToken& token );

	/** @return The file a write to uri replaces: the target of a symlink (so the link survives the
	 * replacement), otherwise uri itself. */
	static VFSResult<URI> resolveTarget( VFSProvider& provider, const URI& uri,
										 const CancellationToken& token );

	/** Writes a new version of target with write( FileHandle& ). The data goes to a temporary
	 * sibling that replaces target only once write() and beforeReplace() succeeded, so a failure
	 * never damages the current version, nor another name of the same file (a hard link to it).
	 * Non-atomic writes, and providers without atomic replacement (when allowed), rewrite target
	 * in place. */
	template <typename Write, typename BeforeReplace>
	static VFSResult<void> replace( VFSProvider& provider, const URI& target,
									const ReplaceOptions& options, const CancellationToken& token,
									Write&& write, BeforeReplace&& beforeReplace ) {
		ProviderCapabilities caps = provider.capabilities( target );
		auto writeTo = [&]( const URI& file, OpenFlags flags,
							bool* created = nullptr ) -> VFSResult<void> {
			OpenOptions openOptions( flags );
			openOptions.createPermissions =
				options.exactPermissions ? options.exactPermissions : options.createPermissions;
			auto opened = await( provider.open( file, openOptions, token ) );
			if ( !opened )
				return Err( std::move( opened ).error() );
			if ( created )
				*created = true;
			FileHandle handle( std::move( opened ).value() );
			VFSResult<void> written = write( handle );
			if ( written && options.fsync && caps.has( VFSCapability::Fsync ) )
				written = await( handle.flush() );
			auto closed = await( handle.close() );
			if ( !written )
				return written;
			if ( !closed )
				return closed;
			if ( options.exactPermissions && caps.has( VFSCapability::Permissions ) )
				return await( provider.setPermissions( file, *options.exactPermissions, token ) );
			return Ok();
		};

		if ( !options.atomic || !caps.has( VFSCapability::Rename ) ||
			 ( options.overwrite && !caps.has( VFSCapability::AtomicReplace ) ) ) {
			if ( options.atomic && !options.allowInPlace ) {
				return Err( VFSError( VFSErrorCode::Unsupported, target,
									  "Atomic replacement is not supported by this provider" ) );
			}
			// beforeReplace() guards a replacement; in place, the old version is already gone.
			return writeTo( target, OpenFlags::Write | OpenFlags::Create | OpenFlags::Truncate );
		}

		URI temporary( target.getParent().withAppendedSegment( temporarySiblingName() ) );
		bool created = false;
		auto written = writeTo(
			temporary, OpenFlags::Write | OpenFlags::Create | OpenFlags::Exclusive, &created );
		if ( written )
			written = beforeReplace();
		if ( written ) {
			written = publish( provider, temporary, target, options.overwrite, token );
		}
		if ( !written && created ) {
			// Best effort: remove only the temporary we created, never another creator's file
			// after an Exclusive open failed.
			await( provider.remove( temporary, {}, CancellationToken() ) );
		}
		return written;
	}

  private:
	/** Publish the exclusive temporary; only this path may use the local link/unlink fallback. */
	static VFSResult<void> publish( VFSProvider& provider, const URI& temporary, const URI& target,
									bool overwrite, const CancellationToken& token );

	/** @return A short hidden basename, unique to this process and call. */
	static std::string temporarySiblingName();
};

}}} // namespace EE::VFS::detail

#endif
