#ifndef EE_VFS_PROVIDERS_RESOURCEPROVIDER_HPP
#define EE_VFS_PROVIDERS_RESOURCEPROVIDER_HPP

#include <eepp/system/threadpool.hpp>
#include <eepp/vfs/provider.hpp>
#include <memory>

namespace EE { namespace VFS {

/** @brief Read-only `res://` provider over the files of every open Pack.
 *
 * `res:///fonts/DejaVuSans.ttf` resolves to the pack path "fonts/DejaVuSans.ttf" through
 * System::PackFileIndex. Directories are implicit (they exist while any packed file lies below
 * them). Packs are compressed archives, so files are extracted to memory when opened.
 *
 * Revisions change whenever a pack is opened or closed. Sizes are reported once a file is opened
 * (packs expose no cheap size query), so listing and stat leave them unknown.
 */
class ResourceProvider : public VFSProvider {
  public:
	explicit ResourceProvider( std::shared_ptr<System::ThreadPool> pool );

	std::string_view scheme() const override { return "res"; }

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

	/** @return The pack-relative path for a res:// URI ("" for the root). */
	static std::string toPackPath( const URI& uri );

  private:
	std::shared_ptr<System::ThreadPool> mPool;
};

}} // namespace EE::VFS

#endif
