#ifndef ECODE_GITPLUGIN_HPP
#define ECODE_GITPLUGIN_HPP

#include "../plugin.hpp"
#include "../pluginmanager.hpp"
#include "git.hpp"
#include <eepp/ui/models/model.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <optional>

using namespace EE::UI::Models;
using namespace EE::UI;

namespace EE::UI {
class UITreeView;
class UIDropDownList;
class UIStackWidget;
class UIListBoxItem;
class UIMenu;
} // namespace EE::UI

namespace ecode {

class Git;
class GitBranchModel;

static constexpr const char* GIT_EMPTY = "";
static constexpr const char* GIT_SUCCESS = "success";
static constexpr const char* GIT_ERROR = "error";
static constexpr const char* GIT_BOLD = "bold";
static constexpr const char* GIT_NOT_BOLD = "notbold";
static constexpr const char* GIT_TAG = "tag";
static constexpr const char* GIT_REPO = "repo";
static constexpr const char* GIT_STASH = "git-stash";

class GitPlugin : public PluginBase {
  public:
	static PluginDefinition Definition() {
		return { "git", "Git", "Git integration", GitPlugin::New, { 0, 0, 1 }, GitPlugin::NewSync };
	}

	static Plugin* New( PluginManager* pluginManager );

	static Plugin* NewSync( PluginManager* pluginManager );

	virtual ~GitPlugin();

	std::string getId() override { return Definition().id; }

	std::string getTitle() override { return Definition().name; }

	std::string getDescription() override { return Definition().description; }

	void onFileSystemEvent( const FileEvent& ev, const FileInfo& file ) override;

	void onRegister( UICodeEditor* ) override;

	void onUnregister( UICodeEditor* ) override;

	bool onCreateContextMenu( UICodeEditor* editor, UIPopUpMenu* menu, const Vector2i& position,
							  const Uint32& flags ) override;

	bool onKeyDown( UICodeEditor*, const KeyEvent& ) override;

	bool onMouseLeave( UICodeEditor*, const Vector2i&, const Uint32& ) override;

	std::string gitBranch();

	std::string statusTypeToString( Git::GitStatusType type );

	std::vector<std::string> repos();

	std::unordered_map<std::string, std::string> updateReposBranches();

	void updateRepos();

	bool isSilent() const { return mSilence; }

  protected:
	std::unique_ptr<Git> mGit;
	std::unordered_map<std::string, std::string> mGitBranches;
	Git::Status mGitStatus;
	std::vector<std::pair<std::string, std::string>> mRepos;
	std::string mProjectPath;
	std::string mRepoSelected;

	Time mRefreshFreq{ Seconds( 5 ) };
	bool mGitFound{ false };
	bool mTooltipInfoShowing{ false };
	bool mStatusBarDisplayBranch{ true };
	bool mStatusBarDisplayModifications{ true };
	bool mStatusRecurseSubmodules{ true };
	bool mOldDontAutoHideOnMouseMove{ false };
	bool mOldUsingCustomStyling{ false };
	bool mInitialized{ false };
	bool mSilence{ true };
	Uint32 mOldTextStyle{ 0 };
	Uint32 mOldTextAlign{ 0 };
	Color mOldBackgroundColor;
	UITabWidget* mSidePanel{ nullptr };
	UITab* mTab{ nullptr };

	UILinearLayout* mStatusBar{ nullptr };
	UIPushButton* mStatusButton{ nullptr };
	UITreeView* mBranchesTree{ nullptr };
	UITreeView* mStatusTree{ nullptr };
	UIDropDownList* mPanelSwicher{ nullptr };
	UIDropDownList* mRepoDropDown{ nullptr };
	UIStackWidget* mStackWidget{ nullptr };
	std::vector<UIWidget*> mStackMap;
	UIWidget* mGitContentView{ nullptr };
	UIWidget* mGitNoContentView{ nullptr };
	UILoader* mLoader{ nullptr };
	std::atomic<int> mRunningUpdateBranches{ 0 };
	std::atomic<int> mRunningUpdateStatus{ 0 };
	Clock mLastBranchesUpdate;
	Mutex mGitBranchMutex;
	Mutex mGitStatusMutex;
	Mutex mRepoMutex;
	Mutex mReposMutex;
	String mLastCommitMsg;

	struct CustomTokenizer {
		SyntaxDefinition def;
		SyntaxColorScheme scheme;
	};
	std::optional<CustomTokenizer> mStatusCustomTokenizer;
	std::optional<SyntaxDefinition> mTooltipCustomSyntaxDef;

	GitPlugin( PluginManager* pluginManager, bool sync );

	void load( PluginManager* pluginManager );

	PluginRequestHandle processMessage( const PluginMessage& msg );

	void displayTooltip( UICodeEditor* editor, const Git::Blame& blame, const Vector2f& position );

	void hideTooltip( UICodeEditor* editor );

	void onRegisterListeners( UICodeEditor*, std::vector<Uint32>& listeners ) override;

	void onBeforeUnregister( UICodeEditor* ) override;

	void onUnregisterDocument( TextDocument* ) override;

	Color getVarColor( const std::string& var );

	void blame( UICodeEditor* editor );

	void checkout( Git::Branch branch );

	void branchRename( Git::Branch branch );

	void branchDelete( Git::Branch branch );

	void branchMerge( Git::Branch branch );

	void fastForwardMerge( Git::Branch branch );

	void pull( const std::string& repoPath );

	void push( const std::string& repoPath );

	void fetch( const std::string& repoPath );

	void branchCreate();

	void commit( const std::string& repoPath );

	void stage( const std::vector<std::string>& files );

	void unstage( const std::vector<std::string>& files );

	void discard( const std::string& file );

	void diff( const std::string& file, bool isStaged );

	void openFile( const std::string& file );

	void updateStatus( bool force = false );

	void updateStatusBarSync();

	void updateUI();

	void updateUINow( bool force = false );

	void updateBranches( bool force = false );

	void buildSidePanelTab();

	void updateBranchesUI( std::shared_ptr<GitBranchModel> );

	void openBranchMenu( const Git::Branch& branch );

	void openFileStatusMenu( const Git::DiffFile& file );

	void stashPush( const std::vector<std::string>& files, const std::string& repoPath );

	void stashApply( const Git::Branch& branch );

	void stashDrop( const Git::Branch& branch );

	void runAsync( std::function<Git::Result()> fn, bool updateStatus, bool updateBranches,
				   bool displaySuccessMsg = false, bool updateBranchesOnError = false,
				   bool updateStatusOnError = false );

	void addMenuItem( UIMenu* menu, const std::string& txtKey, const std::string& txtVal,
					  const std::string& icon = "",
					  const KeyBindings::Shortcut& forcedKeybinding = KeyBindings::Shortcut() );

	std::string repoSelected();

	std::string projectPath();

	std::string repoName( const std::string& repoPath );

	std::string repoPath( const std::string& repoName );

	std::string repoFullName( const std::string& repoPath );

	std::string fixFilePath( const std::string& file );

	std::vector<std::string> fixFilePaths( const std::vector<std::string>& files );

	std::optional<Git::Branch> getBranchFromRepoPath( const std::string& repoPath );
};

} // namespace ecode

#endif // ECODE_GITPLUGIN_HPP
