#include <algorithm>
#include <eepp/core/string.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/vfs/ui/vfsdirectorymodel.hpp>
#include <mutex>
#include <unordered_map>
#include <unordered_set>

namespace EE { namespace VFS {

namespace {

struct CountState {
	DirectoryHandle handle;
	CancellationToken token;
	Uint64 all{ 0 };
	Uint64 visible{ 0 };
};

/** Counts directory entries batch by batch without keeping them. */
template <typename Count> VFSTask<Count> readCount( std::shared_ptr<CountState> state ) {
	return state->handle.read( 1024, state->token )
		.then( [state]( VFSResult<DirectoryBatch>&& batch ) -> VFSTask<Count> {
			if ( !batch ) {
				state->handle.close();
				return makeReadyTask( VFSResult<Count>( Err( std::move( batch ).error() ) ) );
			}
			for ( const auto& entry : batch.value().entries ) {
				++state->all;
				if ( !entry.hidden )
					++state->visible;
			}
			if ( batch.value().endOfDirectory ) {
				state->handle.close();
				return makeReadyTask(
					VFSResult<Count>( Ok( Count{ state->all, state->visible } ) ) );
			}
			return readCount<Count>( state );
		} );
}

std::string defaultItemCount( Uint64 count ) {
	return count == 1 ? std::string( "1 item" ) : String::toString( count ) + " items";
}

} // namespace

struct VFSDirectoryModel::Session {
	CancellationSource cancel;
	DirectoryHandle handle;
	WatchHandle watch;
	/** Events that arrived while a listing or re-list was in progress. */
	std::vector<WatchEvent> buffered;
	Uint64 generation{ 0 };
	/** A listing or re-list is running: watch events are buffered. */
	bool listing{ true };
	/** Too many buffered events (or a rescan request) while listing: re-list afterwards. */
	bool rescanAfterListing{ false };
};

std::shared_ptr<VFSDirectoryModel> VFSDirectoryModel::New( Executor& uiExecutor ) {
	return New( uiExecutor, Config{} );
}

std::shared_ptr<VFSDirectoryModel> VFSDirectoryModel::New( Executor& uiExecutor,
														   const Config& config ) {
	return std::shared_ptr<VFSDirectoryModel>( new VFSDirectoryModel( uiExecutor, config ) );
}

class VFSDirectoryModel::ExecutorGate final : public Executor {
  public:
	explicit ExecutorGate( Executor& target ) : mTarget( &target ) {}

	void execute( Function fn ) override {
		{
			std::lock_guard<std::mutex> lock( mMutex );
			if ( mTarget ) {
				mTarget->execute( std::move( fn ) );
				return;
			}
		}
		// Closed: the model is gone. Its continuations only check their weak reference and
		// return, so running them here is safe and keeps their task chains resolved.
		fn();
	}

	void close() {
		std::lock_guard<std::mutex> lock( mMutex );
		mTarget = nullptr;
	}

  private:
	std::mutex mMutex;
	Executor* mTarget;
};

VFSDirectoryModel::VFSDirectoryModel( Executor& uiExecutor, const Config& config ) :
	mExecutor( std::make_shared<ExecutorGate>( uiExecutor ) ), mConfig( config ) {
	if ( !mConfig.vfs )
		mConfig.vfs = &VirtualFileSystem::instance();
	if ( mConfig.firstBatchSize == 0 )
		mConfig.firstBatchSize = 1;
	if ( mConfig.batchSize == 0 )
		mConfig.batchSize = 1;
}

VFSDirectoryModel::~VFSDirectoryModel() {
	mExecutor->close();
	if ( mSession )
		mSession->cancel.cancel();
}

// ---------------------------------------------------------------------------------------------
// Navigation lifecycle
// ---------------------------------------------------------------------------------------------

void VFSDirectoryModel::setURI( URI uri ) {
	mURI = std::move( uri );
	startSession();
}

void VFSDirectoryModel::refresh() {
	if ( mURI.empty() )
		return;
	if ( !mSession || mLoadState == LoadState::Error ) {
		startSession();
		return;
	}
	rescan();
}

void VFSDirectoryModel::startSession() {
	++mGeneration;
	if ( mSession ) {
		mSession->cancel.cancel();
		// Destroying the session drops the watch: no event of the old location is delivered
		// after this point.
		mSession.reset();
	}
	mEntries.clear();
	mRows.clear();
	mError.reset();
	mItemCounts.clear();
	mCountQueue.clear();
	mCountQueued.clear();
	mCountsInFlight = 0; // Jobs of the previous generation are ignored when they finish.
	invalidate( Model::UpdateFlag::InvalidateAllIndexes );
	if ( mURI.empty() ) {
		setLoadState( LoadState::Idle );
		return;
	}
	mSession = std::make_unique<Session>();
	mSession->generation = mGeneration;
	setLoadState( LoadState::Loading );

	Uint64 generation = mGeneration;
	if ( !mConfig.watch ) {
		startListing( generation );
		return;
	}
	// The watch is established before listing so no change between the two can be missed.
	std::weak_ptr<VFSDirectoryModel> weak = weak_from_this();
	mConfig.vfs->watch( mURI, WatchOptions{} )
		.then( *mExecutor, [gate = mExecutor, weak, generation]( VFSResult<WatchHandle>&& watch ) {
			auto self = weak.lock();
			if ( !self || self->mGeneration != generation )
				return;
			self->onWatchReady( generation, std::move( watch ) );
		} );
}

void VFSDirectoryModel::onWatchReady( Uint64 generation, VFSResult<WatchHandle>&& watch ) {
	if ( watch ) {
		// A missing watch is not an error: the listing still works, it just is not live.
		mSession->watch = std::move( watch ).value();
		std::weak_ptr<VFSDirectoryModel> weak = weak_from_this();
		mSession->watch.onEvents( *mExecutor,
								  [gate = mExecutor, weak, generation]( WatchEventBatch&& batch ) {
									  auto self = weak.lock();
									  if ( self && self->mGeneration == generation )
										  self->onWatchEvents( generation, std::move( batch ) );
								  } );
	}
	startListing( generation );
}

void VFSDirectoryModel::startListing( Uint64 generation ) {
	mSession->listing = true;
	DirectoryOpenOptions options;
	options.metadata = mConfig.metadata;
	std::weak_ptr<VFSDirectoryModel> weak = weak_from_this();
	mConfig.vfs->openDirectory( mURI, options, mSession->cancel.token() )
		.then( *mExecutor,
			   [gate = mExecutor, weak, generation]( VFSResult<DirectoryHandle>&& handle ) {
				   auto self = weak.lock();
				   if ( !self || self->mGeneration != generation )
					   return;
				   if ( !handle ) {
					   self->mSession->listing = false;
					   self->setLoadState( LoadState::Error, std::move( handle ).error() );
					   return;
				   }
				   self->mSession->handle = std::move( handle ).value();
				   self->readNext( generation, true );
			   } );
}

void VFSDirectoryModel::readNext( Uint64 generation, bool first ) {
	std::weak_ptr<VFSDirectoryModel> weak = weak_from_this();
	size_t count = first ? mConfig.firstBatchSize : mConfig.batchSize;
	mSession->handle.read( count, mSession->cancel.token() )
		.then( *mExecutor,
			   [gate = mExecutor, weak, generation]( VFSResult<DirectoryBatch>&& batch ) {
				   auto self = weak.lock();
				   if ( !self || self->mGeneration != generation )
					   return;
				   if ( !batch ) {
					   self->mSession->handle.close();
					   self->mSession->listing = false;
					   self->setLoadState( LoadState::Error, std::move( batch ).error() );
					   return;
				   }
				   bool done = batch.value().endOfDirectory;
				   self->mergeBatch( std::move( batch.value().entries ) );
				   if ( done )
					   self->finishListing( generation );
				   else
					   self->readNext( generation, false );
			   } );
}

void VFSDirectoryModel::finishListing( Uint64 ) {
	mSession->handle.close();
	mSession->listing = false;
	setLoadState( LoadState::Ready );
	auto buffered = std::move( mSession->buffered );
	mSession->buffered.clear();
	if ( mSession->rescanAfterListing ) {
		mSession->rescanAfterListing = false;
		rescan();
		return;
	}
	if ( !buffered.empty() )
		applyEvents( std::move( buffered ) );
}

// ---------------------------------------------------------------------------------------------
// Watch events
// ---------------------------------------------------------------------------------------------

void VFSDirectoryModel::onWatchEvents( Uint64, WatchEventBatch&& batch ) {
	notifyChanged(); // The batch carries the current watch status.
	if ( batch.events.empty() )
		return;
	if ( mSession->listing ) {
		auto& buffered = mSession->buffered;
		for ( auto& event : batch.events ) {
			if ( event.type == WatchEventType::RescanRequired ||
				 buffered.size() >= mConfig.maxIncrementalEvents ) {
				mSession->rescanAfterListing = true;
				buffered.clear();
				break;
			}
			buffered.push_back( std::move( event ) );
		}
		return;
	}
	applyEvents( std::move( batch.events ) );
}

void VFSDirectoryModel::applyEvents( std::vector<WatchEvent>&& events ) {
	if ( events.size() > mConfig.maxIncrementalEvents ) {
		rescan();
		return;
	}
	// Coalesce per name: the stat issued afterwards reports the latest state anyway.
	std::vector<std::string> toStat;
	std::unordered_set<std::string> seen;
	auto isChild = []( const std::string& path ) {
		return !path.empty() && path.find( '/' ) == std::string::npos;
	};
	for ( auto& event : events ) {
		switch ( event.type ) {
			case WatchEventType::RescanRequired:
				rescan();
				return;
			case WatchEventType::Removed:
				if ( isChild( event.relativePath ) ) {
					removeByName( event.relativePath );
					seen.erase( event.relativePath );
				}
				break;
			case WatchEventType::Moved:
				if ( isChild( event.oldRelativePath ) )
					removeByName( event.oldRelativePath );
				[[fallthrough]];
			case WatchEventType::Added:
			case WatchEventType::Modified:
				if ( isChild( event.relativePath ) && seen.insert( event.relativePath ).second )
					toStat.push_back( event.relativePath );
				break;
		}
	}
	for ( const auto& name : toStat )
		statAndUpsert( name );
}

void VFSDirectoryModel::statAndUpsert( const std::string& name ) {
	Uint64 generation = mGeneration;
	std::weak_ptr<VFSDirectoryModel> weak = weak_from_this();
	// Describe links as links (like the listing does), with their target type resolved.
	mConfig.vfs
		->stat( mURI.withAppendedSegment( name ), StatOptions{ false }, mSession->cancel.token() )
		.then( *mExecutor,
			   [gate = mExecutor, weak, generation, name]( VFSResult<FileInfo>&& info ) {
				   auto self = weak.lock();
				   if ( !self || self->mGeneration != generation )
					   return;
				   if ( info ) {
					   self->upsert( std::move( info ).value() );
				   } else if ( info.error().is( VFSErrorCode::NotFound ) ) {
					   self->removeByName( name );
				   }
			   } );
}

void VFSDirectoryModel::rescan() {
	if ( !mSession )
		return;
	if ( mSession->listing ) {
		mSession->rescanAfterListing = true;
		mSession->buffered.clear();
		return;
	}
	mSession->listing = true;
	Uint64 generation = mGeneration;
	DirectoryOpenOptions options;
	options.metadata = mConfig.metadata;
	std::weak_ptr<VFSDirectoryModel> weak = weak_from_this();
	mConfig.vfs->listAll( mURI, options, mSession->cancel.token() )
		.then( *mExecutor,
			   [gate = mExecutor, weak, generation]( VFSResult<std::vector<FileInfo>>&& entries ) {
				   auto self = weak.lock();
				   if ( !self || self->mGeneration != generation )
					   return;
				   if ( !entries ) {
					   self->mSession->listing = false;
					   self->setLoadState( LoadState::Error, std::move( entries ).error() );
					   return;
				   }
				   self->reconcile( std::move( entries ).value() );
				   self->finishListing( generation );
			   } );
}

// ---------------------------------------------------------------------------------------------
// Entry storage
// ---------------------------------------------------------------------------------------------

bool VFSDirectoryModel::less( const FileInfo& a, const FileInfo& b, const std::string* aType,
							  const std::string* bType ) const {
	bool aDir = a.isDirectory(), bDir = b.isDirectory();
	if ( aDir != bDir )
		return aDir; // Directories first, whatever the order.
	int cmp = 0;
	// Runs O(n log n) times per sort on the UI thread: no allocations.
	auto compareNames = [&] {
		int c = String::icompare( a.name, b.name );
		return c != 0 ? c : a.name.compare( b.name );
	};
	auto compareNumbers = []( auto x, auto y ) { return x < y ? -1 : ( y < x ? 1 : 0 ); };
	switch ( mSortColumn ) {
		case Size:
			if ( aDir ) {
				cmp = compareNumbers( itemCountOf( a ).value_or( 0 ),
									  itemCountOf( b ).value_or( 0 ) );
			} else {
				cmp = compareNumbers( a.size().value_or( 0 ), b.size().value_or( 0 ) );
			}
			break;
		case Type:
			cmp = aType && bType ? aType->compare( *bType ) : typeOf( a ).compare( typeOf( b ) );
			break;
		case Modified:
			cmp = compareNumbers( a.modifiedTime().value_or( 0 ), b.modifiedTime().value_or( 0 ) );
			break;
		case Permissions:
			cmp = compareNumbers( a.permissions().value_or( 0 ), b.permissions().value_or( 0 ) );
			break;
		case Owner:
			cmp = a.owner.compare( b.owner );
			break;
		case Group:
			cmp = a.group.compare( b.group );
			break;
		default:
			break;
	}
	if ( cmp == 0 )
		cmp = compareNames();
	return mSortOrder == SortOrder::Descending ? cmp > 0 : cmp < 0;
}

void VFSDirectoryModel::sortEntries( std::vector<FileInfo>& entries ) const {
	if ( mSortColumn != Type ) {
		std::stable_sort( entries.begin(), entries.end(),
						  [this]( const FileInfo& a, const FileInfo& b ) { return less( a, b ); } );
		return;
	}
	// The type provider (a std::function returning a string) runs once per entry, not twice per
	// comparison.
	std::vector<std::string> types;
	types.reserve( entries.size() );
	for ( const auto& entry : entries )
		types.push_back( typeOf( entry ) );
	std::vector<size_t> order( entries.size() );
	for ( size_t i = 0; i < order.size(); ++i )
		order[i] = i;
	std::stable_sort( order.begin(), order.end(), [&]( size_t x, size_t y ) {
		return less( entries[x], entries[y], &types[x], &types[y] );
	} );
	std::vector<FileInfo> sorted;
	sorted.reserve( entries.size() );
	for ( size_t index : order )
		sorted.push_back( std::move( entries[index] ) );
	entries.swap( sorted );
}

void VFSDirectoryModel::rowsInserted( size_t entryIndex, bool visible ) {
	size_t row = visibleRowFor( entryIndex );
	for ( size_t i = row; i < mRows.size(); ++i )
		++mRows[i];
	if ( visible )
		mRows.insert( mRows.begin() + row, entryIndex );
}

void VFSDirectoryModel::rowsErased( size_t entryIndex ) {
	size_t row = visibleRowFor( entryIndex );
	if ( row < mRows.size() && mRows[row] == entryIndex )
		mRows.erase( mRows.begin() + row );
	for ( size_t i = row; i < mRows.size(); ++i )
		--mRows[i];
}

void VFSDirectoryModel::rebuildRows() {
	mRows.clear();
	mRows.reserve( mEntries.size() );
	for ( size_t i = 0; i < mEntries.size(); ++i ) {
		if ( isVisible( mEntries[i] ) )
			mRows.push_back( i );
	}
}

size_t VFSDirectoryModel::visibleRowFor( size_t entryIndex ) const {
	return static_cast<size_t>( std::lower_bound( mRows.begin(), mRows.end(), entryIndex ) -
								mRows.begin() );
}

void VFSDirectoryModel::mergeBatch( std::vector<FileInfo>&& batch ) {
	if ( batch.empty() )
		return;
	for ( const auto& info : batch )
		queueItemCount( info );
	auto cmp = [this]( const FileInfo& a, const FileInfo& b ) { return less( a, b ); };
	sortEntries( batch );
	if ( mEntries.empty() ) {
		mEntries = std::move( batch );
	} else {
		std::vector<FileInfo> merged;
		merged.reserve( mEntries.size() + batch.size() );
		std::merge( std::make_move_iterator( mEntries.begin() ),
					std::make_move_iterator( mEntries.end() ),
					std::make_move_iterator( batch.begin() ),
					std::make_move_iterator( batch.end() ), std::back_inserter( merged ), cmp );
		mEntries = std::move( merged );
	}
	rebuildRows();
	// Rows shift while the initial listing streams in; no selection exists yet.
	invalidate( Model::UpdateFlag::InvalidateAllIndexes );
	notifyChanged();
	pumpItemCounts();
}

void VFSDirectoryModel::removeByName( const std::string& name ) {
	auto it = std::find_if( mEntries.begin(), mEntries.end(),
							[&name]( const FileInfo& info ) { return info.name == name; } );
	if ( it == mEntries.end() )
		return;
	size_t index = static_cast<size_t>( it - mEntries.begin() );
	bool visible = isVisible( *it );
	mItemCounts.erase( name );
	// Rows are updated in place: watch events arrive one by one, a full rebuild would cost a
	// pass over every entry each time.
	if ( visible ) {
		int row = static_cast<int>( visibleRowFor( index ) );
		beginDeleteRows( {}, row, row );
		mEntries.erase( it );
		rowsErased( index );
		endDeleteRows();
	} else {
		mEntries.erase( it );
		rowsErased( index );
	}
	onModelUpdate( Model::UpdateFlag::DontInvalidateIndexes );
	notifyChanged();
}

void VFSDirectoryModel::upsert( FileInfo&& info ) {
	if ( info.isDirectory() && mConfig.countDirectoryItems ) {
		// The directory changed: its count is stale.
		mItemCounts.erase( info.name );
		queueItemCount( info );
		pumpItemCounts();
	}
	auto cmp = [this]( const FileInfo& a, const FileInfo& b ) { return less( a, b ); };
	auto it = std::find_if( mEntries.begin(), mEntries.end(),
							[&info]( const FileInfo& e ) { return e.name == info.name; } );
	if ( it != mEntries.end() ) {
		size_t index = static_cast<size_t>( it - mEntries.begin() );
		bool orderKept = ( index == 0 || !cmp( info, mEntries[index - 1] ) ) &&
						 ( index + 1 == mEntries.size() || !cmp( mEntries[index + 1], info ) );
		if ( orderKept && isVisible( *it ) == isVisible( info ) ) {
			// In-place update keeps the row (and its selection).
			*it = std::move( info );
			onModelUpdate( Model::UpdateFlag::DontInvalidateIndexes );
			notifyChanged();
			return;
		}
		// Copy: removeByName() erases the element the name lives in.
		std::string name = it->name;
		removeByName( name );
	}
	auto pos = std::upper_bound( mEntries.begin(), mEntries.end(), info, cmp );
	size_t index = static_cast<size_t>( pos - mEntries.begin() );
	if ( isVisible( info ) ) {
		int row = static_cast<int>( visibleRowFor( index ) );
		beginInsertRows( {}, row, row );
		mEntries.insert( pos, std::move( info ) );
		rowsInserted( index, true );
		endInsertRows();
	} else {
		mEntries.insert( pos, std::move( info ) );
		rowsInserted( index, false );
	}
	onModelUpdate( Model::UpdateFlag::DontInvalidateIndexes );
	notifyChanged();
}

void VFSDirectoryModel::reconcile( std::vector<FileInfo>&& entries ) {
	// Large re-lists are replaced wholesale; row-level diffing would be quadratic.
	constexpr size_t MaxIncrementalReconcile = 4096;
	if ( entries.size() > MaxIncrementalReconcile || mEntries.size() > MaxIncrementalReconcile ) {
		mEntries.clear();
		mergeBatch( std::move( entries ) );
		return;
	}
	std::unordered_map<std::string, size_t> fresh;
	for ( size_t i = 0; i < entries.size(); ++i )
		fresh.emplace( entries[i].name, i );
	std::vector<std::string> gone;
	for ( const auto& entry : mEntries ) {
		if ( !fresh.count( entry.name ) )
			gone.push_back( entry.name );
	}
	for ( const auto& name : gone )
		removeByName( name );
	for ( auto& entry : entries )
		upsert( std::move( entry ) );
}

void VFSDirectoryModel::setLoadState( LoadState state, std::optional<VFSError> error ) {
	mLoadState = state;
	mError = std::move( error );
	notifyChanged();
}

void VFSDirectoryModel::notifyChanged() {
	if ( mOnStateChanged )
		mOnStateChanged();
}

std::optional<WatchStatus> VFSDirectoryModel::watchStatus() const {
	if ( !mSession || !mSession->watch.isValid() )
		return std::nullopt;
	return mSession->watch.status();
}

void VFSDirectoryModel::setShowHidden( bool show ) {
	if ( mConfig.showHidden == show )
		return;
	mConfig.showHidden = show;
	rebuildRows();
	invalidate( Model::UpdateFlag::InvalidateAllIndexes );
}

const FileInfo& VFSDirectoryModel::entry( size_t row ) const {
	return mEntries[mRows[row]];
}

void VFSDirectoryModel::setTypeProvider( TextProvider provider ) {
	mTypeProvider = std::move( provider );
	invalidate( Model::UpdateFlag::DontInvalidateIndexes );
}

void VFSDirectoryModel::setTimeFormatter( TimeFormatter formatter ) {
	mTimeFormatter = std::move( formatter );
	invalidate( Model::UpdateFlag::DontInvalidateIndexes );
}

void VFSDirectoryModel::setItemCountFormatter( ItemCountFormatter formatter ) {
	mItemCountFormatter = std::move( formatter );
	invalidate( Model::UpdateFlag::DontInvalidateIndexes );
}

std::string VFSDirectoryModel::typeOf( const FileInfo& info ) const {
	if ( mTypeProvider )
		return mTypeProvider( info );
	return info.isDirectory() ? "Folder" : "File";
}

// ---------------------------------------------------------------------------------------------
// Directory item counts
// ---------------------------------------------------------------------------------------------

void VFSDirectoryModel::setCountDirectoryItems( bool count ) {
	if ( mConfig.countDirectoryItems == count )
		return;
	mConfig.countDirectoryItems = count;
	if ( !count ) {
		mItemCounts.clear();
		mCountQueue.clear();
		mCountQueued.clear();
	} else {
		for ( const auto& entry : mEntries )
			queueItemCount( entry );
		pumpItemCounts();
	}
	invalidate( Model::UpdateFlag::DontInvalidateIndexes );
}

std::optional<Uint64> VFSDirectoryModel::itemCountOf( const FileInfo& info ) const {
	auto it = mItemCounts.find( info.name );
	if ( it == mItemCounts.end() )
		return std::nullopt;
	return mConfig.showHidden ? it->second.all : it->second.visible;
}

std::optional<Uint64> VFSDirectoryModel::directoryItemCount( size_t row ) const {
	if ( row >= mRows.size() )
		return std::nullopt;
	return itemCountOf( mEntries[mRows[row]] );
}

void VFSDirectoryModel::queueItemCount( const FileInfo& info ) {
	if ( !mConfig.countDirectoryItems || !info.isDirectory() || !mSession )
		return;
	if ( mCountQueued.insert( info.name ).second )
		mCountQueue.push_back( info.name );
}

void VFSDirectoryModel::pumpItemCounts() {
	if ( !mSession )
		return;
	while ( mCountsInFlight < std::max<size_t>( 1, mConfig.maxConcurrentItemCounts ) &&
			!mCountQueue.empty() ) {
		std::string name = std::move( mCountQueue.front() );
		mCountQueue.pop_front();
		mCountQueued.erase( name );
		++mCountsInFlight;
		Uint64 generation = mGeneration;
		std::weak_ptr<VFSDirectoryModel> weak = weak_from_this();
		DirectoryOpenOptions options{ DirectoryMetadata::NamesAndTypes, false };
		CancellationToken token = mSession->cancel.token();
		mConfig.vfs->openDirectory( mURI.withAppendedSegment( name ), options, token )
			.then( [token]( VFSResult<DirectoryHandle>&& handle ) -> VFSTask<ItemCount> {
				if ( !handle ) {
					return makeReadyTask(
						VFSResult<ItemCount>( Err( std::move( handle ).error() ) ) );
				}
				auto state = std::make_shared<CountState>();
				state->handle = std::move( handle ).value();
				state->token = token;
				return readCount<ItemCount>( std::move( state ) );
			} )
			.then( *mExecutor, [gate = mExecutor, weak, generation,
								name]( VFSResult<ItemCount>&& count ) mutable {
				auto self = weak.lock();
				if ( !self || self->mGeneration != generation )
					return;
				self->onItemCounted( name, std::move( count ) );
			} );
	}
}

void VFSDirectoryModel::onItemCounted( const std::string& name, VFSResult<ItemCount>&& count ) {
	if ( mCountsInFlight > 0 )
		--mCountsInFlight;
	// Counting may have been turned off while this one ran.
	if ( count && mConfig.countDirectoryItems )
		mItemCounts[name] = count.value();
	pumpItemCounts();
	// Many counts finish in the same frame: refresh the view once for all of them.
	if ( mCountRefreshPending )
		return;
	mCountRefreshPending = true;
	Uint64 generation = mGeneration;
	std::weak_ptr<VFSDirectoryModel> weak = weak_from_this();
	mExecutor->execute( [gate = mExecutor, weak, generation] {
		auto self = weak.lock();
		if ( !self )
			return;
		self->mCountRefreshPending = false;
		if ( self->mGeneration == generation )
			self->onModelUpdate( Model::UpdateFlag::DontInvalidateIndexes );
	} );
}

// ---------------------------------------------------------------------------------------------
// Name filter
// ---------------------------------------------------------------------------------------------

bool VFSDirectoryModel::isVisible( const FileInfo& info ) const {
	if ( !mConfig.showHidden && info.hidden )
		return false;
	return mFilterPattern.empty() || matchesNameFilter( info.name );
}

bool VFSDirectoryModel::matchesNameFilter( const std::string& name ) const {
	switch ( mFilterMode ) {
		case NameFilterMode::PlainText:
			if ( mFilterCaseSensitive )
				return name.find( mFilterPattern ) != std::string::npos;
			// Every entry is matched on each rebuild: no lowercase copies.
			return String::BMH::find( std::string_view( name ), mFilterPattern, 0,
									  mFilterOccurrences, true ) != -1;
		case NameFilterMode::Glob:
			return String::globMatch( name, mFilterGlob, !mFilterCaseSensitive );
		case NameFilterMode::RegEx:
			return !mFilterRegEx || !mFilterValid || mFilterRegEx->matches( name );
	}
	return true;
}

void VFSDirectoryModel::setNameFilter( std::string pattern, NameFilterMode mode,
									   bool caseSensitive ) {
	if ( pattern == mFilterPattern && mode == mFilterMode && caseSensitive == mFilterCaseSensitive )
		return;
	mFilterPattern = std::move( pattern );
	mFilterMode = mode;
	mFilterCaseSensitive = caseSensitive;
	mFilterRegEx.reset();
	mFilterValid = true;
	mFilterOccurrences.clear();
	if ( mode == NameFilterMode::PlainText && !caseSensitive && !mFilterPattern.empty() ) {
		mFilterOccurrences = String::BMH::createOccTable(
			reinterpret_cast<const unsigned char*>( mFilterPattern.data() ), mFilterPattern.size(),
			true );
	}
	// Unanchored like Dolphin ("md" matches "notes.md"). A doubled '*' would mean "**" (any
	// path) to globMatch, so only missing wildcards are added.
	mFilterGlob.clear();
	if ( mode == NameFilterMode::Glob && !mFilterPattern.empty() ) {
		if ( mFilterPattern.front() != '*' )
			mFilterGlob += '*';
		mFilterGlob += mFilterPattern;
		if ( mFilterPattern.back() != '*' )
			mFilterGlob += '*';
	}
	if ( mode == NameFilterMode::RegEx && !mFilterPattern.empty() ) {
		Uint32 options = System::RegEx::Options::Utf;
		if ( !caseSensitive )
			options |= System::RegEx::Options::Caseless;
		mFilterRegEx = std::make_unique<System::RegEx>( mFilterPattern, options, false );
		mFilterValid = mFilterRegEx->isValid();
	}
	rebuildRows();
	invalidate( Model::UpdateFlag::InvalidateAllIndexes );
	notifyChanged();
}

size_t VFSDirectoryModel::filteredOutCount() const {
	if ( mFilterPattern.empty() )
		return 0;
	size_t count = 0;
	for ( const auto& info : mEntries ) {
		if ( ( mConfig.showHidden || !info.hidden ) && !matchesNameFilter( info.name ) )
			++count;
	}
	return count;
}

std::optional<size_t> VFSDirectoryModel::rowOf( std::string_view name ) const {
	for ( size_t row = 0; row < mRows.size(); ++row ) {
		if ( mEntries[mRows[row]].name == name )
			return row;
	}
	return std::nullopt;
}

// ---------------------------------------------------------------------------------------------
// Model
// ---------------------------------------------------------------------------------------------

size_t VFSDirectoryModel::rowCount( const ModelIndex& parent ) const {
	return parent.isValid() ? 0 : mRows.size();
}

std::string VFSDirectoryModel::columnName( const size_t& column ) const {
	switch ( column ) {
		case Name:
			return "Name";
		case Size:
			return "Size";
		case Modified:
			return "Modified";
		case Type:
			return "Type";
		case Permissions:
			return "Permissions";
		case Owner:
			return "Owner";
		case Group:
			return "Group";
		default:
			return {};
	}
}

std::string VFSDirectoryModel::permissionsToString( const FileInfo& info ) {
	std::string out( 10, '-' );
	switch ( info.type ) {
		case FileType::Directory:
			out[0] = 'd';
			break;
		case FileType::Symlink:
			out[0] = 'l';
			break;
		case FileType::BlockDevice:
			out[0] = 'b';
			break;
		case FileType::CharacterDevice:
			out[0] = 'c';
			break;
		case FileType::FIFO:
			out[0] = 'p';
			break;
		case FileType::Socket:
			out[0] = 's';
			break;
		default:
			break;
	}
	auto perms = info.permissions();
	if ( !perms )
		return out;
	static constexpr char Letters[] = "rwxrwxrwx";
	for ( int i = 0; i < 9; ++i ) {
		if ( *perms & ( 1u << ( 8 - i ) ) )
			out[i + 1] = Letters[i];
	}
	if ( *perms & 04000 )
		out[3] = ( *perms & 0100 ) ? 's' : 'S';
	if ( *perms & 02000 )
		out[6] = ( *perms & 0010 ) ? 's' : 'S';
	if ( *perms & 01000 )
		out[9] = ( *perms & 0001 ) ? 't' : 'T';
	return out;
}

bool VFSDirectoryModel::isEditable( const ModelIndex& index ) const {
	return mOnRename && index.column() == Name && index.row() >= 0 &&
		   static_cast<size_t>( index.row() ) < rowCount();
}

void VFSDirectoryModel::setData( const ModelIndex& index, const Variant& data ) {
	if ( !isEditable( index ) )
		return;
	const FileInfo& info = entry( static_cast<size_t>( index.row() ) );
	std::string name = data.toString();
	if ( name.empty() || name == info.name )
		return;
	// Copy: the handler may refresh the listing, which replaces the entries.
	FileInfo entryCopy = info;
	mOnRename( entryCopy, name );
}

Variant VFSDirectoryModel::data( const ModelIndex& index, ModelRole role ) const {
	if ( !index.isValid() || static_cast<size_t>( index.row() ) >= mRows.size() )
		return {};
	const FileInfo& info = mEntries[mRows[index.row()]];
	if ( role == ModelRole::Icon ) {
		if ( index.column() == Name && mIconProvider )
			return mIconProvider( info );
		return {};
	}
	if ( role == ModelRole::Class ) {
		// Per column classes, for styling (for example right aligned sizes).
		static const std::string Classes[ColumnCount] = {
			"vfs_name",		   "vfs_size",	"vfs_modified", "vfs_type",
			"vfs_permissions", "vfs_owner", "vfs_group" };
		static const std::string FolderName = "vfs_folder";
		if ( index.column() == Name && info.isDirectory() )
			return Variant( &FolderName );
		return index.column() < ColumnCount ? Variant( &Classes[index.column()] ) : Variant();
	}
	if ( role != ModelRole::Display && role != ModelRole::Sort )
		return {};
	switch ( index.column() ) {
		case Name:
			return Variant::fromRef( info.name );
		case Size: {
			if ( info.isDirectory() ) {
				auto count = itemCountOf( info );
				if ( role == ModelRole::Sort )
					return Variant( count.value_or( 0 ) );
				if ( !count )
					return Variant( std::string() );
				return Variant( mItemCountFormatter ? mItemCountFormatter( *count )
													: defaultItemCount( *count ) );
			}
			if ( role == ModelRole::Sort )
				return Variant( info.size().value_or( 0 ) );
			if ( !info.size() )
				return Variant( std::string() );
			return Variant(
				System::FileSystem::sizeToString( static_cast<Int64>( *info.size() ) ) );
		}
		case Modified:
			if ( role == ModelRole::Sort )
				return Variant( info.modifiedTime().value_or( 0 ) );
			if ( !info.modifiedTime() )
				return Variant( std::string() );
			if ( mTimeFormatter )
				return Variant( mTimeFormatter( *info.modifiedTime() ) );
			return Variant(
				System::Sys::epochToString( static_cast<Uint64>( *info.modifiedTime() ) ) );
		case Type:
			return Variant( typeOf( info ) );
		case Permissions:
			return Variant( permissionsToString( info ) );
		case Owner:
			return Variant::fromRef( info.owner );
		case Group:
			return Variant::fromRef( info.group );
		default:
			return {};
	}
}

void VFSDirectoryModel::sort( const size_t& column, const SortOrder& order ) {
	mSortColumn = column < ColumnCount ? column : Name;
	mSortOrder = order == SortOrder::None ? SortOrder::Ascending : order;
	sortEntries( mEntries );
	rebuildRows();
	invalidate( Model::UpdateFlag::InvalidateAllIndexes );
}

}} // namespace EE::VFS
