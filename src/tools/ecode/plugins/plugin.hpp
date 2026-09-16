#ifndef ECODE_PLUGIN_HPP
#define ECODE_PLUGIN_HPP

#include "../filesystemlisteneroptions.hpp"
#include "lsp/lspprotocol.hpp"
#include <eepp/ui/models/filesystemmodel.hpp>
#include <eepp/ui/uicodeeditor.hpp>

using namespace EE;
using namespace EE::UI;
using namespace EE::UI::Models;

namespace EE::System {
class IniFile;
}

namespace EE::UI {
class UIListView;
}

namespace EE::UI::Abstract {
class ModelEvent;
}

namespace ecode {

class PluginManager;
class PluginContextProvider;
class SettingsPage;

/**
 * Base class for ecode plugins.
 *
 * Plugin teardown has two distinct phases:
 *
 * 1. PluginManager calls shutdown() synchronously on the main thread. shutdown() waits for
 *    loading to finish, prevents new work, unsubscribes manager-owned callbacks, and calls
 *    unregisterEditors() while the complete derived object is still alive.
 * 2. PluginManager destroys the plugin. Normal disable/reload may run the destructor on a worker
 *    thread; application shutdown may run it on the main thread.
 *
 * Implementers must detach everything that can call into the plugin from unregisterEditors():
 * editor and document commands, keybindings, event listeners, document clients, application
 * commands, UI widgets/actions, and queued main-thread callbacks. When unregisterEditors()
 * returns, no application-owned object may retain a pointer or callback to the plugin.
 *
 * Destructors must only release plugin-owned resources. They may cancel or join workers and stop
 * child processes or servers, but must not access editors, documents, UI objects, or PluginManager.
 * Potentially blocking resource shutdown belongs in the destructor so normal unload does not
 * stall the main thread.
 *
 * PluginManager owns the lifecycle: implementations must not call shutdown() themselves and must
 * not defer application detachment to their destructor. PluginBase is the preferred base class
 * when a plugin registers with editors or documents because it provides the common bookkeeping
 * and cleanup hooks described below.
 */
class Plugin : public UICodeEditorPlugin {
  public:
	explicit Plugin( PluginManager* manager );

	void subscribeFileSystemListener();

	void unsubscribeFileSystemListener();

	bool isReady() const;

	bool isLoading() const;

	bool isShuttingDown() const;

	/**
	 * Starts the main-thread application-detachment phase.
	 *
	 * This operation is idempotent and is called exclusively by PluginManager. It sets
	 * isShuttingDown(), waits for loading to complete, removes the plugin's filesystem and message
	 * subscriptions, and invokes unregisterEditors().
	 */
	void shutdown();

	virtual bool hasFileConfig();

	virtual std::string getFileConfigPath();

	virtual bool hasSettingsPage() const { return false; }

	virtual void registerSettings( SettingsPage& ) {}

	PluginManager* getManager() const;

	PluginContextProvider* getPluginContext() const;

	UISceneNode* getUISceneNode() const;

	virtual String::HashType getConfigFileHash() { return 0; }

	virtual void onFileSystemEvent( const FileEvent& ev, const FileInfo& file );

	virtual FileSystemListenerOptions getFileSystemListenerOptions() const;

	String i18n( const std::string& key, const String& def ) const;

	UIIcon* findIcon( const std::string& iconName );

	DrawablePtr iconDrawable( const std::string& iconName, Float dpSize );

	virtual void onVersionUpgrade( Uint32 /*oldVersion*/, Uint32 /*currentVersion*/ ) {}

	void showMessage( LSPMessageType type, const std::string& message,
					  const std::string& title = "" );

	virtual void onSaveState( IniFile* state ) {}

	virtual void onSaveProject( const std::string& /*projectFolder*/,
								const std::string& /*projectStatePath*/,
								bool /*rewriteStateOnlyIfNeeded*/ ) {}

	virtual void onLoadProject( const std::string& /*projectFolder*/,
								const std::string& /*projectStatePath*/ ) {}

	typedef std::function<void( const ModelEvent* )> ModelEventCallback;

	void createListView( UICodeEditor* editor, std::shared_ptr<Model> model,
						 const ModelEventCallback& onModelEventCb,
						 const std::function<void( UIListView* )> onCreateCb = {} );

  protected:
	PluginManager* mManager{ nullptr };
	std::shared_ptr<ThreadPool> mThreadPool;
	std::string mConfigPath;
	FileInfo mConfigFileInfo;

	std::atomic<bool> mReady{ false };
	std::atomic<bool> mLoading{ false };
	std::atomic<bool> mShuttingDown{ false };
	bool mUnregistering{ false };

	void setReady( Time loadTime = Seconds( 0 ) );

	void waitUntilLoaded();

	bool editorExists( UICodeEditor* editor );

	/**
	 * Detaches all application-owned references to this plugin.
	 *
	 * This is called by shutdown() on the main thread, with isShuttingDown() already true and while
	 * virtual dispatch still reaches the most-derived class. Implementations must be synchronous,
	 * must not wait for slow plugin-owned resources, and must leave no editor, document, UI,
	 * manager, or queued callback able to call the plugin.
	 *
	 * PluginBase subclasses overriding this method must call PluginBase::unregisterEditors()
	 * exactly once so its per-editor and per-document unregister hooks run.
	 */
	virtual void unregisterEditors() = 0;

	static UIListView* createListViewHelper( UICodeEditor* editor, std::shared_ptr<Model> model,
											 const ModelEventCallback& onModelEventCb );
};

/**
 * Plugin implementation helper for editor- and document-oriented plugins.
 *
 * PluginBase records every registered editor, document, and editor event listener. During
 * shutdown, unregisterEditors() asks each editor to unregister the plugin. That invokes the
 * following cleanup hooks while the derived plugin is still fully alive:
 *
 * - onBeforeUnregister(): removes keybindings declared in mKeyBindings;
 * - onUnregisterEditor(): removes plugin-specific state owned by one editor;
 * - onUnregisterDocument(): removes plugin commands and other per-document state after the last
 *   editor using that document is unregistered.
 *
 * The default onUnregisterDocument() removes commands named by mKeyBindings. A derived override
 * that adds other commands must remove them and call PluginBase::onUnregisterDocument(). Event
 * listener IDs added through onRegisterListeners() are removed automatically.
 *
 * Plugin implementer checklist:
 *
 * 1. Append every editor event listener ID to the vector passed to onRegisterListeners().
 * 2. Undo per-editor registrations in onUnregisterEditor().
 * 3. Undo per-document registrations in onUnregisterDocument(), then call the base implementation.
 * 4. Override unregisterEditors() only for state not covered by those hooks: invalidate async
 *    callbacks, detach document clients, remove application commands, and close plugin UI. Call
 *    PluginBase::unregisterEditors() exactly once.
 * 5. Keep only plugin-owned resource cleanup, such as joining workers or stopping servers, in the
 *    destructor.
 *
 * The unregister hooks are also called when individual editors and documents close, so they must
 * work both during normal editor use and during full plugin shutdown.
 *
 * A typical implementation is:
 *
 * @code
 * class ExamplePlugin : public PluginBase {
 *   public:
 *     ~ExamplePlugin() override {
 *         // No editor, document, UI, or PluginManager access here. This may block on a worker.
 *         mWorker.stopAndWait();
 *     }
 *
 *   protected:
 *     void onRegisterDocument( TextDocument* doc ) override {
 *         doc->setCommand( "example-command", [this] { runCommand(); } );
 *     }
 *
 *     void onRegisterListeners( UICodeEditor* editor,
 *                               std::vector<Uint32>& listeners ) override {
 *         listeners.emplace_back(
 *             editor->on( Event::OnDocumentSave, [this]( const Event* ) { onSave(); } ) );
 *     }
 *
 *     void onUnregisterDocument( TextDocument* doc ) override {
 *         doc->removeCommand( "example-command" );
 *         PluginBase::onUnregisterDocument( doc );
 *     }
 *
 *     void unregisterEditors() override {
 *         // Cancel/invalidate callbacks first, then detach application-owned state.
 *         mLifetime.invalidate();
 *         removeApplicationCommand();
 *         PluginBase::unregisterEditors();
 *         closePluginUI();
 *     }
 * };
 * @endcode
 *
 * If no plugin-specific application state exists, do not override unregisterEditors(); the base
 * implementation is sufficient. Never call onUnregisterEditor() or onUnregisterDocument()
 * directly from the destructor.
 */
class PluginBase : public Plugin {
  public:
	explicit PluginBase( PluginManager* manager ) : Plugin( manager ) {}

	/** Destructor finalizes plugin-owned state only; application detachment is already complete. */
	virtual ~PluginBase();

	virtual void onRegister( UICodeEditor* ) override;

	virtual void onUnregister( UICodeEditor* ) override;

	virtual String::HashType getConfigFileHash() override { return mConfigHash; }

	const std::map<std::string, std::string>& getKeybindings() { return mKeyBindings; }

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

	virtual void onDocumentLoaded( TextDocument* ) {}

	virtual void onDocumentClosed( TextDocument* ) {}

	virtual void onDocumentChanged( UICodeEditor*, TextDocument* /*oldDoc*/ ) {};

	virtual void onRegisterListeners( UICodeEditor*, std::vector<Uint32>& /*listeners*/ ) {};

	//! Called before an editor is unregistered. The default removes mKeyBindings from the editor.
	virtual void onBeforeUnregister( UICodeEditor* );

	//! Registers commands and other state shared by all editors using the document.
	virtual void onRegisterDocument( TextDocument* ) {}

	//! Registers state owned by one editor. The default installs mKeyBindings.
	virtual void onRegisterEditor( UICodeEditor* );

	//! Removes plugin-specific state owned by one editor.
	virtual void onUnregisterEditor( UICodeEditor* ) {}

	//! Removes per-document state. The default removes commands named by mKeyBindings.
	virtual void onUnregisterDocument( TextDocument* );

	//! Unregisters every tracked editor and dispatches the cleanup hooks documented above.
	virtual void unregisterEditors() override;
};

} // namespace ecode

#endif
