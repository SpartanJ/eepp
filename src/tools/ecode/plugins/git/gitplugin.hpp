#ifndef ECODE_GITPLUGIN_HPP
#define ECODE_GITPLUGIN_HPP

#include "../plugin.hpp"
#include "../pluginmanager.hpp"
#include "git.hpp"

namespace ecode {

class Git;

class GitPlugin : public PluginBase {
  public:
	static PluginDefinition Definition() {
		return { "git", "Git", "Git integration", GitPlugin::New, { 0, 0, 1 }, GitPlugin::NewSync };
	}

	static UICodeEditorPlugin* New( PluginManager* pluginManager );

	static UICodeEditorPlugin* NewSync( PluginManager* pluginManager );

	virtual ~GitPlugin();

	std::string getId() override { return Definition().id; }

	std::string getTitle() override { return Definition().name; }

	std::string getDescription() override { return Definition().description; }

	virtual void onFileSystemEvent( const FileEvent& ev, const FileInfo& file ) override;

	virtual void onRegister( UICodeEditor* ) override;

	virtual void onUnregister( UICodeEditor* ) override;

	virtual bool onCreateContextMenu( UICodeEditor* editor, UIPopUpMenu* menu,
									  const Vector2i& position, const Uint32& flags ) override;

  protected:
	std::unique_ptr<Git> mGit;

	GitPlugin( PluginManager* pluginManager, bool sync );

	void load( PluginManager* pluginManager );

	PluginRequestHandle processMessage( const PluginMessage& msg );

	void displayTooltip( UICodeEditor* editor, const Git::BlameData& blame,
						 const Vector2f& position );

	void hideTooltip( UICodeEditor* editor );

	void onRegisterListeners( UICodeEditor*, std::vector<Uint32>& listeners ) override;

	bool mGitFound{ false };
	bool mTooltipInfoShowing{ false };
	bool mStatusBarDisplayBranch{ true };
	bool mStatusBarDisplayModifications{ true };
	bool mOldDontAutoHideOnMouseMove{ false };
	bool mOldUsingCustomStyling{ false };
	Uint32 mOldTextStyle{ 0 };
	Uint32 mOldTextAlign{ 0 };

	void blame( UICodeEditor* editor );
};

} // namespace ecode

#endif // ECODE_GITPLUGIN_HPP
