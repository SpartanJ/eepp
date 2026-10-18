#ifndef EE_VFS_PROVIDERS_ZIPPROVIDER_HPP
#define EE_VFS_PROVIDERS_ZIPPROVIDER_HPP

#include <eepp/vfs/config.hpp>
#include <eepp/vfs/provider.hpp>
#include <eepp/vfs/providers/localioscheduler.hpp>
#include <memory>

namespace EE { namespace VFS {

class ZipProviderImpl;

/** Read-only zip:// filesystem over individual local archives. Use makeURI() to identify an
 * archive and a member path: zip:///path/archive.zip!/member. Literal bangs in the backing
 * path are doubled to distinguish them from the archive boundary.
 * Metadata is indexed without decompressing entries. Open handles retain their archive snapshot;
 * new operations notice changes to the backing file. At most eight archive indexes are cached.
 * Archives on network mounts use LocalIOScheduler's lanes and buffer-safe timeout delivery. */
class EE_VFS_API ZipProvider : public VFSProvider {
  public:
	explicit ZipProvider( std::shared_ptr<LocalIOScheduler> scheduler );

	~ZipProvider() override;

	/** Builds an archive location. member is an unescaped, '/'-separated path inside it. Only
	 * file:// backing archives are currently supported; other inputs return an empty URI.
	 * Root is "/". */
	static URI makeURI( const URI& archive, std::string_view member = "/" );

	/** Decodes and validates the backing file:// URI of a zip:// location. */
	static VFSResult<URI> archiveURI( const URI& uri );

	/** Validates a ZIP location and returns its normalized path within the archive. */
	static VFSResult<std::string> memberPath( const URI& uri );

	std::string_view scheme() const override { return "zip"; }

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
	std::shared_ptr<ZipProviderImpl> mImpl;
};

}} // namespace EE::VFS

#endif
