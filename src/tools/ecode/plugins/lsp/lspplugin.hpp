#ifndef ECODE_LSPPLUGIN_HPP
#define ECODE_LSPPLUGIN_HPP

#include "../pluginmanager.hpp"
#include "lspclientmanager.hpp"
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

struct LSP {
	std::string name;
	std::string language;
	const SyntaxDefinition* langDefinition;
	std::vector<SyntaxPattern> filePatterns;
	std::string command;
	std::string url;
	std::vector<std::string> rootIndicationFileName;
};

class LSPPlugin : public UICodeEditorPlugin {
  public:
	static PluginDefinition Definition() {
		return {
			"lsp", "LSP Client", "Language Server Protocol Client.", LSPPlugin::New, { 0, 1, 0 } };
	}

	static UICodeEditorPlugin* New( const PluginManager* pluginManager );

	virtual ~LSPPlugin();

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
	LSPClientManager mClientManager;
	std::string mConfigPath;
	bool mClosing{ false };
	bool mReady{ false };

	LSPPlugin( const PluginManager* pluginManager );

	void load( const PluginManager* pluginManager );

	void loadLSPConfig( const std::string& path );
};

} // namespace ecode

#endif // ECODE_LSPPLUGIN_HPP
