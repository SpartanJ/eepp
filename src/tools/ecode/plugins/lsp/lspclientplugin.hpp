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

	std::string getId() { return Definition().id; }

	std::string getTitle() { return Definition().name; }

	std::string getDescription() { return Definition().description; }

	bool isReady() const { return true; }

	void onRegister( UICodeEditor* );

	void onUnregister( UICodeEditor* );

  protected:
	std::shared_ptr<ThreadPool> mPool;
	Clock mClock;
	Mutex mDocMutex;
	std::unordered_map<UICodeEditor*, std::vector<Uint32>> mEditors;
	std::set<TextDocument*> mDocs;
	std::unordered_map<UICodeEditor*, TextDocument*> mEditorDocs;
	LSPClientServerManager mClientManager;
	std::string mConfigPath;
	bool mClosing{ false };
	bool mReady{ false };

	LSPClientPlugin( const PluginManager* pluginManager );

	void load( const PluginManager* pluginManager );

	void loadLSPConfig( std::vector<LSPDefinition>& lsps, const std::string& path );

	size_t lspFilePatternPosition( const std::vector<LSPDefinition>& lsps,
								   const std::vector<std::string>& patterns );
};

} // namespace ecode

#endif // ECODE_LSPPLUGIN_HPP
