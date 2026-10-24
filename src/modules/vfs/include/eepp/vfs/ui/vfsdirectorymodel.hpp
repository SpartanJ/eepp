#ifndef EE_VFS_UI_VFSDIRECTORYMODEL_HPP
#define EE_VFS_UI_VFSDIRECTORYMODEL_HPP

#include <deque>
#include <eepp/core/executor.hpp>
#include <eepp/system/regex.hpp>
#include <eepp/ui/models/model.hpp>
#include <eepp/ui/models/variant.hpp>
#include <eepp/vfs/config.hpp>
#include <eepp/vfs/virtualfilesystem.hpp>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace EE { namespace VFS {

using UI::Models::Model;
using UI::Models::ModelIndex;
using UI::Models::ModelRole;
using UI::Models::SortOrder;
using UI::Models::Variant;

/** @brief Async-native model of the entries of one directory URI (any VFS scheme).
 *
 * Built for file managers: it shows one location, enumerates it incrementally, stays live through
 * VFS watches and never blocks or mutates from a non-UI thread.
 *
 * Navigation lifecycle (setURI):
 *   generation++ -> cancel the previous listing -> drop the previous watch -> clear -> Loading ->
 *   establish the watch -> open the directory -> read batches (delivered on the UI executor and
 *   merged incrementally) -> Ready -> replay watch events buffered during the listing -> live.
 *
 * Results from a previous generation are discarded. Watch events are applied idempotently (the
 * listing and the events are not a transactional snapshot); bursts larger than
 * Config::maxIncrementalEvents and RescanRequired trigger a reconciling re-list that keeps rows
 * (and therefore selection) of entries that still exist.
 *
 * All public methods must be called on the UI thread, which must be the thread the executor runs
 * functions on. The executor must outlive the model. InlineExecutor is suitable only when
 * the provider also completes on the UI thread.
 */
class EE_VFS_API VFSDirectoryModel : public Model,
									 public std::enable_shared_from_this<VFSDirectoryModel> {
  public:
	enum Column : Uint8 { Name, Size, Modified, Type, Permissions, Owner, Group, ColumnCount };

	/** How setNameFilter() patterns match entry names (all of them unanchored). */
	enum class NameFilterMode : Uint8 { PlainText, Glob, RegEx };

	enum class LoadState : Uint8 { Idle, Loading, Ready, Error };

	struct Config {
		/** Defaults to VirtualFileSystem::instance(). */
		VirtualFileSystem* vfs{ nullptr };
		/** The first batch is small so the first rows appear quickly. */
		size_t firstBatchSize{ 128 };
		size_t batchSize{ 2048 };
		/** Above this many pending events, re-list instead of applying them one by one. */
		size_t maxIncrementalEvents{ 256 };
		/** Directory counts running at the same time. */
		size_t maxConcurrentItemCounts{ 4 };
		DirectoryMetadata metadata{ DirectoryMetadata::Full };
		bool showHidden{ false };
		bool watch{ true };
		/** Count the entries of each listed directory in the background (shown in the Size
		 * column). Each count lists a directory, so enable it where listings are cheap. */
		bool countDirectoryItems{ false };
	};

	using IconProvider = std::function<Variant( const VFSFileInfo& )>;

	/** Text for an entry (used by the Type column). */
	using TextProvider = std::function<std::string( const VFSFileInfo& )>;

	/** Formats a Unix timestamp in seconds (used by the Modified column). */
	using TimeFormatter = std::function<std::string( Int64 )>;

	/** Formats the number of entries of a directory (used by the Size column). */
	using ItemCountFormatter = std::function<std::string( Uint64 )>;

	static std::shared_ptr<VFSDirectoryModel> New( Executor& uiExecutor );

	static std::shared_ptr<VFSDirectoryModel> New( Executor& uiExecutor, const Config& config );

	~VFSDirectoryModel() override;

	VFSDirectoryModel( const VFSDirectoryModel& ) = delete;

	VFSDirectoryModel& operator=( const VFSDirectoryModel& ) = delete;

	/** Navigates to a directory. */
	void setURI( URI uri );

	const URI& uri() const { return mURI; }

	/** Re-lists the current location, keeping rows of entries that still exist. */
	void refresh();

	LoadState loadState() const { return mLoadState; }

	/** The last listing error, when loadState() is Error. */
	const std::optional<VFSError>& error() const { return mError; }

	/** @return The watch status, or nullopt when the location is not watched. */
	std::optional<WatchStatus> watchStatus() const;

	bool showHidden() const { return mConfig.showHidden; }

	void setShowHidden( bool show );

	/** Visible entry for a row. */
	const VFSFileInfo& entry( size_t row ) const;

	/** URI of the visible entry for a row. */
	URI entryURI( size_t row ) const { return entry( row ).uriIn( mURI ); }

	std::optional<size_t> rowOf( std::string_view name ) const;

	void setIconProvider( IconProvider provider ) { mIconProvider = std::move( provider ); }

	/** Sets the Type column text. Defaults to "Folder" / "File". Called often while sorting by
	 * type, so it should be cheap (cache by extension). */
	void setTypeProvider( TextProvider provider );

	void setTimeFormatter( TimeFormatter formatter );

	void setItemCountFormatter( ItemCountFormatter formatter );

	/** Enables or disables Config::countDirectoryItems. Turning it off drops the counts; turning
	 * it on counts the folders listed. */
	void setCountDirectoryItems( bool count );

	bool countDirectoryItems() const { return mConfig.countDirectoryItems; }

	/** @return The number of visible entries of the directory in row, once counted (requires
	 * Config::countDirectoryItems). */
	std::optional<Uint64> directoryItemCount( size_t row ) const;

	/** Shows only entries whose name matches pattern (an empty pattern shows everything). */
	void setNameFilter( std::string pattern, NameFilterMode mode = NameFilterMode::Glob,
						bool caseSensitive = false );

	const std::string& nameFilter() const { return mFilterPattern; }

	NameFilterMode nameFilterMode() const { return mFilterMode; }

	bool nameFilterCaseSensitive() const { return mFilterCaseSensitive; }

	/** @return False when the current RegEx filter does not compile (it is then ignored). */
	bool isNameFilterValid() const { return mFilterValid; }

	/** Number of entries hidden by the name filter (not counting hidden files). */
	size_t filteredOutCount() const;

	/** Called on the UI thread whenever the load state, error, watch status or the entries
	 * change. */
	void setOnStateChanged( std::function<void()> callback ) {
		mOnStateChanged = std::move( callback );
	}

	/** Handles a rename typed in place: the entry and its new name. The model does not rename:
	 * the handler performs it (asynchronously) and the listing follows through the watch or a
	 * refresh. */
	using RenameHandler = std::function<void( const VFSFileInfo& entry, const std::string& name )>;

	/** Makes names editable in place (the view's editing delegate) and sets who renames. */
	void setOnRename( RenameHandler handler ) { mOnRename = std::move( handler ); }

	// Model

	size_t rowCount( const ModelIndex& = ModelIndex() ) const override;

	size_t columnCount( const ModelIndex& = ModelIndex() ) const override { return ColumnCount; }

	std::string columnName( const size_t& column ) const override;

	Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const override;

	bool isSortable() override { return true; }

	/** Cells get a per column class: vfs_name (vfs_folder for folders), vfs_size, vfs_modified,
	 * vfs_type... */
	bool classModelRoleEnabled() override { return true; }

	void sort( const size_t& column, const SortOrder& order ) override;

	SortOrder sortOrder() const override { return mSortOrder; }

	int keyColumn() const override { return static_cast<int>( mSortColumn ); }

	/** Type-to-select always matches names, whatever the sort column (as file managers do). */
	int searchColumn() const override { return Name; }

	/** Names are editable when a rename handler is set. */
	bool isEditable( const ModelIndex& index ) const override;

	/** A name edited in place: passed to the rename handler (unchanged names are ignored). */
	void setData( const ModelIndex& index, const Variant& data ) override;

	/** Formats POSIX permission bits like `ls -l` ("drwxr-xr-x"). */
	static std::string permissionsToString( const VFSFileInfo& info );

  protected:
	/** Subclasses must be owned by a shared_ptr before navigation starts. */
	VFSDirectoryModel( Executor& uiExecutor, const Config& config );

  private:
	struct Session;

	void startSession();

	void onWatchReady( Uint64 generation, VFSResult<WatchHandle>&& watch );

	void startListing( Uint64 generation );

	void readNext( Uint64 generation, bool first );

	void finishListing( Uint64 generation );

	void onWatchEvents( Uint64 generation, WatchEventBatch&& batch );

	void applyEvents( std::vector<WatchEvent>&& events );

	void statAndUpsert( const std::string& name );

	void upsert( VFSFileInfo&& info );

	void removeByName( const std::string& name );

	void rescan();

	void reconcile( std::vector<VFSFileInfo>&& entries );

	void mergeBatch( std::vector<VFSFileInfo>&& batch );

	void rebuildRows();

	/** Updates mRows for an entry inserted at entryIndex (already in mEntries). */
	void rowsInserted( size_t entryIndex, bool visible );

	/** Updates mRows for the entry erased from entryIndex. */
	void rowsErased( size_t entryIndex );

	/** Sorts entries by the current order (stable). */
	void sortEntries( std::vector<VFSFileInfo>& entries ) const;

	void setLoadState( LoadState state, std::optional<VFSError> error = std::nullopt );

	void notifyChanged();

	/** The sort order. aType and bType are the entries' types when already known. */
	bool less( const VFSFileInfo& a, const VFSFileInfo& b, const std::string* aType = nullptr,
			   const std::string* bType = nullptr ) const;

	bool isVisible( const VFSFileInfo& info ) const;

	bool matchesNameFilter( const std::string& name ) const;

	struct ItemCount {
		Uint64 all{ 0 };
		Uint64 visible{ 0 };
	};

	void queueItemCount( const VFSFileInfo& info );

	void pumpItemCounts();

	void onItemCounted( const std::string& name, VFSResult<ItemCount>&& count );

	std::optional<Uint64> itemCountOf( const VFSFileInfo& info ) const;

	std::string typeOf( const VFSFileInfo& info ) const;

	size_t visibleRowFor( size_t entryIndex ) const;

	class ExecutorGate;

	/** Forwards to the UI executor while the model is alive. Continuations hold it (not the UI
	 * executor), so operations that finish after the model is destroyed never touch an executor
	 * that may already be gone. */
	std::shared_ptr<ExecutorGate> mExecutor;
	Config mConfig;
	URI mURI;
	std::unique_ptr<Session> mSession;
	/** All entries (including hidden ones), kept sorted. */
	std::vector<VFSFileInfo> mEntries;
	/** Visible rows: indices into mEntries. */
	std::vector<size_t> mRows;
	IconProvider mIconProvider;
	TextProvider mTypeProvider;
	TimeFormatter mTimeFormatter;
	ItemCountFormatter mItemCountFormatter;
	std::function<void()> mOnStateChanged;
	RenameHandler mOnRename;
	std::unordered_map<std::string, ItemCount> mItemCounts;
	std::deque<std::string> mCountQueue;
	std::unordered_set<std::string> mCountQueued;
	size_t mCountsInFlight{ 0 };
	std::string mFilterPattern;
	std::string mFilterGlob;
	/** Case-insensitive plain text filter: the pattern's search table. */
	String::BMH::OccTable mFilterOccurrences;
	std::unique_ptr<System::RegEx> mFilterRegEx;
	std::optional<VFSError> mError;
	Uint64 mGeneration{ 0 };
	size_t mSortColumn{ Name };
	SortOrder mSortOrder{ SortOrder::Ascending };
	LoadState mLoadState{ LoadState::Idle };
	NameFilterMode mFilterMode{ NameFilterMode::Glob };
	bool mCountRefreshPending{ false };
	bool mFilterCaseSensitive{ false };
	bool mFilterValid{ true };
};

}} // namespace EE::VFS

#endif
