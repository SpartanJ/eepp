#ifndef ECODE_LSPPLUGIN_HPP
#define ECODE_LSPPLUGIN_HPP

#include "../pluginmanager.hpp"
#include "lspclientservermanager.hpp"
#include <eepp/config.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/threadpool.hpp>
#include <eepp/ui/doc/syntaxdefinitionmanager.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <set>
using namespace EE;
using namespace EE::System;
using namespace EE::UI;

namespace ecode {

// Implementation of the LSP Client:
// https://microsoft.github.io/language-server-protocol/specifications/lsp/3.17/specification/
class LSPClientPlugin : public UICodeEditorPlugin {
  public:
	static PluginDefinition Definition() {
		return { "lspclient",
				 "LSP Client",
				 "Language Server Protocol Client.",
				 LSPClientPlugin::New,
				 { 0, 0, 1 } };
	}

	static UICodeEditorPlugin* New( PluginManager* pluginManager );

	virtual ~LSPClientPlugin();

	virtual void update( UICodeEditor* );

	std::string getId() { return Definition().id; }

	std::string getTitle() { return Definition().name; }

	std::string getDescription() { return Definition().description; }

	bool isReady() const { return true; }

	void onRegister( UICodeEditor* );

	void onUnregister( UICodeEditor* );

	const std::unordered_map<UICodeEditor*, TextDocument*>& getEditorDocs() { return mEditorDocs; };

	PluginManager* getManager() const;

	virtual bool onCreateContextMenu( UICodeEditor* editor, UIPopUpMenu* menu,
									  const Vector2i& position, const Uint32& flags );

	virtual bool onMouseMove( UICodeEditor* editor, const Vector2i& position, const Uint32& flags );

	virtual void onFocusLoss( UICodeEditor* editor );

	virtual bool onKeyDown( UICodeEditor*, const KeyEvent& );

	const Time& getHoverDelay() const;

	void setHoverDelay( const Time& hoverDelay );

	const LSPClientServerManager& getClientManager() const;

	bool hasFileConfig();

	std::string getFileConfigPath();

  protected:
	PluginManager* mManager{ nullptr };
	std::shared_ptr<ThreadPool> mThreadPool;
	Clock mClock;
	Mutex mDocMutex;
	std::unordered_map<UICodeEditor*, std::vector<Uint32>> mEditors;
	std::unordered_map<UICodeEditor*, std::set<String::HashType>> mEditorsTags;
	std::set<TextDocument*> mDocs;
	std::unordered_map<UICodeEditor*, TextDocument*> mEditorDocs;
	LSPClientServerManager mClientManager;
	std::string mConfigPath;
	bool mClosing{ false };
	bool mReady{ false };
	bool mOldDontAutoHideOnMouseMove{ false };
	bool mOldUsingCustomStyling{ false };
	bool mSymbolInfoShowing{ false };
	std::map<std::string, std::string> mKeyBindings; /* cmd, shortcut */
	std::map<TextDocument*, std::shared_ptr<TextDocument>> mDelayedDocs;
	Uint32 mHoverWaitCb;
	LSPHover mCurrentHover;
	Time mHoverDelay{ Seconds( 1.f ) };
	Uint32 mOldTextStyle{ 0 };
	Uint32 mOldTextAlign{ 0 };

	LSPClientPlugin( PluginManager* pluginManager );

	void load( PluginManager* pluginManager );

	void loadLSPConfig( std::vector<LSPDefinition>& lsps, const std::string& path,
						bool updateConfigFile );

	size_t lspFilePatternPosition( const std::vector<LSPDefinition>& lsps,
								   const std::vector<std::string>& patterns );

	PluginRequestHandle processMessage( const PluginMessage& msg );

	PluginRequestHandle processCodeCompletionRequest( const PluginMessage& msg );

	PluginRequestHandle processSignatureHelpRequest( const PluginMessage& msg );

	PluginRequestHandle processCancelRequest( const PluginMessage& msg );

	void tryHideTooltip( UICodeEditor* editor, const Vector2i& position );

	void hideTooltip( UICodeEditor* editor );

	void tryDisplayTooltip( UICodeEditor* editor, const LSPHover& resp, const Vector2i& position );

	void displayTooltip( UICodeEditor* editor, const LSPHover& resp, const Vector2f& position );

	void getSymbolInfo( UICodeEditor* editor );

	void switchSourceHeader( UICodeEditor* editor );
};

} // namespace ecode

#endif // ECODE_LSPPLUGIN_HPP
