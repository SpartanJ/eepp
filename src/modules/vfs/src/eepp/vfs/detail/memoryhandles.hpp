#ifndef EE_VFS_DETAIL_MEMORYHANDLES_HPP
#define EE_VFS_DETAIL_MEMORYHANDLES_HPP

#include <algorithm>
#include <cstring>
#include <eepp/vfs/directoryhandle.hpp>
#include <eepp/vfs/filehandle.hpp>
#include <mutex>
#include <vector>

namespace EE { namespace VFS { namespace detail {

/** Directory handle over an already materialized listing. Batches are returned as ready tasks. */
class VectorDirectoryImpl : public DirectoryHandleImpl {
  public:
	explicit VectorDirectoryImpl( std::vector<FileInfo> entries ) :
		mEntries( std::move( entries ) ) {}

	VFSTask<DirectoryBatch> read( size_t maxEntries, CancellationToken token ) override {
		DirectoryBatch batch;
		{
			std::lock_guard<std::mutex> lock( mMutex );
			if ( token.isCancellationRequested() )
				return readyError<DirectoryBatch>( VFSErrorCode::Cancelled, URI(),
												   "Operation cancelled" );
			size_t count = std::min( maxEntries, mEntries.size() - mPosition );
			if ( mPosition == 0 && count == mEntries.size() ) {
				batch.entries = std::move( mEntries );
				mEntries.clear();
				mPosition = 0;
			} else {
				batch.entries.reserve( count );
				for ( size_t i = 0; i < count; ++i )
					batch.entries.push_back( std::move( mEntries[mPosition + i] ) );
				mPosition += count;
			}
			batch.endOfDirectory = mPosition >= mEntries.size();
		}
		return readyValue<DirectoryBatch>( std::move( batch ) );
	}

	VFSTask<void> close() override {
		std::lock_guard<std::mutex> lock( mMutex );
		mEntries.clear();
		mPosition = 0;
		return readyValue<void>();
	}

  private:
	std::mutex mMutex;
	std::vector<FileInfo> mEntries;
	size_t mPosition{ 0 };
};

/** Read-only file handle over an immutable buffer. */
class BufferFileImpl : public FileHandleImpl {
  public:
	BufferFileImpl( std::shared_ptr<const std::vector<Uint8>> data, URI uri, FileInfo info,
					ProviderCapabilities caps ) :
		mData( std::move( data ) ),
		mUri( std::move( uri ) ),
		mInfo( std::move( info ) ),
		mCaps( caps ) {}

	VFSTask<size_t> read( Uint64 offset, std::span<Uint8> data, CancellationToken token ) override {
		if ( token.isCancellationRequested() )
			return readyError<size_t>( cancelledError( mUri ) );
		if ( offset >= mData->size() )
			return readyValue<size_t>( 0 );
		size_t count = std::min<size_t>( data.size(), mData->size() - offset );
		std::memcpy( data.data(), mData->data() + offset, count );
		return readyValue<size_t>( count );
	}

	VFSTask<size_t> write( Uint64, std::span<const Uint8>, CancellationToken ) override {
		return readyError<size_t>( VFSErrorCode::ReadOnly, mUri );
	}

	VFSTask<void> truncate( Uint64 ) override {
		return readyError<void>( VFSErrorCode::ReadOnly, mUri );
	}

	VFSTask<void> flush() override { return readyValue<void>(); }

	VFSTask<FileInfo> stat() override { return readyValue<FileInfo>( mInfo ); }

	VFSTask<void> close() override { return readyValue<void>(); }

	ProviderCapabilities capabilities() const override { return mCaps; }

  private:
	std::shared_ptr<const std::vector<Uint8>> mData;
	URI mUri;
	FileInfo mInfo;
	ProviderCapabilities mCaps;
};

}}} // namespace EE::VFS::detail

#endif
