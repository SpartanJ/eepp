#ifndef EEFM_APP_HPP
#define EEFM_APP_HPP

#include "config.hpp"
#include "dialogs.hpp"
#include "fileview.hpp"
#include "places.hpp"
#include "sshprompts.hpp"

#include <eepp/ee.hpp>
#include <eepp/scene/mainthreadexecutor.hpp>
#include <eepp/ui/uisystemicontheme.hpp>
#include <eepp/vfs.hpp>
#include <eepp/vfs/ui/vfsdirectorymodel.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <unordered_map>

namespace eefm {

using namespace EE;
using namespace EE::Scene;
using namespace EE::UI;
using namespace EE::UI::Models;
using namespace EE::Window;
using EE::System::Clock;
using EE::System::FileSystem;
using EE::System::Milliseconds;
using EE::System::Seconds;
using EE::System::Sys;
using EE::System::ThreadPool;
// Declared inside the namespace so they win over names other headers inject globally.
using EE::VFS::CopyOptions;
using EE::VFS::FileHandle;
using EE::VFS::OpenFlags;
using EE::VFS::OverwritePolicy;
using EE::VFS::RemoveOptions;
using EE::VFS::SpaceInfo;
using EE::VFS::Transfer;
using EE::VFS::TransferProgress;
using EE::VFS::TransferProgressSubscription;
using EE::VFS::TransferResult;
using EE::VFS::VFSDirectoryModel;
using EE::VFS::VFSError;
using EE::VFS::VFSErrorCode;
using EE::VFS::VFSFileInfo;
using EE::VFS::VFSResult;
using EE::VFS::VirtualFileSystem;
using EE::VFS::WatchStatus;
#ifdef EE_VFS_SFTP
using EE::VFS::SFTPProvider;
#endif

class FileOperations;
class PaneManager;
class PlacesController;
class SettingsActions;
class TerminalPanel;

/** @brief eefm: the window, its layout and the services the parts of the file manager share.
 *
 * The parts (panes, file operations, places, terminal and settings) are owned here and reach each
 * other through it.
 */
class App {
  public:
	App();

	~App();

	int run( int argc, char* argv[] );

	UISceneNode* ui() const { return mUI; }

	EE::Window::Window* window() const { return mApp->getWindow(); }

	MainThreadExecutor& executor() const { return *mExecutor; }

	/** Pool for blocking work (shared with the scene). */
	const std::shared_ptr<ThreadPool>& pool() const { return mPool; }

	AppConfig& config() const { return *mConfig; }

	VirtualFileSystem* vfs() const { return mVFS; }

	PaneManager& panes() const { return *mPaneManager; }

	FileOperations& fileOps() const { return *mFileOperations; }

	PlacesController& places() const { return *mPlacesController; }

	TerminalPanel& terminal() const { return *mTerminalPanel; }

	SettingsActions& settings() const { return *mSettingsActions; }

	/** Display label for a command's registered shortcut, including platform modifiers. */
	std::string getKeybind( const std::string& command ) const;

	/** Appends the registered shortcut to a tooltip when the command has one. */
	String shortcutTooltip( String text, const std::string& command ) const;

	/** Saves the state and closes the window. */
	void quit();

	bool isSplit() const { return mSplit; }

	void setSplit( bool split );

	bool showHidden() const { return mShowHidden; }

	void setShowHidden( bool show );

	void setFilterBarVisible( bool visible );

	/** The splitter that holds the panes (the right one only while split). */
	UISplitter* paneSplitter() const { return mPaneSplitter; }

	/** Hidden container for widgets that are not shown (the right pane, the terminal). */
	UIWidget* parking() const { return mParking; }

	/** Icon of a file row: the platform icon for its type, eefm's glyph until it loads. */
	UIIcon* fileIcon( const VFSFileInfo& info );

	/** Forgets the file icons resolved so far (the icon settings changed). */
	void clearFileIcons() { mFileIcons.clear(); }

	/** Icon of a place: the platform icon for it (special folders, drives, trash...). */
	UIIcon* placeIcon( const Place& place );

	/** Theme icon as a drawable for menus, tabs and buttons. */
	DrawablePtr icon( const std::string& name, int sizeDp = 16 ) const;

	/** Marks the scene when eefm's own (glyph) icons are used, for the stylesheet. */
	void updateIconStyle();

  private:
	bool init( const std::vector<std::string>& locations, bool forceSplit );

	void loadState();

	void saveState();

	void registerShortcuts();

	void showAppMenu();

	void setPlacesVisible( bool visible );

	void layoutLocationBars();

	std::unique_ptr<UIApplication> mApp;
	UISceneNode* mUI{ nullptr };
	std::unique_ptr<MainThreadExecutor> mExecutor;
	std::shared_ptr<ThreadPool> mPool;
	std::unique_ptr<AppConfig> mConfig;
	VirtualFileSystem* mVFS{ nullptr };
	std::unique_ptr<PaneManager> mPaneManager;
	std::unique_ptr<FileOperations> mFileOperations;
	std::unique_ptr<PlacesController> mPlacesController;
	std::unique_ptr<TerminalPanel> mTerminalPanel;
	std::unique_ptr<SettingsActions> mSettingsActions;
	/** Colored icons from the user's icon theme / the OS shell (glyph icons until loaded). */
	std::shared_ptr<UISystemIconTheme> mSystemIcons;
	/** fileIcon() by MIME type; cleared when the icon settings change. */
	std::unordered_map<std::string, UIIcon*> mFileIcons;
	std::string mConfigDir;
	nlohmann::json mState;

	UIWidget* mToolbar{ nullptr };
	UIWidget* mMenuButton{ nullptr };
	UISplitter* mMainSplitter{ nullptr };
	UISplitter* mPaneSplitter{ nullptr };
	UIWidget* mPlacesScroll{ nullptr };
	UIWidget* mParking{ nullptr };
#ifdef EE_VFS_SFTP
	std::shared_ptr<SSHPrompts> mSSHPrompts;
#endif
	bool mSplit{ true };
	bool mPlacesVisible{ true };
	bool mFilterBarVisible{ true };
	bool mShowHidden{ false };
};

} // namespace eefm

#endif
