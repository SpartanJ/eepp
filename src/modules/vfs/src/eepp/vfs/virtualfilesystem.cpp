#include <algorithm>
#include <atomic>
#include <eepp/core/string.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/vfs/detail/await.hpp>
#include <eepp/vfs/detail/transferstate.hpp>
#include <eepp/vfs/providers/localprovider.hpp>
#include <eepp/vfs/providers/resourceprovider.hpp>
#include <eepp/vfs/virtualfilesystem.hpp>
#include <eepp/vfs/watchers/localwatchbackend.hpp>
#include <eepp/vfs/watchers/pollingwatchbackend.hpp>
#include <mutex>

#ifdef EE_VFS_SFTP
#include <eepp/vfs/sftp/sftpprovider.hpp>
#endif

namespace EE { namespace VFS {

using detail::await;

namespace {

std::mutex sInstanceMutex;
std::unique_ptr<VirtualFileSystem> sInstance;

/** Chunk used by readAll()/writeAll() and the generic copy loop. */
constexpr size_t IOChunkSize = 256 * 1024;

bool sameAuthority( const URI& a, const URI& b ) {
	return a.getScheme() == b.getScheme() && a.getHost() == b.getHost() &&
		   a.getPort() == b.getPort() && a.getUserInfo() == b.getUserInfo();
}

std::string temporarySiblingName( std::string_view name ) {
	static std::atomic<Uint32> sCounter{ 0 };
	std::string tmp( "." );
	tmp.append( name );
	tmp += ".eepp-";
	tmp += String::toString( static_cast<Uint64>( System::Sys::getProcessID() ) );
	tmp += '-';
	tmp += String::toString( static_cast<Uint64>( System::Sys::getTicks() ) );
	tmp += '-';
	tmp += String::toString( ++sCounter );
	tmp += ".tmp";
	return tmp;
}

/** Writes every byte of data through handle, starting at offset. */
VFSResult<void> writeFully( FileHandle& handle, std::span<const Uint8> data,
							const CancellationToken& token, Uint64 offset = 0 ) {
	size_t done = 0;
	while ( done < data.size() ) {
		size_t chunk = std::min<size_t>( IOChunkSize, data.size() - done );
		auto written = await( handle.write( offset + done, data.subspan( done, chunk ), token ) );
		if ( !written )
			return Err( std::move( written ).error() );
		if ( written.value() == 0 )
			return Err( VFSError( VFSErrorCode::IO, handle.uri(), "Short write" ) );
		done += written.value();
	}
	return Ok();
}

/** makeDirs() body. Orchestration jobs call it directly: waiting on a makeDirs() task from the
 * orchestration pool would wait on a job queued behind the waiting one. */
VFSResult<void> makeDirsBlocking( VFSProvider& provider, const URI& uri,
								  const MkDirOptions& options, const CancellationToken& token ) {
	std::vector<URI> missing;
	URI current( uri );
	while ( true ) {
		auto info = await( provider.stat( current, StatOptions{}, token ) );
		if ( info ) {
			if ( !info.value().isDirectory() )
				return Err( VFSError( VFSErrorCode::NotDirectory, current ) );
			break;
		}
		if ( !info.error().is( VFSErrorCode::NotFound ) )
			return Err( std::move( info ).error() );
		missing.push_back( current );
		URI parent( current.getParent() );
		if ( parent.getPath() == current.getPath() )
			break;
		current = std::move( parent );
	}
	for ( auto it = missing.rbegin(); it != missing.rend(); ++it ) {
		auto made = await( provider.mkdir( *it, options, token ) );
		if ( !made ) {
			if ( !made.error().is( VFSErrorCode::AlreadyExists ) )
				return made;
			// Lost a race with another creator: fine if it is a directory.
			auto info = await( provider.stat( *it, StatOptions{}, token ) );
			if ( !info || !info.value().isDirectory() )
				return made;
		}
	}
	return Ok();
}

/** The file a write to uri replaces: the target of a symlink (so the link survives the
 * replacement), otherwise uri itself. */
VFSResult<URI> resolveWriteTarget( VFSProvider& provider, const URI& uri,
								   const CancellationToken& token ) {
	auto linkInfo = await( provider.stat( uri, StatOptions{ false }, token ) );
	if ( !linkInfo || !linkInfo.value().isSymlink() )
		return Ok( URI( uri ) );
	auto resolved = await( provider.canonicalize( uri, token ) );
	// A dangling link is replaced by the new file.
	if ( !resolved && resolved.error().is( VFSErrorCode::NotFound ) )
		return Ok( URI( uri ) );
	return resolved;
}

struct ReplaceOptions {
	/** Mode of a newly created file (the provider filters it through the umask). */
	std::optional<Uint32> createPermissions;
	/** Exact mode to give the new version (for example the replaced file's), applied after it is
	 * written: creation modes are filtered through the umask. */
	std::optional<Uint32> exactPermissions;
	bool fsync{ false };
	/** Replace through a temporary sibling. When false, target is rewritten in place. */
	bool atomic{ true };
	/** Without atomic replacement support, rewrite target in place instead of failing. */
	bool allowInPlace{ false };
	/** Fail if another entry appeared at target while the temporary was being written. */
	bool overwrite{ true };
};

/** Writes a new version of target with write( FileHandle& ). The data goes to a temporary sibling
 * that replaces target only once write() and beforeReplace() succeeded, so a failure never damages
 * the current version, nor another name of the same file (a hard link to it). Non-atomic writes,
 * and providers without atomic replacement (when allowed), rewrite target in place. */
template <typename Write, typename BeforeReplace>
VFSResult<void> replaceFile( VFSProvider& provider, const URI& target,
							 const ReplaceOptions& options, const CancellationToken& token,
							 Write&& write, BeforeReplace&& beforeReplace ) {
	ProviderCapabilities caps = provider.capabilities( target );
	auto writeTo = [&]( const URI& file, OpenFlags flags ) -> VFSResult<void> {
		OpenOptions openOptions( flags );
		openOptions.createPermissions =
			options.exactPermissions ? options.exactPermissions : options.createPermissions;
		auto opened = await( provider.open( file, openOptions, token ) );
		if ( !opened )
			return Err( std::move( opened ).error() );
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

	URI temporary( target.getParent().withAppendedSegment(
		temporarySiblingName( target.getLastPathSegment() ) ) );
	auto written =
		writeTo( temporary, OpenFlags::Write | OpenFlags::Create | OpenFlags::Exclusive );
	if ( written )
		written = beforeReplace();
	if ( written )
		written = await(
			provider.rename( temporary, target, RenameOptions{ options.overwrite }, token ) );
	if ( !written ) {
		// Best effort: never leave the temporary behind.
		await( provider.remove( temporary, {}, CancellationToken() ) );
	}
	return written;
}

} // namespace

VirtualFileSystem& VirtualFileSystem::instance() {
	std::lock_guard<std::mutex> lock( sInstanceMutex );
	if ( !sInstance )
		sInstance = std::make_unique<VirtualFileSystem>();
	return *sInstance;
}

void VirtualFileSystem::destroyInstance() {
	std::unique_ptr<VirtualFileSystem> instance;
	{
		std::lock_guard<std::mutex> lock( sInstanceMutex );
		instance = std::move( sInstance );
	}
}

VirtualFileSystem::VirtualFileSystem() : VirtualFileSystem( Config{} ) {}

VirtualFileSystem::VirtualFileSystem( const Config& config ) :
	mIOPool( ThreadPool::createShared( std::max<Uint32>( 1, config.ioThreads ) ) ),
	mOrchestrationPool(
		ThreadPool::createShared( std::max<Uint32>( 1, config.orchestrationThreads ) ) ),
	mTransferPool( ThreadPool::createShared( std::max<Uint32>( 1, config.transferThreads ) ) ),
	mWatchManager( std::make_unique<WatchManager>() ) {
	if ( config.registerDefaultProviders )
		registerDefaultProviders();
}

VirtualFileSystem::~VirtualFileSystem() {
	// Orchestration jobs wait on provider tasks: stop them first, while providers still run.
	mTransferPool.reset();
	mOrchestrationPool.reset();
	mWatchManager.reset();
	{
		std::unique_lock<std::shared_mutex> lock( mProvidersMutex );
		mProviders.clear();
	}
	mIOPool.reset();
}

void VirtualFileSystem::registerDefaultProviders() {
	registerProvider( std::make_shared<LocalProvider>( mIOPool ) );
	registerProvider( std::make_shared<ResourceProvider>( mIOPool ) );
	mWatchManager->registerBackend( "file", std::make_shared<LocalWatchBackend>( mIOPool ) );
	mWatchManager->setFallbackBackend( std::make_shared<PollingWatchBackend>( *this ) );
#ifdef EE_VFS_SFTP
	auto sftp = std::make_shared<SFTPProvider>();
	registerProvider( sftp );
	mWatchManager->registerBackend( "sftp", sftp->createWatchBackend( *this ) );
#endif
}

void VirtualFileSystem::registerProvider( std::shared_ptr<VFSProvider> provider ) {
	if ( !provider )
		return;
	std::string scheme( provider->scheme() );
	std::unique_lock<std::shared_mutex> lock( mProvidersMutex );
	mProviders[std::move( scheme )] = std::move( provider );
}

void VirtualFileSystem::unregisterProvider( std::string_view scheme ) {
	std::shared_ptr<VFSProvider> removed;
	std::unique_lock<std::shared_mutex> lock( mProvidersMutex );
	auto it = mProviders.find( std::string( scheme ) );
	if ( it != mProviders.end() ) {
		removed = std::move( it->second );
		mProviders.erase( it );
	}
}

std::shared_ptr<VFSProvider> VirtualFileSystem::provider( std::string_view scheme ) const {
	std::shared_lock<std::shared_mutex> lock( mProvidersMutex );
	auto it = mProviders.find( std::string( scheme ) );
	return it != mProviders.end() ? it->second : nullptr;
}

std::shared_ptr<VFSProvider> VirtualFileSystem::providerFor( const URI& uri ) const {
	return provider( uri.getScheme() );
}

std::vector<std::string> VirtualFileSystem::schemes() const {
	std::shared_lock<std::shared_mutex> lock( mProvidersMutex );
	std::vector<std::string> schemes;
	schemes.reserve( mProviders.size() );
	for ( const auto& provider : mProviders )
		schemes.push_back( provider.first );
	std::sort( schemes.begin(), schemes.end() );
	return schemes;
}

ProviderCapabilities VirtualFileSystem::capabilities( const URI& uri ) const {
	auto provider = providerFor( uri );
	return provider ? provider->capabilities( uri ) : ProviderCapabilities();
}

template <typename T> VFSTask<T> VirtualFileSystem::unsupportedScheme( const URI& uri ) const {
	return readyError<T>( uri.getScheme().empty() ? VFSErrorCode::InvalidURI
												  : VFSErrorCode::Unsupported,
						  uri, "No provider registered for scheme '" + uri.getScheme() + "'" );
}

template <typename T, typename F> VFSTask<T> VirtualFileSystem::orchestrate( F&& job ) {
	auto pool = mOrchestrationPool;
	if ( !pool )
		return readyError<T>( VFSErrorCode::InvalidOperation, URI(), "VFS is shutting down" );
	return pool->submit( std::forward<F>( job ) );
}

#define EE_VFS_FORWARD( Type, uri, call )      \
	auto provider = providerFor( uri );        \
	if ( !provider )                           \
		return unsupportedScheme<Type>( uri ); \
	return provider->call

VFSTask<FileInfo> VirtualFileSystem::stat( const URI& uri, const StatOptions& options,
										   CancellationToken token ) {
	EE_VFS_FORWARD( FileInfo, uri, stat( uri, options, std::move( token ) ) );
}

VFSTask<FileHandle> VirtualFileSystem::open( const URI& uri, const OpenOptions& options,
											 CancellationToken token ) {
	EE_VFS_FORWARD( FileHandle, uri, open( uri, options, std::move( token ) ) );
}

VFSTask<DirectoryHandle> VirtualFileSystem::openDirectory( const URI& uri,
														   const DirectoryOpenOptions& options,
														   CancellationToken token ) {
	EE_VFS_FORWARD( DirectoryHandle, uri, openDirectory( uri, options, std::move( token ) ) );
}

VFSTask<void> VirtualFileSystem::makeDir( const URI& uri, const MkDirOptions& options,
										  CancellationToken token ) {
	EE_VFS_FORWARD( void, uri, mkdir( uri, options, std::move( token ) ) );
}

VFSTask<void> VirtualFileSystem::rename( const URI& from, const URI& to,
										 const RenameOptions& options, CancellationToken token ) {
	if ( from.getScheme() != to.getScheme() ) {
		return readyError<void>( VFSErrorCode::Unsupported, from,
								 "Rename across schemes is not possible, use move()" );
	}
	EE_VFS_FORWARD( void, from, rename( from, to, options, std::move( token ) ) );
}

VFSTask<URI> VirtualFileSystem::canonicalize( const URI& uri, CancellationToken token ) {
	EE_VFS_FORWARD( URI, uri, canonicalize( uri, std::move( token ) ) );
}

VFSTask<void> VirtualFileSystem::setPermissions( const URI& uri, Uint32 permissions,
												 CancellationToken token ) {
	EE_VFS_FORWARD( void, uri, setPermissions( uri, permissions, std::move( token ) ) );
}

VFSTask<SpaceInfo> VirtualFileSystem::spaceInfo( const URI& uri, CancellationToken token ) {
	EE_VFS_FORWARD( SpaceInfo, uri, spaceInfo( uri, std::move( token ) ) );
}

#undef EE_VFS_FORWARD

// ---------------------------------------------------------------------------------------------
// listAll: non-blocking loop over DirectoryHandle::read().
// ---------------------------------------------------------------------------------------------

namespace {

struct ListAllState {
	DirectoryHandle handle;
	/** Batches are kept apart and joined once at the end: appending to one growing vector would
	 * move every (large) FileInfo again on each reallocation. */
	std::vector<std::vector<FileInfo>> batches;
	size_t total{ 0 };
	TaskSource<VFSResult<std::vector<FileInfo>>> source;
	CancellationToken token;
};

/** @return True when the listing finished (successfully or not). */
bool consumeBatch( ListAllState& state, VFSResult<DirectoryBatch>&& batch ) {
	if ( !batch ) {
		state.handle.close();
		state.source.resolve( Err( std::move( batch ).error() ) );
		return true;
	}
	auto& entries = batch.value().entries;
	if ( !entries.empty() ) {
		state.total += entries.size();
		state.batches.push_back( std::move( entries ) );
	}
	if ( !batch.value().endOfDirectory )
		return false;
	state.handle.close();
	std::vector<FileInfo> all;
	if ( state.batches.size() == 1 ) {
		all = std::move( state.batches.front() );
	} else {
		all.reserve( state.total );
		for ( auto& part : state.batches )
			all.insert( all.end(), std::make_move_iterator( part.begin() ),
						std::make_move_iterator( part.end() ) );
	}
	state.batches.clear();
	state.source.resolve( Ok( std::move( all ) ) );
	return true;
}

void listAllStep( std::shared_ptr<ListAllState> state ) {
	// Ready batches (synchronous providers) are consumed in a loop instead of nested
	// continuations, so long listings cannot grow the stack.
	while ( true ) {
		auto task = state->handle.read( 1024, state->token );
		if ( !task.isReady() ) {
			std::move( task ).then( [state]( VFSResult<DirectoryBatch>&& batch ) mutable {
				if ( !consumeBatch( *state, std::move( batch ) ) )
					listAllStep( std::move( state ) );
			} );
			return;
		}
		if ( consumeBatch( *state, std::move( task ).takeValue() ) )
			return;
	}
}

} // namespace

VFSTask<std::vector<FileInfo>> VirtualFileSystem::listAll( const URI& uri,
														   const DirectoryOpenOptions& options,
														   CancellationToken token ) {
	return openDirectory( uri, options, token )
		.then( [token]( VFSResult<DirectoryHandle>&& handle ) -> VFSTask<std::vector<FileInfo>> {
			if ( !handle )
				return readyError<std::vector<FileInfo>>( std::move( handle ).error() );
			auto state = std::make_shared<ListAllState>();
			state->handle = std::move( handle ).value();
			state->token = token;
			auto task = state->source.task();
			listAllStep( std::move( state ) );
			return task;
		} );
}

// ---------------------------------------------------------------------------------------------
// Orchestrated operations. These run on the orchestration pool and wait on provider tasks.
// ---------------------------------------------------------------------------------------------

VFSTask<std::vector<Uint8>> VirtualFileSystem::readAll( const URI& uri, CancellationToken token,
														Uint64 maxSize ) {
	return orchestrate<std::vector<Uint8>>( [this, uri, token,
											 maxSize]() -> VFSResult<std::vector<Uint8>> {
		auto opened = await( open( uri, OpenFlags::Read, token ) );
		if ( !opened )
			return Err( std::move( opened ).error() );
		FileHandle handle( std::move( opened ).value() );
		std::vector<Uint8> data;
		auto info = await( handle.stat() );
		if ( info && info.value().size() ) {
			if ( *info.value().size() > maxSize ) {
				handle.close();
				return Err( VFSError( VFSErrorCode::InvalidOperation, uri,
									  "File exceeds the maximum read size" ) );
			}
			data.reserve( static_cast<size_t>( *info.value().size() ) );
		}
		Uint64 offset = 0;
		while ( true ) {
			if ( token.isCancellationRequested() ) {
				handle.close();
				return Err( cancelledError( uri ) );
			}
			size_t old = data.size();
			size_t chunk = IOChunkSize;
			if ( data.capacity() > old && data.capacity() - old >= 1 )
				chunk = std::max<size_t>( chunk, data.capacity() - old );
			if ( old + chunk > maxSize + 1 )
				chunk = static_cast<size_t>( maxSize + 1 - old );
			data.resize( old + chunk );
			auto read =
				await( handle.read( offset, std::span<Uint8>( data.data() + old, chunk ), token ) );
			if ( !read ) {
				handle.close();
				return Err( std::move( read ).error() );
			}
			data.resize( old + read.value() );
			offset += read.value();
			if ( data.size() > maxSize ) {
				handle.close();
				return Err( VFSError( VFSErrorCode::InvalidOperation, uri,
									  "File exceeds the maximum read size" ) );
			}
			if ( read.value() == 0 )
				break;
		}
		auto closed = await( handle.close() );
		if ( !closed )
			return Err( std::move( closed ).error() );
		return Ok( std::move( data ) );
	} );
}

VFSTask<FileInfo> VirtualFileSystem::writeAll( const URI& uri, std::string_view data,
											   const WriteOptions& options,
											   CancellationToken token ) {
	return writeAll( uri, std::vector<Uint8>( data.begin(), data.end() ), options,
					 std::move( token ) );
}

VFSTask<FileInfo> VirtualFileSystem::writeAll( const URI& uri, std::vector<Uint8> data,
											   const WriteOptions& options,
											   CancellationToken token ) {
	auto provider = providerFor( uri );
	if ( !provider )
		return unsupportedScheme<FileInfo>( uri );
	// The buffer is moved into the job: the caller's data must outlive the asynchronous write.
	return orchestrate<FileInfo>(
		[provider, uri, data = std::move( data ), options, token]() -> VFSResult<FileInfo> {
			auto checkRevision = [&]( const VFSResult<FileInfo>& current ) -> VFSResult<void> {
				if ( !options.expectedRevision )
					return Ok();
				if ( !current ) {
					if ( current.error().is( VFSErrorCode::NotFound ) )
						return Err( VFSError( VFSErrorCode::ChangedExternally, uri,
											  "The file was removed externally" ) );
					return Err( current.error() );
				}
				if ( !current.value().revision.matches( *options.expectedRevision ) )
					return Err( VFSError( VFSErrorCode::ChangedExternally, uri,
										  "The file was modified externally" ) );
				return Ok();
			};

			auto target = resolveWriteTarget( *provider, uri, token );
			if ( !target )
				return Err( std::move( target ).error() );

			auto existing = await( provider->stat( target.value(), StatOptions{}, token ) );
			if ( !existing && !existing.error().is( VFSErrorCode::NotFound ) )
				return Err( std::move( existing ).error() );
			if ( auto check = checkRevision( existing ); !check )
				return Err( std::move( check ).error() );
			if ( existing && existing.value().isDirectory() )
				return Err( VFSError( VFSErrorCode::IsDirectory, uri ) );

			if ( options.createParents && !existing ) {
				auto parents = makeDirsBlocking( *provider, target.value().getParent(), {}, token );
				if ( !parents )
					return Err( std::move( parents ).error() );
			}

			ReplaceOptions replace;
			if ( existing )
				replace.exactPermissions = existing.value().permissions();
			else
				replace.createPermissions = options.permissions;
			replace.fsync = options.fsync;
			replace.atomic = options.atomic;
			replace.allowInPlace = options.allowNonAtomicFallback;
			std::span<const Uint8> bytes( data.data(), data.size() );
			auto written = replaceFile(
				*provider, target.value(), replace, token,
				[&]( FileHandle& handle ) { return writeFully( handle, bytes, token ); },
				[&]() -> VFSResult<void> {
					// Narrow the race window: re-check right before replacing.
					if ( !options.expectedRevision )
						return Ok();
					return checkRevision(
						await( provider->stat( target.value(), StatOptions{}, token ) ) );
				} );
			if ( !written )
				return Err( std::move( written ).error() );
			return await( provider->stat( target.value(), StatOptions{}, token ) );
		} );
}

VFSTask<void> VirtualFileSystem::makeDirs( const URI& uri, const MkDirOptions& options,
										   CancellationToken token ) {
	auto provider = providerFor( uri );
	if ( !provider )
		return unsupportedScheme<void>( uri );
	return orchestrate<void>( [provider, uri, options, token]() -> VFSResult<void> {
		return makeDirsBlocking( *provider, uri, options, token );
	} );
}

namespace {

VFSResult<void> removeTree( VFSProvider& provider, const URI& uri, const CancellationToken& token,
							int depth ) {
	if ( token.isCancellationRequested() )
		return Err( cancelledError( uri ) );
	if ( depth > 512 )
		return Err( VFSError( VFSErrorCode::TooManySymlinks, uri, "Directory tree too deep" ) );
	auto info = await( provider.stat( uri, StatOptions{ false }, token ) );
	if ( !info )
		return Err( std::move( info ).error() );
	if ( info.value().type == FileType::Directory ) {
		auto opened = await( provider.openDirectory(
			uri, DirectoryOpenOptions{ DirectoryMetadata::NamesAndTypes, false }, token ) );
		if ( !opened )
			return Err( std::move( opened ).error() );
		DirectoryHandle handle( std::move( opened ).value() );
		while ( true ) {
			auto batch = await( handle.read( 512, token ) );
			if ( !batch )
				return Err( std::move( batch ).error() );
			for ( const auto& entry : batch.value().entries ) {
				// Symlinks are removed, never followed.
				VFSResult<void> removed =
					entry.type == FileType::Directory
						? removeTree( provider, uri.withAppendedSegment( entry.name ), token,
									  depth + 1 )
						: await(
							  provider.remove( uri.withAppendedSegment( entry.name ), {}, token ) );
				if ( !removed )
					return removed;
			}
			if ( batch.value().endOfDirectory )
				break;
		}
		handle.close();
	}
	return await( provider.remove( uri, {}, token ) );
}

} // namespace

VFSTask<void> VirtualFileSystem::remove( const URI& uri, const RemoveOptions& options,
										 CancellationToken token ) {
	auto provider = providerFor( uri );
	if ( !provider )
		return unsupportedScheme<void>( uri );
	if ( !options.recursive )
		return provider->remove( uri, options, std::move( token ) );
	return orchestrate<void>( [provider, uri, token]() -> VFSResult<void> {
		return removeTree( *provider, uri, token, 0 );
	} );
}

VFSTask<WatchHandle> VirtualFileSystem::watch( const URI& root, const WatchOptions& options ) {
	return mWatchManager->watch( root, options );
}

// ---------------------------------------------------------------------------------------------
// Transfer engine: recursive copy/move across providers.
// ---------------------------------------------------------------------------------------------

class TransferEngine {
  public:
	TransferEngine( VirtualFileSystem& vfs, std::shared_ptr<detail::TransferState> state,
					URI source, URI destination, CopyOptions options, bool move ) :
		mVfs( vfs ),
		mState( std::move( state ) ),
		mSource( std::move( source ) ),
		mDestination( std::move( destination ) ),
		mOptions( options ),
		mToken( mState->token() ),
		mMove( move ) {
		if ( mOptions.chunkSize == 0 )
			mOptions.chunkSize = IOChunkSize;
	}

	VFSResult<TransferResult> run() {
		auto srcProvider = mVfs.providerFor( mSource );
		auto dstProvider = mVfs.providerFor( mDestination );
		if ( !srcProvider )
			return Err( VFSError( VFSErrorCode::Unsupported, mSource, "Unknown scheme" ) );
		if ( !dstProvider )
			return Err( VFSError( VFSErrorCode::Unsupported, mDestination, "Unknown scheme" ) );
		mSrcProvider = srcProvider.get();
		mDstProvider = dstProvider.get();

		if ( mSource == mDestination )
			return Err( VFSError( VFSErrorCode::InvalidOperation, mSource,
								  "Source and destination are the same" ) );

		mSameProvider = mSrcProvider == mDstProvider && sameAuthority( mSource, mDestination );

		if ( mMove && mSameProvider ) {
			auto renamed = await( mSrcProvider->rename(
				mSource, mDestination,
				RenameOptions{ mOptions.overwrite == OverwritePolicy::Overwrite }, mToken ) );
			if ( renamed ) {
				mResult.renamed = true;
				mState->updateProgress(
					[&]( TransferProgress& p ) {
						p.currentSource = mSource;
						p.currentDestination = mDestination;
						p.filesDone = 1;
						p.filesTotal = 1;
					},
					true );
				return Ok( mResult );
			}
			const auto code = renamed.error().code;
			bool fallback = code == VFSErrorCode::Unsupported || code == VFSErrorCode::NotEmpty ||
							( code == VFSErrorCode::AlreadyExists &&
							  mOptions.overwrite != OverwritePolicy::Fail ) ||
							( code == VFSErrorCode::IsDirectory &&
							  mOptions.overwrite != OverwritePolicy::Fail );
			if ( !fallback )
				return Err( std::move( renamed ).error() );
		}

		auto rootInfo = await( mSrcProvider->stat( mSource, StatOptions{ false }, mToken ) );
		if ( !rootInfo )
			return Err( std::move( rootInfo ).error() );
		if ( rootInfo.value().isSymlink() ) {
			// Providers cannot create links, so a copy takes the link's content. A move cannot:
			// removing what was copied would empty the link's target, not move the link.
			if ( mMove ) {
				return Err( VFSError( VFSErrorCode::Unsupported, mSource,
									  "Symbolic links can only be moved within their file "
									  "system" ) );
			}
			rootInfo = await( mSrcProvider->stat( mSource, StatOptions{ true }, mToken ) );
			if ( !rootInfo )
				return Err( std::move( rootInfo ).error() );
		}

		if ( rootInfo.value().isDirectory() && mSameProvider &&
			 mDestination.isSameOrChildOf( mSource ) ) {
			return Err( VFSError( VFSErrorCode::InvalidOperation, mDestination,
								  "Cannot copy a directory into itself" ) );
		}

		// Scan first: totals for progress, and the copy phase reuses the listing instead of
		// enumerating (possibly remote) directories twice.
		Node root{ std::move( rootInfo ).value(), {} };
		Uint64 files = 0, bytes = 0;
		if ( auto scanned = scan( root, mSource, 0, files, bytes ); !scanned )
			return Err( std::move( scanned ).error() );
		mState->updateProgress(
			[&]( TransferProgress& p ) {
				p.filesTotal = files;
				p.bytesTotal = bytes;
			},
			true );

		auto copied = copyNode( root, mSource, mDestination, 0 );
		if ( !copied )
			return Err( std::move( copied ).error() );
		return Ok( mResult );
	}

  private:
	struct Node {
		FileInfo info;
		std::vector<Node> children;
	};

	static constexpr int MaxDepth = 256;

	VFSResult<void> checkCancelled( const URI& uri ) const {
		if ( mToken.isCancellationRequested() )
			return Err( cancelledError( uri ) );
		return Ok();
	}

	VFSResult<void> scan( Node& node, const URI& uri, int depth, Uint64& files, Uint64& bytes ) {
		if ( auto c = checkCancelled( uri ); !c )
			return c;
		if ( depth > MaxDepth )
			return Err( VFSError( VFSErrorCode::TooManySymlinks, uri, "Directory tree too deep" ) );
		if ( !node.info.isDirectory() ) {
			++files;
			bytes += node.info.size().value_or( 0 );
			return Ok();
		}
		if ( !mOptions.recursive )
			return Err( VFSError( VFSErrorCode::IsDirectory, uri, "Recursive copy disabled" ) );
		auto listed = await( mVfs.listAll( uri, DirectoryOpenOptions{}, mToken ) );
		if ( !listed )
			return Err( std::move( listed ).error() );
		node.children.reserve( listed.value().size() );
		for ( auto& entry : listed.value() ) {
			Node child{ std::move( entry ), {} };
			if ( child.info.isSymlink() && child.info.targetType == FileType::Directory ) {
				// Providers cannot create symlinks, and following directory links risks cycles:
				// symlinked directories inside the tree are skipped.
				node.children.push_back( std::move( child ) );
				continue;
			}
			if ( auto scanned = scan( child, uri.withAppendedSegment( child.info.name ), depth + 1,
									  files, bytes );
				 !scanned )
				return scanned;
			node.children.push_back( std::move( child ) );
		}
		return Ok();
	}

	/** @return True if the entry (and everything below it) was fully copied. */
	VFSResult<bool> copyNode( const Node& node, const URI& source, const URI& destination,
							  int depth ) {
		if ( auto c = checkCancelled( source ); !c )
			return Err( std::move( c ).error() );
		const FileInfo& info = node.info;

		if ( depth > 0 && info.isSymlink() && info.targetType == FileType::Directory ) {
			++mResult.filesSkipped;
			return Ok( false );
		}

		if ( info.isDirectory() )
			return copyDirectory( node, source, destination, depth );

		if ( !info.isRegularFile() ) {
			// Sockets, FIFOs, devices and dangling links are not copied.
			++mResult.filesSkipped;
			return Ok( false );
		}

		auto existing = await( mDstProvider->stat( destination, StatOptions{}, mToken ) );
		if ( existing ) {
			if ( existing.value().isDirectory() )
				return Err( VFSError( VFSErrorCode::IsDirectory, destination ) );
			if ( mOptions.overwrite == OverwritePolicy::Fail )
				return Err( VFSError( VFSErrorCode::AlreadyExists, destination ) );
			if ( mOptions.overwrite == OverwritePolicy::Skip ) {
				++mResult.filesSkipped;
				advanceFiles( info );
				return Ok( false );
			}
			if ( mSameProvider ) {
				auto same = isSameFile( source, destination );
				if ( !same )
					return Err( std::move( same ).error() );
				if ( same.value() ) {
					return Err( VFSError( VFSErrorCode::InvalidOperation, destination,
										  "Source and destination are the same file" ) );
				}
			}
		} else if ( !existing.error().is( VFSErrorCode::NotFound ) ) {
			return Err( std::move( existing ).error() );
		}

		if ( auto copied = copyFile( info, source, destination ); !copied ) {
			// A destination can appear while the temporary is written. Skip still applies at
			// publication time, and a move must keep that source.
			if ( mOptions.overwrite == OverwritePolicy::Skip &&
				 copied.error().is( VFSErrorCode::AlreadyExists ) ) {
				++mResult.filesSkipped;
				advanceFiles( info );
				return Ok( false );
			}
			return Err( std::move( copied ).error() );
		}
		++mResult.filesTransferred;
		advanceFiles( info, false );

		if ( mMove ) {
			auto removed = await( mSrcProvider->remove( source, {}, mToken ) );
			if ( !removed )
				return Err( std::move( removed ).error() );
			++mResult.sourcesRemoved;
		}
		return Ok( true );
	}

	VFSResult<bool> copyDirectory( const Node& node, const URI& source, const URI& destination,
								   int depth ) {
		auto existing = await( mDstProvider->stat( destination, StatOptions{}, mToken ) );
		if ( existing ) {
			if ( !existing.value().isDirectory() )
				return Err( VFSError( VFSErrorCode::NotDirectory, destination ) );
			if ( mOptions.overwrite == OverwritePolicy::Fail )
				return Err( VFSError( VFSErrorCode::AlreadyExists, destination ) );
		} else if ( !existing.error().is( VFSErrorCode::NotFound ) ) {
			return Err( std::move( existing ).error() );
		} else {
			MkDirOptions mkdir;
			if ( mOptions.preservePermissions &&
				 mDstProvider->capabilities( destination ).has( VFSCapability::Permissions ) )
				mkdir.permissions = node.info.permissions();
			auto made = await( mDstProvider->mkdir( destination, mkdir, mToken ) );
			if ( !made )
				return Err( std::move( made ).error() );
			++mResult.directoriesCreated;
		}

		bool complete = true;
		for ( const auto& child : node.children ) {
			auto copied = copyNode( child, source.withAppendedSegment( child.info.name ),
									destination.withAppendedSegment( child.info.name ), depth + 1 );
			if ( !copied )
				return copied;
			complete = complete && copied.value();
		}

		// A move only removes a source directory whose whole content was moved: skipped or
		// uncopyable entries keep it (and their data) in place.
		if ( mMove && complete ) {
			auto removed = await( mSrcProvider->remove( source, {}, mToken ) );
			if ( !removed )
				return Err( std::move( removed ).error() );
			++mResult.sourcesRemoved;
		}
		return Ok( complete );
	}

	void advanceFiles( const FileInfo& info, bool addBytes = true ) {
		mState->updateProgress(
			[&]( TransferProgress& p ) {
				++p.filesDone;
				if ( addBytes )
					p.bytesDone += info.size().value_or( 0 );
			},
			true );
	}

	/** @return True when both URIs name the same file through symlinks or "..". Hard links are
	 * not detected; replaceFile() keeps their data safe. */
	VFSResult<bool> isSameFile( const URI& source, const URI& destination ) {
		auto canonicalSource = await( mSrcProvider->canonicalize( source, mToken ) );
		if ( !canonicalSource )
			return Err( std::move( canonicalSource ).error() );
		auto canonicalDestination = await( mDstProvider->canonicalize( destination, mToken ) );
		if ( !canonicalDestination )
			return Err( std::move( canonicalDestination ).error() );
		return Ok( canonicalSource.value() == canonicalDestination.value() );
	}

	VFSResult<void> copyFile( const FileInfo& info, const URI& source, const URI& destination ) {
		mState->updateProgress(
			[&]( TransferProgress& p ) {
				p.currentSource = source;
				p.currentDestination = destination;
			},
			true );

		auto target = resolveWriteTarget( *mDstProvider, destination, mToken );
		if ( !target )
			return Err( std::move( target ).error() );

		auto srcOpened = await( mSrcProvider->open( source, OpenFlags::Read, mToken ) );
		if ( !srcOpened )
			return Err( std::move( srcOpened ).error() );
		FileHandle src( std::move( srcOpened ).value() );

		ReplaceOptions replace;
		if ( mOptions.preservePermissions && info.permissions() )
			replace.exactPermissions = info.permissions();
		replace.overwrite = mOptions.overwrite == OverwritePolicy::Overwrite;
		std::vector<Uint8> buffer( mOptions.chunkSize );
		auto copied = replaceFile(
			*mDstProvider, target.value(), replace, mToken,
			[&]( FileHandle& dst ) -> VFSResult<void> {
				Uint64 offset = 0;
				while ( true ) {
					if ( auto c = checkCancelled( source ); !c )
						return c;
					auto read = await( src.read( offset, buffer, mToken ) );
					if ( !read )
						return Err( std::move( read ).error() );
					size_t got = read.value();
					if ( got == 0 )
						return Ok();
					if ( auto written = writeFully(
							 dst, std::span<const Uint8>( buffer.data(), got ), mToken, offset );
						 !written )
						return written;
					offset += got;
					mResult.bytesTransferred += got;
					mState->updateProgress( [got]( TransferProgress& p ) { p.bytesDone += got; } );
				}
			},
			[]() -> VFSResult<void> { return Ok(); } );
		await( src.close() );
		return copied;
	}

	VirtualFileSystem& mVfs;
	std::shared_ptr<detail::TransferState> mState;
	URI mSource;
	URI mDestination;
	CopyOptions mOptions;
	CancellationToken mToken;
	VFSProvider* mSrcProvider{ nullptr };
	VFSProvider* mDstProvider{ nullptr };
	TransferResult mResult;
	bool mMove;
	/** Source and destination are served by the same provider and authority. */
	bool mSameProvider{ false };
};

namespace {

Transfer startTransfer( VirtualFileSystem& vfs, std::shared_ptr<ThreadPool> pool, URI source,
						URI destination, const CopyOptions& options, bool move ) {
	auto state = std::make_shared<detail::TransferState>();
	Transfer transfer( state );
	if ( !pool ) {
		state->complete(
			Err( VFSError( VFSErrorCode::InvalidOperation, source, "VFS is shutting down" ) ) );
		return transfer;
	}
	auto engine = std::make_shared<TransferEngine>( vfs, state, std::move( source ),
													std::move( destination ), options, move );
	pool->run( [engine = std::move( engine ), state]() { state->complete( engine->run() ); } );
	return transfer;
}

} // namespace

Transfer VirtualFileSystem::copy( const URI& source, const URI& destination,
								  const CopyOptions& options ) {
	return startTransfer( *this, mTransferPool, source, destination, options, false );
}

Transfer VirtualFileSystem::move( const URI& source, const URI& destination,
								  const CopyOptions& options ) {
	return startTransfer( *this, mTransferPool, source, destination, options, true );
}

}} // namespace EE::VFS
