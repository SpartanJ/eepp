#ifndef EEFM_APP_HPP
#define EEFM_APP_HPP

#include "dialogs.hpp"
#include "fileview.hpp"
#include "places.hpp"
#include "placespanel.hpp"

#include <eepp/ee.hpp>
#include <eepp/scene/mainthreadexecutor.hpp>
#include <eepp/vfs.hpp>
#include <eepp/vfs/ui/vfsdirectorymodel.hpp>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>

namespace eefm {

using namespace EE;
using namespace EE::Scene;
using namespace EE::UI;
using namespace EE::UI::Models;
using namespace EE::Window;
using EE::System::FileSystem;
using EE::System::Sys;
using EE::System::ThreadPool;
// Declared inside the namespace so they win over names other headers inject globally (for
// example System::FileInfo).
using EE::VFS::CopyOptions;
using EE::VFS::FileInfo;
using EE::VFS::RemoveOptions;
using EE::VFS::Transfer;
using EE::VFS::TransferProgress;
using EE::VFS::TransferProgressSubscription;
using EE::VFS::TransferResult;
using EE::VFS::VFSDirectoryModel;
using EE::VFS::VFSError;
using EE::VFS::VFSErrorCode;
using EE::VFS::VFSResult;
using EE::VFS::VirtualFileSystem;
using EE::VFS::WatchStatus;

struct Pane;

/** One tab: a directory view with its own history and filter. */
struct FileTab {
	Pane* pane{ nullptr };
	UITab* tab{ nullptr };
	FileTableView* view{ nullptr };
	std::shared_ptr<VFSDirectoryModel> model;
	std::vector<URI> history;
	size_t historyIndex{ 0 };
	/** Filter text of this tab (the filter bar shows the current tab's). */
	std::string filter;
	/** Entry to select once it is listed (the folder we came from when going up). */
	std::string selectWhenListed;
};

/** Location bar of a pane (lives in the toolbar, aligned with its pane). */
struct LocationBar {
	UIWidget* box{ nullptr };
	UITextInput* input{ nullptr };
};

/** One side of the split view: a tab widget with its status and filter bars. */
struct Pane {
	int index{ 0 };
	UIWidget* root{ nullptr };
	UITabWidget* tabs{ nullptr };
	UIPushButton* newTabButton{ nullptr };
	UITextView* status{ nullptr };
	UIWidget* transferBox{ nullptr };
	UIProgressBar* transferBar{ nullptr };
	UITextView* transferText{ nullptr };
	UIWidget* filterBar{ nullptr };
	UITextInput* filterInput{ nullptr };
	UIPushButton* filterLock{ nullptr };
	UIPushButton* filterCase{ nullptr };
	UIDropDownList* filterMode{ nullptr };
	LocationBar location;
	std::vector<std::unique_ptr<FileTab>> tabList;
	bool filterLocked{ false };
};

struct ActiveTransfer {
	Uint64 id;
	int pane;
	String label;
	Transfer transfer;
	TransferProgressSubscription subscription;
	TransferProgress progress;
};

class App {
  public:
	int run( int argc, char* argv[] );

  private:
	// --- app.cpp: setup, layout, state, shortcuts
	bool init( const std::vector<std::string>& locations, bool forceSplit );

	void loadState();

	void saveState();

	/** Saves the state and closes the window. */
	void quit();

	void restoreTabs( const std::vector<std::string>& locations, bool forceSplit );

	void registerShortcuts();

	void showAppMenu();

	void setSplit( bool split );

	void setPlacesVisible( bool visible );

	void setFilterBarVisible( bool visible );

	void setShowHidden( bool show );

	void layoutLocationBars();

	void applyViewState( FileTab* tab );

	void rememberViewState( FileTab* tab );

	// --- panes.cpp: panes, tabs, navigation, status, filter
	void createPane( int index, UIWidget* parent );

	Pane& pane( int index ) { return *mPanes[index]; }

	Pane& activePane() { return *mPanes[mActivePane]; }

	FileTab* currentTab( Pane& pane );

	FileTab* currentTab() { return mPanes[mActivePane] ? currentTab( activePane() ) : nullptr; }

	FileTab* findTab( UITab* tab );

	FileTab* openTab( Pane& pane, const URI& uri, bool select = true );

	void closeTab( FileTab* tab );

	void setActivePane( int index );

	void navigate( FileTab* tab, URI uri, bool recordHistory = true );

	void goBack();

	void goForward();

	void goUp();

	void goHome();

	void updateChrome( Pane& pane );

	void updateStatus( Pane& pane );

	void updateTabTitle( FileTab* tab );

	void applyFilter( FileTab* tab );

	void onTabStateChanged( FileTab* tab );

	void positionNewTabButton( Pane& pane );

	void showLocationHistory( Pane& pane );

	static URI parseLocation( const std::string& text );

	static String displayName( const URI& uri );

	String locationText( const URI& uri ) const;

	/** A location for Copy Location: a plain path (no trailing slash) for local files and
	 * folders, the URI otherwise. Copying files (Ctrl+C) puts URIs in the clipboard instead. */
	std::string clipboardText( const URI& uri ) const;

	// --- fileops.cpp: actions on files
	std::vector<URI> selectedURIs( FileTab* tab ) const;

	std::vector<size_t> selectedRows( FileTab* tab ) const;

	void openRow( FileTab* tab, size_t row );

	void openFiles( const std::vector<URI>& uris );

	void openWith( const std::vector<URI>& uris );

	void showContextMenu( FileTab* tab, bool background );

	void showHeaderMenu( FileTab* tab, UIPopUpMenu* menu );

	void copySelection( bool cut );

	void paste( FileTab* tab, std::optional<URI> into = std::nullopt );

	struct ClipboardFiles {
		std::vector<URI> uris;
		bool cut{ false };
	};

	/** Files in the clipboard: copied here or in another file manager (text/uri-list, with the
	 * GNOME / KDE cut markers). */
	ClipboardFiles clipboardFiles() const;

	void moveToTrash();

	void deleteSelection();

	void restoreFromTrash( const std::vector<URI>& uris );

	void emptyTrash();

	void renameSelection();

	void newFolder( FileTab* tab );

	void newTextFile( FileTab* tab );

	void showProperties( FileTab* tab );

	void openTerminalHere( FileTab* tab );

	void addToPlaces( const URI& uri );

	void dropFiles( const std::vector<URI>& uris, const URI& destination, int paneIndex );

	void transferFiles( const std::vector<URI>& sources, const URI& destination, bool move,
						int paneIndex );

	void startTransfer( const URI& source, const URI& destination, bool move, int paneIndex,
						const CopyOptions& options = {} );

	void updateTransfers();

	void reportError( const String& action, const VFSError& error );

	void runBlocking( std::function<std::string()> work, std::function<void()> refresh,
					  String errorTitle );

	/** Theme icon as a drawable for menus, tabs and buttons. */
	DrawablePtr icon( const std::string& name, int sizeDp = 16 ) const;

	// --- places
	void refreshPlaces();

	void refreshDeviceUsage();

	void showPlaceMenu( const PlacesPanel::Entry& entry );

	void showPlacesSectionMenu( PlaceSection section );

	/** Items shared by the place and section menus (Add Entry, Show Hidden Places, Hide
	 * Section). */
	void addPlacesPanelItems( UIPopUpMenu* menu, PlaceSection section );

	/** @return True if id was one of addPlacesPanelItems' items. */
	bool onPlacesPanelItem( const std::string& id, PlaceSection section );

	/** Add (index empty) or edit a place: label, location and icon. */
	void showPlaceDialog( std::optional<size_t> index, Place initial );

	void savePlaces();

	std::unique_ptr<UIApplication> mApp;
	UISceneNode* mUI{ nullptr };
	std::unique_ptr<MainThreadExecutor> mExecutor;
	std::shared_ptr<ThreadPool> mPool;
	VirtualFileSystem* mVFS{ nullptr };
	std::string mConfigDir;
	nlohmann::json mState;

	UIWidget* mToolbar{ nullptr };
	UIWidget* mMenuButton{ nullptr };
	UISplitter* mMainSplitter{ nullptr };
	UISplitter* mPaneSplitter{ nullptr };
	UIWidget* mPlacesScroll{ nullptr };
	UIWidget* mParking{ nullptr };
	PlacesPanel* mPlacesPanel{ nullptr };
	std::unique_ptr<Places> mPlaces;
	std::vector<Place> mDevices;

	std::unique_ptr<Pane> mPanes[2];
	int mActivePane{ 0 };
	bool mSplit{ true };
	bool mPlacesVisible{ true };
	bool mFilterBarVisible{ true };
	bool mShowHidden{ false };
	VFSDirectoryModel::NameFilterMode mFilterMode{ VFSDirectoryModel::NameFilterMode::Glob };
	bool mFilterCaseSensitive{ false };
	/** Column widths / visibility / sort shared by new tabs. */
	nlohmann::json mViewState;

	/** Fallback for backends without clipboard MIME data: the files copied here, while the
	 * clipboard text is still mClipboardText. */
	std::vector<URI> mClipboard;
	bool mClipboardCut{ false };
	std::string mClipboardText;
	std::map<Uint64, ActiveTransfer> mTransfers;
	Uint64 mLastTransferId{ 0 };
#ifdef EE_VFS_SFTP
	std::shared_ptr<SSHPrompts> mSSHPrompts;
#endif
};

} // namespace eefm

#endif
