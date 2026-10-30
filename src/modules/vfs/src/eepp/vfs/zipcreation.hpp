#ifndef EE_VFS_ZIPCREATION_HPP
#define EE_VFS_ZIPCREATION_HPP

#include <eepp/system/threadpool.hpp>
#include <eepp/system/zip.hpp>
#include <eepp/vfs/fileinfo.hpp>
#include <eepp/vfs/transfer.hpp>
#include <optional>
#include <vector>

namespace EE { namespace VFS {

using namespace EE::System;

class VirtualFileSystem;

namespace detail {

class TransferState;

} // namespace detail

/** Creates a fresh ZIP from VFS sources on the transfer pool. The output is published only
 * after every entry and the central directory have been written successfully. */
class ZipCreation {
  public:
	static Transfer start( VirtualFileSystem& vfs, std::shared_ptr<ThreadPool> pool,
						   std::vector<URI> sources, URI destination );

	ZipCreation( VirtualFileSystem& vfs, std::shared_ptr<detail::TransferState> state,
				 std::vector<URI> sources, URI destination );

	VFSResult<TransferResult> run();

  private:
	struct Entry {
		URI uri;
		std::string name;
		std::string link;
		Zip::EntryOptions options;
		Uint64 size;
		FileRevision revision;
		FileType type;
	};

	VFSResult<void> scan( const URI& uri, std::string name, unsigned depth );

	VFSResult<void> checkSource( const Entry& entry );

	VirtualFileSystem& mVfs;
	std::shared_ptr<detail::TransferState> mState;
	CancellationToken mToken;
	std::vector<URI> mSources;
	URI mDestination;
	std::vector<Entry> mEntries;
	std::optional<VFSError> mSourceError;
	TransferResult mResult;
};

}} // namespace EE::VFS

#endif
