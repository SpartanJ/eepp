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
} // namespace EE::UI

namespace ecode {

class Git;
class GitBranchModel;

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

  protected:
	std::unique_ptr<Git> mGit;
	std::string mGitBranch;
	Git::Status mGitStatus;
	UILinearLayout* mStatusBar{ nullptr };
	UIPushButton* mStatusButton{ nullptr };
	Time mRefreshFreq{ Seconds( 5 ) };

	GitPlugin( PluginManager* pluginManager, bool sync );

	void load( PluginManager* pluginManager );

	PluginRequestHandle processMessage( const PluginMessage& msg );

	void displayTooltip( UICodeEditor* editor, const Git::Blame& blame, const Vector2f& position );

	void hideTooltip( UICodeEditor* editor );

	void onRegisterListeners( UICodeEditor*, std::vector<Uint32>& listeners ) override;

	void onBeforeUnregister( UICodeEditor* ) override;

	void onUnregisterDocument( TextDocument* ) override;

	bool mGitFound{ false };
	bool mTooltipInfoShowing{ false };
	bool mStatusBarDisplayBranch{ true };
	bool mStatusBarDisplayModifications{ true };
	bool mStatusRecurseSubmodules{ true };
	bool mOldDontAutoHideOnMouseMove{ false };
	bool mOldUsingCustomStyling{ false };
	bool mInitialized{ false };
	Uint32 mOldTextStyle{ 0 };
	Uint32 mOldTextAlign{ 0 };
	Color mOldBackgroundColor;
	UITabWidget* mSidePanel{ nullptr };
	UITab* mTab{ nullptr };

	UITreeView* mBranchesTree{ nullptr };
	UITreeView* mStatusTree{ nullptr };
	UIDropDownList* mPanelSwicher{ nullptr };
	UIStackWidget* mStackWidget{ nullptr };
	std::vector<UIWidget*> mStackMap;
	UIWidget* mGitContentView{ nullptr };
	UIWidget* mGitNoContentView{ nullptr };
	std::atomic<bool> mRunningUpdateBranches{ false };
	std::atomic<bool> mRunningUpdateStatus{ false };
	Mutex mGitBranchMutex;
	Mutex mGitStatusMutex;

	struct CustomTokenizer {
		SyntaxDefinition def;
		SyntaxColorScheme scheme;
	};
	std::optional<CustomTokenizer> mStatusCustomTokenizer;
	std::optional<SyntaxDefinition> mTooltipCustomSyntaxDef;

	Color getVarColor( const std::string& var );

	void blame( UICodeEditor* editor );

	void updateStatus( bool force = false );

	void updateStatusBarSync();

	void updateUI();

	void updateUINow( bool force = false );

	void updateBranches();

	void buildSidePanelTab();

	void updateBranchesUI( std::shared_ptr<GitBranchModel> );
};

} // namespace ecode

#endif // ECODE_GITPLUGIN_HPP
