#ifndef ECODE_PLUGINMANAGER_HPP
#define ECODE_PLUGINMANAGER_HPP

#include "lsp/lspprotocol.hpp"
#include <eepp/ui/models/model.hpp>
#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::Models;
using namespace EE::UI::Tools;

namespace ecode {

class PluginManager;

typedef std::function<UICodeEditorPlugin*( PluginManager* pluginManager )> PluginCreatorFn;

#ifdef minor
#undef minor
#endif
#ifdef major
#undef major
#endif
struct PluginVersion {
	PluginVersion() {}

	PluginVersion( Uint8 major, Uint8 minor, Uint8 patch ) :
		major( major ),
		minor( minor ),
		patch( patch ),
		string( String::format( "%d.%d.%d", major, minor, patch ) ) {}

	Uint8 major; /**< major version */
	Uint8 minor; /**< minor version */
	Uint8 patch; /**< update version */
	std::string string;

	Uint32 getVersion() const { return major * 1000 + minor * 100 + patch; }

	const std::string& getVersionString() const { return string; }
};

struct PluginDefinition {
	std::string id;
	std::string name;
	std::string description;
	PluginCreatorFn creatorFn;
	PluginVersion version;
};

enum class PluginMessageType {
	WorkspaceFolderChanged, // Broadcast the workspace folder from the application to the plugins
	Diagnostics,			// Broadcast a document diagnostics from the LSP Client
	CodeCompletion, // Request the LSP Client to start a code completion in the requested document
					// and position
	LanguageServerCapabilities, // Request the language server capabilities of a language if there
								// is any available, it will be returned as a broadcast
	SignatureHelp,				// Request the LSP Client to provide function/method signature help
	CancelRequest,
	Undefined
};

enum class PluginMessageFormat {
	JSON,
	Diagnostics,
	CodeCompletion,
	LanguageServerCapabilities,
	SignatureHelp
};

using PluginIDType = Int64;

class LSPClientPlugin;

struct PluginMessage {
	PluginMessageType type;
	PluginMessageFormat format;
	const void* data;
	PluginIDType responseID{ 0 }; // 0 if it's not a response;

	const nlohmann::json& asJSON() const { return *static_cast<const nlohmann::json*>( data ); }

	bool isJSON() const { return format == PluginMessageFormat::JSON; }

	const LSPPublishDiagnosticsParams& asDiagnostics() const {
		return *static_cast<const LSPPublishDiagnosticsParams*>( data );
	}

	const LSPCompletionList& asCodeCompletion() const {
		return *static_cast<const LSPCompletionList*>( data );
	}

	const LSPServerCapabilities& asLanguageServerCapabilities() const {
		return *static_cast<const LSPServerCapabilities*>( data );
	}

	const LSPSignatureHelp& asSignatureHelp() const {
		return *static_cast<const LSPSignatureHelp*>( data );
	}

	const PluginIDType& asPluginID() const { return *static_cast<const PluginIDType*>( data ); }

	bool isResponse() const { return -1 != responseID && 0 != responseID; }

	bool isRequest() const { return -1 != responseID && 0 == responseID; }

	bool isBroadcast() const { return -1 == responseID; }
};

class PluginRequestHandle {
  public:
	static PluginRequestHandle broadcast() { return PluginRequestHandle( -1 ); }

	static PluginRequestHandle empty() { return PluginRequestHandle(); }

	PluginRequestHandle() {}
	PluginRequestHandle( PluginIDType id ) : mId( id ) {}
	virtual const PluginIDType& id() const { return mId; }
	virtual void cancel() {}

	bool isEmpty() const { return mId == 0; }

	bool isBroadcast() const { return mId == -1; }

  protected:
	PluginIDType mId{ 0 };
};

class PluginManager {
  public:
	static constexpr int versionNumber( int major, int minor, int patch ) {
		return ( (major)*1000 + (minor)*100 + ( patch ) );
	}

	static std::string versionString( int major, int minor, int patch ) {
		return String::format( "%d.%d.%.d", major, minor, patch );
	}

	PluginManager( const std::string& resourcesPath, const std::string& pluginsPath,
				   std::shared_ptr<ThreadPool> pool );

	~PluginManager();

	void registerPlugin( const PluginDefinition& def );

	UICodeEditorPlugin* get( const std::string& id );

	bool setEnabled( const std::string& id, bool enable );

	bool isEnabled( const std::string& id ) const;

	const std::string& getResourcesPath() const;

	const std::string& getPluginsPath() const;

	const std::map<std::string, bool>& getPluginsEnabled() const;

	void onNewEditor( UICodeEditor* editor );

	void setPluginsEnabled( const std::map<std::string, bool>& pluginsEnabled );

	const std::shared_ptr<ThreadPool>& getThreadPool() const;

	std::function<void( UICodeEditorPlugin* )> onPluginEnabled;

	const std::map<std::string, PluginDefinition>& getDefinitions() const;

	const PluginDefinition* getDefinitionIndex( const Int64& index ) const;

	UICodeEditorSplitter* getSplitter() const;

	const std::string& getWorkspaceFolder() const;

	void setWorkspaceFolder( const std::string& workspaceFolder );

	PluginRequestHandle sendRequest( UICodeEditorPlugin* pluginWho, PluginMessageType type,
									 PluginMessageFormat format, const void* data );

	void sendResponse( UICodeEditorPlugin* pluginWho, PluginMessageType type,
					   PluginMessageFormat format, const void* data,
					   const PluginIDType& responseID );

	void sendBroadcast( UICodeEditorPlugin* pluginWho, PluginMessageType, PluginMessageFormat,
						const void* data );

	void sendBroadcast( const PluginMessageType& notification, const PluginMessageFormat& format,
						void* data );

	void subscribeMessages( UICodeEditorPlugin* plugin,
							std::function<PluginRequestHandle( const PluginMessage& )> cb );

	void unsubscribeMessages( UICodeEditorPlugin* plugin );

  protected:
	using SubscribedPlugins =
		std::map<std::string, std::function<PluginRequestHandle( const PluginMessage& )>>;
	friend class App;
	std::string mResourcesPath;
	std::string mPluginsPath;
	std::string mWorkspaceFolder;
	std::map<std::string, UICodeEditorPlugin*> mPlugins;
	std::map<std::string, bool> mPluginsEnabled;
	std::map<std::string, PluginDefinition> mDefinitions;
	std::shared_ptr<ThreadPool> mThreadPool;
	UICodeEditorSplitter* mSplitter{ nullptr };
	Mutex mSubscribedPluginsMutex;
	SubscribedPlugins mSubscribedPlugins;
	bool mClosing{ false };

	bool hasDefinition( const std::string& id );

	void setSplitter( UICodeEditorSplitter* splitter );

	PluginRequestHandle sendRequest( const PluginMessageType& notification,
									 const PluginMessageFormat& format, void* data );
};

class PluginsModel : public Model {
  public:
	enum Columns { Id, Title, Enabled, Description, Version };

	static std::shared_ptr<PluginsModel> New( PluginManager* manager );

	PluginsModel( PluginManager* manager ) : mManager( manager ) {}

	virtual ~PluginsModel() {}

	virtual size_t rowCount( const ModelIndex& ) const;

	virtual size_t columnCount( const ModelIndex& ) const { return mColumnNames.size(); }

	virtual std::string columnName( const size_t& col ) const;

	virtual void setColumnName( const size_t& index, const std::string& name ) {
		eeASSERT( index <= Columns::Version );
		mColumnNames[index] = name;
	}

	virtual Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const;

	virtual void update() { onModelUpdate(); }

	PluginManager* getManager() const;

  protected:
	PluginManager* mManager;
	std::vector<std::string> mColumnNames{ "Id", "Title", "Enabled", "Description", "Version" };
};

class UIPluginManager {
  public:
	static UIWindow* New( UISceneNode* sceneNode, PluginManager* manager,
						  std::function<void( const std::string& )> loadFileCb );
};

} // namespace ecode

#endif // ECODE_PLUGINMANAGER_HPP
