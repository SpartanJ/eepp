#ifndef EE_VFS_PROVIDERS_ARCHIVEPROVIDER_HPP
#define EE_VFS_PROVIDERS_ARCHIVEPROVIDER_HPP

#include <eepp/vfs/config.hpp>
#include <eepp/vfs/provider.hpp>
#include <eepp/vfs/providers/localioscheduler.hpp>
#include <memory>
#include <string>
#include <vector>

namespace EE { namespace VFS {

class ArchiveProviderImpl;

/** Runtime capabilities of the installed libarchive 3.x library. Only built-in decoders are
 * enabled: the provider never delegates decompression to external commands. */
struct EE_VFS_API ArchiveSupport {
	std::string version;
	std::vector<std::string> formats;
	std::vector<std::string> filters;
};

/** Read-only archive:// filesystem backed by optional runtime-loaded libarchive 3.x.
 * No libarchive headers or link dependency are required. Locations have the form
 * archive:///path/archive.tar.gz!/member (use makeURI()). Local archives only.
 * Indexing and reads run through LocalIOScheduler, including mount isolation and safe timeouts.
 * Entry data streams with bounded buffers. Backward seeks reopen and replay the archive;
 * RandomAccess is not advertised. Readers retain the runtime library until they are closed.
 * Missing/incompatible libraries return Unsupported. Emscripten has no runtime backend. */
class EE_VFS_API ArchiveProvider : public VFSProvider {
  public:
	/** libraryPath optionally selects an exact runtime library; empty uses platform defaults.
	 * Loading happens lazily on an I/O thread, never in the constructor. */
	explicit ArchiveProvider( std::shared_ptr<LocalIOScheduler> scheduler,
							  std::string libraryPath = {} );

	~ArchiveProvider() override;

	/** Builds an archive location. member is an unescaped, '/'-separated path inside it. Only
	 * file:// backing archives are currently supported; other inputs return an empty URI.
	 * Root is "/". */
	static URI makeURI( const URI& archive, std::string_view member = "/" );

	/** Decodes and validates the backing file:// URI of an archive:// location. */
	static VFSResult<URI> archiveURI( const URI& uri );

	/** Validates an archive location and returns its normalized path within the archive. */
	static VFSResult<std::string> memberPath( const URI& uri );

	std::string_view scheme() const override { return "archive"; }

	/** Reports the actual installed library's capabilities, or Unsupported when unavailable. */
	VFSTask<ArchiveSupport> support( CancellationToken token = {} );

	/** Follows a renamed backing archive or its containing directory, preserving the member. */
	bool relocateURI( URI& uri, const URI& from, const URI& to ) const override;

	ProviderCapabilities capabilities( const URI& uri ) const override;

	VFSTask<VFSFileInfo> stat( const URI& uri, const StatOptions& options,
							   CancellationToken token ) override;

	VFSTask<DirectoryHandle> openDirectory( const URI& uri, const DirectoryOpenOptions& options,
											CancellationToken token ) override;

	VFSTask<FileHandle> open( const URI& uri, const OpenOptions& options,
							  CancellationToken token ) override;

	VFSTask<void> mkdir( const URI& uri, const MkDirOptions& options,
						 CancellationToken token ) override;

	VFSTask<void> remove( const URI& uri, const RemoveOptions& options,
						  CancellationToken token ) override;

	VFSTask<void> rename( const URI& from, const URI& to, const RenameOptions& options,
						  CancellationToken token ) override;

	VFSTask<URI> canonicalize( const URI& uri, CancellationToken token ) override;

	VFSTask<bool> sameFileSystem( const URI& from, const URI& to,
								  CancellationToken token ) override;

  private:
	std::shared_ptr<ArchiveProviderImpl> mImpl;
};

}} // namespace EE::VFS

#endif
