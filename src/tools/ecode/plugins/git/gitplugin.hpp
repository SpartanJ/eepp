#ifndef ECODE_GITPLUGIN_HPP
#define ECODE_GITPLUGIN_HPP

#include "../plugin.hpp"
#include "../pluginmanager.hpp"
#include "git.hpp"
#include "githistorymodel.hpp"
#include <eepp/scene/eventconnection.hpp>
#include <eepp/scene/mainthreadlifetime.hpp>
#include <eepp/ui/models/model.hpp>
#include <eepp/ui/tools/uidiffview.hpp>
#include <eepp/ui/tools/uimergeview.hpp>
#include <eepp/ui/uilinearlayout.hpp>
#include <optional>

using namespace EE::UI::Models;
using namespace EE::UI;
using namespace EE::Scene;

namespace EE::UI {
class UITreeView;
class UIDropDownList;
class UIDropDownModelList;
class UIStackWidget;
class UIListBoxItem;
class UIMenu;
class UITextView;
} // namespace EE::UI

namespace ecode {

class Git;
class GitBranchModel;

class GitHistoryRefModel : public Model {
  public:
	explicit GitHistoryRefModel( std::shared_ptr<GitBranchModel> source, String headLabel );

	size_t rowCount( const ModelIndex& = {} ) const override { return mSourceIndexes.size() + 1; }

	size_t columnCount( const ModelIndex& = {} ) const override { return 1; }

	ModelIndex index( int row, int column, const ModelIndex& parent = {} ) const override;

	Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const override;

	std::string revision( size_t index ) const;

	bool isRevision( size_t index, std::string_view revision ) const;

  private:
	std::shared_ptr<GitBranchModel> mSource;
	std::vector<ModelIndex> mSourceIndexes;
	String mHeadLabel;
};

static constexpr const char* GIT_EMPTY = "";
static constexpr const char* GIT_SUCCESS = "success";
static constexpr const char* GIT_ERROR = "error";
static constexpr const char* GIT_BOLD = "bold";
static constexpr const char* GIT_NOT_BOLD = "notbold";
static constexpr const char* GIT_TAG = "tag";
static constexpr const char* GIT_REPO = "repo";
static constexpr const char* GIT_STASH = "git-stash";
static constexpr const char* GIT_STASH_TOOLTIP_CLASS = "git-stash-tooltip";

class GitPlugin : public PluginBase {
  public:
	static PluginDefinition Definition() {
		return { "git", "Git", "Git integration", GitPlugin::New, { 0, 1, 5 }, GitPlugin::NewSync };
	}

	static Plugin* New( PluginManager* pluginManager );

	static Plugin* NewSync( PluginManager* pluginManager );

	virtual ~GitPlugin();

	std::string getId() override { return Definition().id; }

	std::string getTitle() override { return Definition().name; }

	std::string getDescription() override { return Definition().description; }

	bool hasSettingsPage() const override { return true; }

	void registerSettings( SettingsPage& page ) override;

	void onFileSystemEvent( const FileEvent& ev, const FileInfo& file ) override;

	FileSystemListenerOptions getFileSystemListenerOptions() const override;

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

	bool isSilent() const { return mSilent; }

  protected:
	MainThreadLifetime<GitPlugin> mLifetime;

	std::shared_ptr<Git> mGit;
	std::unordered_map<std::string, std::string> mGitBranches;
	Git::Status mGitStatus;
	std::vector<std::pair<std::string, std::string>> mRepos;
	UnorderedSet<std::string> mGitStatusFilesCache;
	std::string mProjectPath;
	std::string mRepoSelected;
	std::string mHighlightStyleColor;

	Time mRefreshFreq{ Seconds( 5 ) };
	bool mGitFound{ false };
	bool mTooltipInfoShowing{ false };
	bool mStatusBarDisplayBranch{ true };
	bool mStatusBarDisplayModifications{ true };
	bool mStatusRecurseSubmodules{ true };
	bool mFileTreeHighlightChanges{ true };
	bool mOldDontAutoHideOnMouseMove{ false };
	bool mOldUsingCustomStyling{ false };
	bool mInitialized{ false };
	bool mSilent{ true };
	Uint32 mOldTextStyle{ 0 };
	Uint32 mOldTextAlign{ 0 };
	Color mOldBackgroundColor;
	UITabWidget* mSidePanel{ nullptr };
	UITab* mTab{ nullptr };
	UIWidget* mTabContents{ nullptr };

	UILinearLayout* mStatusBar{ nullptr };
	UIPushButton* mStatusButton{ nullptr };
	UITreeView* mBranchesTree{ nullptr };
	UITreeView* mStatusTree{ nullptr };
	UITreeView* mHistoryTree{ nullptr };
	UIDropDownModelList* mHistoryRefDropDown{ nullptr };
	std::shared_ptr<GitHistoryRefModel> mHistoryRefModel;
	std::shared_ptr<GitHistoryModel> mHistoryModel;
	UIDropDownList* mPanelSwicher{ nullptr };
	UIDropDownList* mRepoDropDown{ nullptr };
	UIStackWidget* mStackWidget{ nullptr };
	std::vector<UIWidget*> mStackMap;
	UIWidget* mGitContentView{ nullptr };
	UIWidget* mGitNoContentView{ nullptr };
	UIWidget* mConflictStateBar{ nullptr };
	UITextView* mConflictStateText{ nullptr };
	UILoader* mLoader{ nullptr };
	std::atomic<int> mRunningUpdateBranches{ 0 };
	std::atomic<int> mRunningHistoryRequests{ 0 };
	std::atomic<Uint64> mHistoryGeneration{ 0 };
	std::string mHistoryRepo;
	std::string mHistoryRevision{ "HEAD" };
	bool mUpdatingHistoryRefs{ false };
	bool mHistoryLoaded{ false };
	UIWidget* mCommitDetailsView{ nullptr };
	UITextView* mCommitDetailsSubject{ nullptr };
	UITextView* mCommitDetailsMetadata{ nullptr };
	UITextView* mCommitDetailsMessage{ nullptr };
	UITextView* mCommitDetailsParents{ nullptr };
	UITextView* mCommitDetailsStatus{ nullptr };
	UIPushButton* mCommitDetailsMessageToggle{ nullptr };
	UIPushButton* mCommitDetailsFilesToggle{ nullptr };
	UIPushButton* mCommitDetailsModeToggle{ nullptr };
	UIPushButton* mCommitDetailsGitHub{ nullptr };
	UIWidget* mCommitDetailsDiffContainer{ nullptr };
	UIScrollView* mCommitDetailsDiff{ nullptr };
	std::string mCommitDetailsMessageBody;
	std::string mCommitDetailsURL;
	bool mCommitDetailsMessageExpanded{ false };
	bool mCommitDetailsFilesCollapsed{ false };
	Tools::UIDiffView::ViewMode mCommitDetailsViewMode{ Tools::UIDiffView::ViewMode::Unified };
	Git::Commit mCommitDetailsCommit;
	std::string mCommitDetailsRepo;
	std::atomic<Uint64> mCommitDetailsGeneration{ 0 };
	EventConnection mCommitDetailsCloseConnection;
	std::atomic<int> mRunningUpdateStatus{ 0 };
	std::atomic<bool> mPendingForcedStatusUpdate{ false };
	std::shared_ptr<std::atomic<int>> mRunningAsyncTasks{ std::make_shared<std::atomic<int>>( 0 ) };
	Clock mLastBranchesUpdate;
	Mutex mGitBranchMutex;
	Mutex mGitStatusMutex;
	Mutex mGitStatusFileCacheMutex;
	Mutex mRepoMutex;
	Mutex mReposMutex;
	String mLastCommitMsg;
	struct GitConflictSession {
		std::string repoPath;
		std::vector<std::string> files;
		size_t currentFile{ 0 };
		Uint64 generation{ 0 };
		Git::GitOperation operation{ Git::GitOperation::None };
	};
	UnorderedMap<std::string, std::unique_ptr<GitConflictSession>> mConflictSessions;
	std::string mActiveConflictRepo;
	Tools::UIMergeView* mConflictView{ nullptr };
	EventConnection mConflictViewCloseConnection;
	Uint64 mConflictGeneration{ 0 };
	Uint32 mRepositionCbId{ 0 };

	struct CustomTokenizer {
		SyntaxDefinition def;
		SyntaxColorScheme scheme;
	};
	std::optional<CustomTokenizer> mStatusCustomTokenizer;
	std::optional<SyntaxDefinition> mTooltipCustomSyntaxDef;
	Uint32 mModelChangedId{ 0 };
	Uint32 mModelStylerId{ 0 };

	GitPlugin( PluginManager* pluginManager, bool sync );

	void load( PluginManager* pluginManager );

	PluginRequestHandle processMessage( const PluginMessage& msg );

	void displayTooltip( UICodeEditor* editor, const Git::Blame& blame, const Vector2f& position );

	void hideTooltip( UICodeEditor* editor );

	void onRegisterListeners( UICodeEditor*, std::vector<Uint32>& listeners ) override;

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

	void commit( const std::string& repoPath, bool mergeCommit = false );

	void stage( const std::vector<std::string>& files );

	void unstage( const std::vector<std::string>& files );

	enum class FileOperation { Stage, Unstage, Discard };

	void runFileOperation( std::vector<std::string> files, FileOperation operation );

	void discard( const std::vector<std::string>& files );

	void discard( const std::string& file );

	void diff( const Git::DiffMode mode, const std::string& repoPath );

	void diff( const std::string& file, Git::GitStatusType status );

	void diff( std::vector<Git::DiffFile> files );

	void openFile( const std::string& file );

	void openConflictResolver( const std::string& file );

	void loadConflictResolverView( std::shared_ptr<Doc::TextDocument> resultDocument,
								   Git::ConflictFile conflict, std::string repository,
								   std::vector<std::string> files, size_t currentFile,
								   Git::GitOperation operation, Uint64 generation );

	void recreateConflict();

	void saveAndStageConflict();

	void openAdjacentConflict( bool next );

	void continueConflictOperation();

	void abortConflictOperation();

	void acceptConflictSide( const std::string& file, bool stage2 );

	void runAsyncTask( std::function<void()> task );

	void updateStatus( bool force = false );

	void updateStatusBarSync();

	void updateUI();

	void updateUINow( bool force = false );

	void updateBranches( bool force = false );

	void ensureHistoryLoaded();

	void updateHistoryHeader();

	void updateHistoryRefs( const std::shared_ptr<GitBranchModel>& model );

	void reloadHistory();

	void invalidateHistory();

	void loadHistoryPage( GitHistoryModel::Node* node, Git::HistoryQuery query, bool append );

	void activateHistoryIndex( const ModelIndex& index, bool expand );

	void openCommitDetails( const Git::Commit& commit );

	void loadCommitFiles();

	void buildSidePanelTab();

	void updateBranchesUI( std::shared_ptr<GitBranchModel> );

	void openBranchMenu( const Git::Branch& branch );

	void openFileStatusMenu( std::vector<Git::DiffFile> files );

	void stashPush( const std::vector<std::string>& files, const std::string& repoPath );

	void stashApply( const Git::Branch& branch );

	void stashDrop( const Git::Branch& branch );

	void runAsync( std::function<Git::Result()> fn, bool updateStatus, bool updateBranches,
				   bool displaySuccessMsg = false, bool updateBranchesOnError = false,
				   bool updateStatusOnError = false, bool historyChanged = false );
	void runMergeLikeAsync( std::function<Git::Result( Git& )> fn, const std::string& repoPath );

	GitConflictSession* conflictSession( const std::string& repoPath );

	GitConflictSession* activeConflictSession();
	bool updateConflictSessions( UnorderedMap<std::string, Git::ConflictState>& conflictStates );

	void menuAdd( UIMenu* menu, const std::string& cmd, const std::string& text,
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

	void initModelStyler();

	void endModelStyler();

	void hideSidePanel();
};

} // namespace ecode

#endif // ECODE_GITPLUGIN_HPP
