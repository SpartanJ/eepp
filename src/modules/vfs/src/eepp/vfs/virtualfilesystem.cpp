#include <algorithm>
#include <eepp/vfs/detail/await.hpp>
#include <eepp/vfs/detail/filewriter.hpp>
#include <eepp/vfs/providers/archiveprovider.hpp>
#include <eepp/vfs/providers/localprovider.hpp>
#include <eepp/vfs/providers/resourceprovider.hpp>
#include <eepp/vfs/providers/zipprovider.hpp>
#include <eepp/vfs/transferengine.hpp>
#include <eepp/vfs/virtualfilesystem.hpp>
#include <eepp/vfs/watchers/localwatchbackend.hpp>
#include <eepp/vfs/watchers/pollingwatchbackend.hpp>
#include <mutex>

#ifdef EE_VFS_SFTP
#include <eepp/vfs/sftp/sftpprovider.hpp>
#endif

namespace EE { namespace VFS {

using detail::await;
using detail::FileWriter;

namespace {

std::mutex sInstanceMutex;
std::unique_ptr<VirtualFileSystem> sInstance;

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
	mLocalIO( std::make_shared<LocalIOScheduler>( mIOPool, config.networkMountTimeout ) ),
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
	// Ends the network mount lanes: a thread blocked on an unreachable server is left behind.
	mLocalIO.reset();
	mIOPool.reset();
}

void VirtualFileSystem::registerDefaultProviders() {
	registerProvider( std::make_shared<LocalProvider>( mLocalIO ) );
	registerProvider( std::make_shared<ZipProvider>( mLocalIO ) );
	registerProvider( std::make_shared<ArchiveProvider>( mLocalIO ) );
	registerProvider( std::make_shared<ResourceProvider>( mIOPool ) );
	// Network mounts are polled through the VFS (whose calls time out), not by efsw.
	auto polling = std::make_shared<PollingWatchBackend>( *this );
	mWatchManager->registerBackend( "file",
									std::make_shared<LocalWatchBackend>( mIOPool, polling ) );
	mWatchManager->setFallbackBackend( std::move( polling ) );
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

VFSTask<VFSFileInfo> VirtualFileSystem::stat( const URI& uri, const StatOptions& options,
											  CancellationToken token ) {
	EE_VFS_FORWARD( VFSFileInfo, uri, stat( uri, options, std::move( token ) ) );
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
	 * move every (large) VFSFileInfo again on each reallocation. */
	std::vector<std::vector<VFSFileInfo>> batches;
	size_t total{ 0 };
	TaskResolver<VFSResult<std::vector<VFSFileInfo>>> resolve;
	CancellationToken token;
};

/** @return True when the listing finished (successfully or not). */
bool consumeBatch( ListAllState& state, VFSResult<DirectoryBatch>&& batch ) {
	if ( !batch ) {
		state.handle.close();
		state.resolve( Err( std::move( batch ).error() ) );
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
	std::vector<VFSFileInfo> all;
	if ( state.batches.size() == 1 ) {
		all = std::move( state.batches.front() );
	} else {
		all.reserve( state.total );
		for ( auto& part : state.batches ) {
			all.insert( all.end(), std::make_move_iterator( part.begin() ),
						std::make_move_iterator( part.end() ) );
		}
	}
	state.batches.clear();
	state.resolve( Ok( std::move( all ) ) );
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

VFSTask<std::vector<VFSFileInfo>> VirtualFileSystem::listAll( const URI& uri,
															  const DirectoryOpenOptions& options,
															  CancellationToken token ) {
	return openDirectory( uri, options, token )
		.then( [token]( VFSResult<DirectoryHandle>&& handle ) -> VFSTask<std::vector<VFSFileInfo>> {
			if ( !handle )
				return readyError<std::vector<VFSFileInfo>>( std::move( handle ).error() );
			auto state = std::make_shared<ListAllState>();
			state->handle = std::move( handle ).value();
			state->token = token;
			auto task = VFSTask<std::vector<VFSFileInfo>>::create( state->resolve );
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
			size_t chunk = FileWriter::IOChunkSize;
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

VFSTask<VFSFileInfo> VirtualFileSystem::writeAll( const URI& uri, std::string_view data,
												  const WriteOptions& options,
												  CancellationToken token ) {
	return writeAll( uri, std::vector<Uint8>( data.begin(), data.end() ), options,
					 std::move( token ) );
}

VFSTask<VFSFileInfo> VirtualFileSystem::writeAll( const URI& uri, std::vector<Uint8> data,
												  const WriteOptions& options,
												  CancellationToken token ) {
	auto provider = providerFor( uri );
	if ( !provider )
		return unsupportedScheme<VFSFileInfo>( uri );
	// The buffer is moved into the job: the caller's data must outlive the asynchronous write.
	return orchestrate<VFSFileInfo>( [provider, uri, data = std::move( data ), options,
									  token]() -> VFSResult<VFSFileInfo> {
		auto checkRevision = [&]( const VFSResult<VFSFileInfo>& current ) -> VFSResult<void> {
			if ( !options.expectedRevision )
				return Ok();
			if ( !current ) {
				if ( current.error().is( VFSErrorCode::NotFound ) ) {
					return Err( VFSError( VFSErrorCode::ChangedExternally, uri,
										  "The file was removed externally" ) );
				}
				return Err( current.error() );
			}
			if ( !current.value().revision.matches( *options.expectedRevision ) ) {
				return Err( VFSError( VFSErrorCode::ChangedExternally, uri,
									  "The file was modified externally" ) );
			}
			return Ok();
		};

		auto target = FileWriter::resolveTarget( *provider, uri, token );
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
			auto parents = FileWriter::makeDirs( *provider, target.value().getParent(), {}, token );
			if ( !parents )
				return Err( std::move( parents ).error() );
		}

		FileWriter::ReplaceOptions replace;
		if ( existing )
			replace.exactPermissions = existing.value().permissions();
		else
			replace.createPermissions = options.permissions;
		replace.fsync = options.fsync;
		replace.atomic = options.atomic;
		replace.allowInPlace = options.allowNonAtomicFallback;
		std::span<const Uint8> bytes( data.data(), data.size() );
		auto written = FileWriter::replace(
			*provider, target.value(), replace, token,
			[&]( FileHandle& handle ) { return FileWriter::writeFully( handle, bytes, token ); },
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
		return FileWriter::makeDirs( *provider, uri, options, token );
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

Transfer VirtualFileSystem::copy( const URI& source, const URI& destination,
								  const CopyOptions& options ) {
	return TransferEngine::start( *this, mTransferPool, source, destination, options, false );
}

Transfer VirtualFileSystem::move( const URI& source, const URI& destination,
								  const CopyOptions& options ) {
	return TransferEngine::start( *this, mTransferPool, source, destination, options, true );
}

}} // namespace EE::VFS
