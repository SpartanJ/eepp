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

	static UICodeEditorPlugin* New( const PluginManager* pluginManager );

	virtual ~LSPClientPlugin();

	virtual void update( UICodeEditor* );

	std::string getId() { return Definition().id; }

	std::string getTitle() { return Definition().name; }

	std::string getDescription() { return Definition().description; }

	bool isReady() const { return true; }

	void onRegister( UICodeEditor* );

	void onUnregister( UICodeEditor* );

	const std::unordered_map<UICodeEditor*, TextDocument*>& getEditorDocs() { return mEditorDocs; };

	const PluginManager* getManager() const;

	virtual bool onCreateContextMenu( UICodeEditor* editor, UIPopUpMenu* menu,
									  const Vector2i& position, const Uint32& flags );

	virtual bool onMouseMove( UICodeEditor* editor, const Vector2i& position, const Uint32& flags );

	virtual void onFocusLoss( UICodeEditor* editor );

	virtual bool onKeyDown( UICodeEditor*, const KeyEvent& );

	const Time& getHoverDelay() const;

	void setHoverDelay( const Time& hoverDelay );

  protected:
	const PluginManager* mManager{ nullptr };
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
	std::map<std::string, std::string> mKeyBindings; /* cmd, shortcut */
	std::map<TextDocument*, std::shared_ptr<TextDocument>> mDelayedDocs;
	Uint32 mHoverWaitCb;
	LSPHover mCurrentHover;
	Time mHoverDelay{ Seconds( 1.f ) };

	LSPClientPlugin( const PluginManager* pluginManager );

	void load( const PluginManager* pluginManager );

	void loadLSPConfig( std::vector<LSPDefinition>& lsps, const std::string& path );

	size_t lspFilePatternPosition( const std::vector<LSPDefinition>& lsps,
								   const std::vector<std::string>& patterns );

	void processNotification( PluginManager::Notification, const nlohmann::json& );
};

} // namespace ecode

#endif // ECODE_LSPPLUGIN_HPP
