#include <algorithm>
#include <atomic>
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/vfs/providers/localprovider.hpp>
#include <eepp/vfs/watchers/localwatchbackend.hpp>
#include <efsw/efsw.hpp>
#include <map>
#include <mutex>
#include <unordered_map>
#include <vector>

namespace EE { namespace VFS {

namespace detail {

/** Shared state of the local backend. Owns the efsw watchers (one per option combination, since
 * symlink options are per watcher in efsw) and one listener per watcher, because efsw watch ids
 * are only unique within a watcher and callbacks do not identify the watcher. */
class LocalWatchBackendImpl : public std::enable_shared_from_this<LocalWatchBackendImpl> {
  public:
	/** Events for a watch id that is not registered yet are buffered (efsw may deliver before
	 * addWatch() returns); beyond this many the watch gets a RescanRequired instead. */
	static constexpr size_t MaxEarlyEvents = 4096;

	explicit LocalWatchBackendImpl( Time pollingInterval ) : mPollingInterval( pollingInterval ) {}

	~LocalWatchBackendImpl() {
		// Watchers stop their threads in their destructors; they must die before the listeners
		// and the entries they call into.
		std::map<int, WatcherSlot> watchers;
		{
			std::lock_guard<std::mutex> lock( mWatchersMutex );
			watchers.swap( mWatchers );
		}
		for ( auto& slot : watchers )
			slot.second.watcher.reset();
	}

	/** @return ( registration token, served by the generic watcher ) */
	VFSResult<std::pair<Uint64, bool>> add( const std::string& nativeRoot,
											const WatchOptions& options,
											const std::shared_ptr<WatchSink>& sink ) {
		std::string root( nativeRoot );
		System::FileSystem::dirAddSlashAtEnd( root );
		Uint64 token = ++mLastToken;

		// Share an existing efsw watch on the same root with the same options.
		{
			std::lock_guard<std::mutex> lock( mEntriesMutex );
			for ( auto& entry : mEntries ) {
				int key = static_cast<int>( entry.first >> KeyShift );
				if ( ( key & ~GenericBit ) == optionBits( options ) &&
					 entry.second.recursive == options.recursive &&
					 entry.second.rootPath == root ) {
					entry.second.sinks.emplace_back( token, sink );
					return Ok( std::make_pair( token, ( key & GenericBit ) != 0 ) );
				}
			}
		}

		std::vector<efsw::WatcherOption> efswOptions;
		if ( options.reportCrossDirectoryMoves )
			efswOptions.emplace_back( efsw::Options::ReportCrossDirectoryMoves, 1 );

		int key = optionBits( options );
		efsw::WatchID id = addWatch( key, options, root, efswOptions );
		if ( id == efsw::Errors::FileRemote || id == efsw::Errors::FileOverlapping ||
			 id == efsw::Errors::FileRepeated || id == efsw::Errors::WatcherFailed ) {
			// Remote filesystems (NFS, SMB mounts) and layouts the native backend cannot
			// represent are served by efsw's generic polling watcher.
			key |= GenericBit;
			id = addWatch( key, options, root, efswOptions );
		}
		if ( id < 0 )
			return Err( mapEfswError( id, nativeRoot ) );

		std::vector<PendingEvent> early;
		Uint64 composite = compose( key, id );
		{
			std::lock_guard<std::mutex> lock( mEntriesMutex );
			Entry& entry = mEntries[composite];
			entry.rootPath = root;
			entry.recursive = options.recursive;
			entry.sinks.emplace_back( token, sink );
			auto& highest = mHighestId[key];
			highest = std::max( highest, id );
			auto it = mEarly.find( composite );
			if ( it != mEarly.end() ) {
				early = std::move( it->second );
				mEarly.erase( it );
			}
		}
		for ( auto& event : early )
			dispatch( key, id, event.dir, event.filename, event.action, event.oldFilename );
		return Ok( std::make_pair( token, ( key & GenericBit ) != 0 ) );
	}

	void remove( Uint64 token ) {
		Uint64 removed = 0;
		{
			std::lock_guard<std::mutex> lock( mEntriesMutex );
			for ( auto it = mEntries.begin(); it != mEntries.end() && removed == 0; ++it ) {
				auto& sinks = it->second.sinks;
				auto sink = std::find_if( sinks.begin(), sinks.end(),
										  [token]( const auto& s ) { return s.first == token; } );
				if ( sink == sinks.end() )
					continue;
				sinks.erase( sink );
				if ( !sinks.empty() )
					return;
				removed = it->first;
				mEntries.erase( it );
				break;
			}
		}
		if ( removed == 0 )
			return;
		// Outside the entries lock: efsw may be delivering a callback that needs it.
		std::lock_guard<std::mutex> lock( mWatchersMutex );
		auto it = mWatchers.find( static_cast<int>( removed >> KeyShift ) );
		if ( it != mWatchers.end() && it->second.watcher )
			it->second.watcher->removeWatch(
				static_cast<efsw::WatchID>( removed & ( ( Uint64( 1 ) << KeyShift ) - 1 ) ) );
	}

	void dispatch( int key, efsw::WatchID id, const std::string& dir, const std::string& filename,
				   efsw::Action action, const std::string& oldFilename ) {
		std::vector<std::shared_ptr<WatchSink>> sinks;
		std::string root;
		{
			std::lock_guard<std::mutex> lock( mEntriesMutex );
			Uint64 composite = compose( key, id );
			auto it = mEntries.find( composite );
			if ( it == mEntries.end() ) {
				// efsw ids only grow: an unknown id above the highest registered one belongs to
				// an addWatch() still in flight; anything else is a removed watch.
				if ( id > mHighestId[key] ) {
					auto& early = mEarly[composite];
					if ( early.size() < MaxEarlyEvents )
						early.push_back( PendingEvent{ dir, filename, oldFilename, action } );
				}
				return;
			}
			root = it->second.rootPath;
			for ( auto& sink : it->second.sinks )
				if ( auto locked = sink.second.lock() )
					sinks.push_back( std::move( locked ) );
		}
		if ( sinks.empty() )
			return;
		WatchEvent event;
		event.relativePath = toRelative( root, dir + filename );
		switch ( action ) {
			case efsw::Actions::Add:
				event.type = WatchEventType::Added;
				break;
			case efsw::Actions::Delete:
				event.type = WatchEventType::Removed;
				break;
			case efsw::Actions::Modified:
				event.type = WatchEventType::Modified;
				break;
			case efsw::Actions::Moved:
				event.type = WatchEventType::Moved;
				// Cross-directory moves carry an absolute source path; same-directory moves
				// carry the old name relative to dir.
				event.oldRelativePath =
					toRelative( root, isAbsolute( oldFilename ) ? oldFilename : dir + oldFilename );
				break;
		}
		if ( event.relativePath.empty() && event.type != WatchEventType::Modified ) {
			// The watched root itself was added/removed/moved: consumers must re-list it.
			event = WatchEvent{ {}, {}, WatchEventType::RescanRequired };
		}
		for ( size_t i = 0; i + 1 < sinks.size(); ++i ) {
			WatchEvent copy( event );
			sinks[i]->push( std::move( copy ) );
		}
		sinks.back()->push( std::move( event ) );
	}

	void missed( int key, efsw::WatchID id ) {
		std::vector<std::shared_ptr<WatchSink>> sinks;
		{
			std::lock_guard<std::mutex> lock( mEntriesMutex );
			auto it = mEntries.find( compose( key, id ) );
			if ( it == mEntries.end() )
				return;
			for ( auto& sink : it->second.sinks )
				if ( auto locked = sink.second.lock() )
					sinks.push_back( std::move( locked ) );
		}
		for ( auto& sink : sinks )
			sink->rescanRequired();
	}

  private:
	static constexpr int GenericBit = 1;
	static constexpr int KeyShift = 48;

	class Listener : public efsw::FileWatchListener {
	  public:
		Listener( LocalWatchBackendImpl& impl, int key ) : mImpl( impl ), mKey( key ) {}

		void handleFileAction( efsw::WatchID watchid, const std::string& dir,
							   const std::string& filename, efsw::Action action,
							   const std::string& oldFilename ) override {
			mImpl.dispatch( mKey, watchid, dir, filename, action, oldFilename );
		}

		void handleMissedFileActions( efsw::WatchID watchid, const std::string& ) override {
			mImpl.missed( mKey, watchid );
		}

	  private:
		LocalWatchBackendImpl& mImpl;
		int mKey;
	};

	struct WatcherSlot {
		std::unique_ptr<Listener> listener;
		std::unique_ptr<efsw::FileWatcher> watcher;
	};

	struct Entry {
		std::string rootPath; // native, with trailing separator
		std::vector<std::pair<Uint64, std::weak_ptr<WatchSink>>> sinks;
		bool recursive{ false };
	};

	struct PendingEvent {
		std::string dir;
		std::string filename;
		std::string oldFilename;
		efsw::Action action;
	};

	static int optionBits( const WatchOptions& options ) {
		return ( options.followSymlinks ? 2 : 0 ) | ( options.allowOutOfScopeLinks ? 4 : 0 );
	}

	static Uint64 compose( int key, efsw::WatchID id ) {
		return ( static_cast<Uint64>( key ) << KeyShift ) | static_cast<Uint64>( id );
	}

	static VFSError mapEfswError( efsw::WatchID error, const std::string& path ) {
		VFSErrorCode code = VFSErrorCode::IO;
		switch ( error ) {
			case efsw::Errors::FileNotFound:
				code = VFSErrorCode::NotFound;
				break;
			case efsw::Errors::FileNotReadable:
				code = VFSErrorCode::PermissionDenied;
				break;
			case efsw::Errors::FileOutOfScope:
				code = VFSErrorCode::InvalidOperation;
				break;
			default:
				break;
		}
		return VFSError( code, URI::fromFSPath( path ), efsw::Errors::Log::getLastErrorLog(),
						 error );
	}

	efsw::WatchID addWatch( int key, const WatchOptions& options, const std::string& root,
							const std::vector<efsw::WatcherOption>& efswOptions ) {
		std::lock_guard<std::mutex> lock( mWatchersMutex );
		auto& slot = mWatchers[key];
		if ( !slot.watcher ) {
			slot.listener = std::make_unique<Listener>( *this, key );
			slot.watcher =
				( key & GenericBit )
					? std::make_unique<efsw::FileWatcher>(
						  true, static_cast<unsigned int>( mPollingInterval.asMilliseconds() ) )
					: std::make_unique<efsw::FileWatcher>();
			slot.watcher->followSymlinks( options.followSymlinks );
			slot.watcher->allowOutOfScopeLinks( options.allowOutOfScopeLinks );
			slot.watcher->watch();
		}
		return slot.watcher->addWatch( root, slot.listener.get(), options.recursive, efswOptions );
	}

	static std::string toRelative( const std::string& root, std::string path ) {
#if EE_PLATFORM == EE_PLATFORM_WIN
		String::replaceAll( path, "\\", "/" );
		std::string normalizedRoot( root );
		String::replaceAll( normalizedRoot, "\\", "/" );
#else
		const std::string& normalizedRoot = root;
#endif
		if ( String::startsWith( path, normalizedRoot ) )
			path.erase( 0, normalizedRoot.size() );
		while ( !path.empty() && path.back() == '/' )
			path.pop_back();
		return path;
	}

	static bool isAbsolute( const std::string& path ) {
		if ( path.empty() )
			return false;
#if EE_PLATFORM == EE_PLATFORM_WIN
		return path.size() >= 2 && ( path[1] == ':' || ( path[0] == '\\' && path[1] == '\\' ) );
#else
		return path[0] == '/';
#endif
	}

	Time mPollingInterval;
	std::mutex mWatchersMutex;
	std::map<int, WatcherSlot> mWatchers;
	std::mutex mEntriesMutex;
	std::unordered_map<Uint64, Entry> mEntries;
	std::unordered_map<int, efsw::WatchID> mHighestId;
	std::unordered_map<Uint64, std::vector<PendingEvent>> mEarly;
	std::atomic<Uint64> mLastToken{ 0 };
};

class LocalWatchRegistration : public WatchRegistration {
  public:
	LocalWatchRegistration( std::shared_ptr<LocalWatchBackendImpl> impl, Uint64 token ) :
		mImpl( std::move( impl ) ), mToken( token ) {}

	~LocalWatchRegistration() override { mImpl->remove( mToken ); }

  private:
	std::shared_ptr<LocalWatchBackendImpl> mImpl;
	Uint64 mToken;
};

} // namespace detail

LocalWatchBackend::LocalWatchBackend( Time genericPollingInterval ) :
	mImpl( std::make_shared<detail::LocalWatchBackendImpl>( genericPollingInterval ) ) {}

LocalWatchBackend::~LocalWatchBackend() = default;

VFSTask<std::unique_ptr<WatchRegistration>>
LocalWatchBackend::watch( const URI& root, const WatchOptions& options,
						  std::shared_ptr<WatchSink> sink ) {
	using Registration = std::unique_ptr<WatchRegistration>;
	auto path = LocalProvider::toNativePath( root );
	if ( !path )
		return readyError<Registration>( std::move( path ).error() );
	if ( !System::FileSystem::isDirectory( path.value() ) ) {
		return readyError<Registration>( System::FileSystem::fileExists( path.value() )
											 ? VFSErrorCode::NotDirectory
											 : VFSErrorCode::NotFound,
										 root );
	}
	// efsw registration is synchronous and fast; once it returns the watch is active, so a
	// listing started afterwards cannot miss changes.
	auto added = mImpl->add( path.value(), options, sink );
	if ( !added )
		return readyError<Registration>( std::move( added ).error() );
	sink->setStatus( added.value().second ? WatchStatus::Polling : WatchStatus::Active );
	return readyValue<Registration>(
		std::make_unique<detail::LocalWatchRegistration>( mImpl, added.value().first ) );
}

}} // namespace EE::VFS
