#ifndef ECODE_PLUGIN_HPP
#define ECODE_PLUGIN_HPP

#include <eepp/ui/models/filesystemmodel.hpp>
#include <eepp/ui/uicodeeditor.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Models;

namespace ecode {

class PluginManager;

class Plugin : public UICodeEditorPlugin {
  public:
	explicit Plugin( PluginManager* manager );

	void subscribeFileSystemListener();

	void unsubscribeFileSystemListener();

	bool isReady() const;

	bool isLoading() const;

	bool isShuttingDown() const;

	virtual bool hasFileConfig();

	virtual std::string getFileConfigPath();

	PluginManager* getManager() const;

	virtual String::HashType getConfigFileHash() { return 0; }

	virtual void onFileSystemEvent( const FileEvent& ev, const FileInfo& file );

	String i18n( const std::string& key, const String& def ) const;

  protected:
	PluginManager* mManager{ nullptr };
	std::shared_ptr<ThreadPool> mThreadPool;
	std::string mConfigPath;
	FileInfo mConfigFileInfo;

	std::atomic<bool> mReady{ false };
	std::atomic<bool> mLoading{ false };
	std::atomic<bool> mShuttingDown{ false };

	void setReady();
};

class PluginBase : public Plugin {
  public:
	explicit PluginBase( PluginManager* manager ) : Plugin( manager ) {}

	virtual ~PluginBase();

	virtual void onRegister( UICodeEditor* ) override;

	virtual void onUnregister( UICodeEditor* ) override;

	virtual String::HashType getConfigFileHash() override { return mConfigHash; }

  protected:
	//! Keep track of the registered editors + all the listeners registered to each editor
	UnorderedMap<UICodeEditor*, std::vector<Uint32>> mEditors;
	//! Keep track of the documents opened
	UnorderedSet<TextDocument*> mDocs;
	//! Documents and Editors mutex
	Mutex mMutex;
	//! Keep track of the document pointer of each editor
	UnorderedMap<UICodeEditor*, TextDocument*> mEditorDocs;
	//! Keep track of the key bindings managed by the plugin
	std::map<std::string, std::string> mKeyBindings; /* cmd, shortcut */
	//! If the configuration is stored in a file, keep track of the config hash
	String::HashType mConfigHash{ 0 };

	virtual void onDocumentLoaded( TextDocument* ){};

	virtual void onDocumentClosed( TextDocument* ){};

	virtual void onDocumentChanged( UICodeEditor*, TextDocument* /*oldDoc*/ ){};

	virtual void onRegisterListeners( UICodeEditor*, std::vector<Uint32>& /*listeners*/ ){};

	//! Usually used to remove keybindings in an editor
	virtual void onBeforeUnregister( UICodeEditor* ){};

	virtual void onRegisterDocument( TextDocument* ){};

	virtual void onUnregisterEditor( UICodeEditor* ){};

	//! Usually used to unregister commands in a document
	virtual void onUnregisterDocument( TextDocument* ){};
};

} // namespace ecode

#endif
