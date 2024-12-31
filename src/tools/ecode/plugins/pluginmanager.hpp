#ifndef ECODE_PLUGINMANAGER_HPP
#define ECODE_PLUGINMANAGER_HPP

#include "../projectsearch.hpp"
#include "lsp/lspprotocol.hpp"
#include "plugincontextprovider.hpp"
#include <eepp/ui/models/filesystemmodel.hpp>
#include <eepp/ui/models/model.hpp>
#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/ui/uicodeeditor.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwindow.hpp>

#include <array>
#include <limits>
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
class Plugin;
class FileSystemListener;
class ProjectBuildManager;

typedef std::function<Plugin*( PluginManager* pluginManager )> PluginCreatorFn;

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

	Uint8 major{ 0 }; /**< major version */
	Uint8 minor{ 0 }; /**< minor version */
	Uint8 patch{ 0 }; /**< update version */
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
	PluginCreatorFn creatorSyncFn{ nullptr };
};

enum class PluginCapability { WorkspaceSymbol, TextDocumentSymbol, FoldingRange, Max };

enum class PluginMessageType {
	WorkspaceFolderChanged, // Broadcast the workspace folder from the application to the plugins
	Diagnostics,			// Broadcast a document diagnostics from the LSP Client
	CodeCompletion, // Request the LSP Server to start a code completion in the requested document
					// and position
	LanguageServerCapabilities, // Request the language server capabilities of a language if there
								// is any available, it will be returned as a broadcast
	SignatureHelp,				// Request the LSP Server to provide function/method signature help
	CancelRequest,				// Cancel a request ID
	FindAndOpenClosestURI,		// Request a component to find and open the closest path from an URI
	DocumentFormatting,			// Request the LSP Server to format a document
	SymbolReference,			// Request the LSP Server to find a symbol reference in the project
	ShowMessage,		// The LSP server sends a request to the client to show a message on screen
	ShowDocument,		// The LSP server sends a request to the client to show a document
	WorkspaceSymbol,	// Request to the LSP server to query workspace symbols
	TextDocumentSymbol, // Request to the LSP server the document symbols
	TextDocumentFlattenSymbol, // Request to the LSP server the document symbols flattened
	DiagnosticsCodeAction,	   // Request a code action to anyone that can handle it
	FileSystemListenerReady,   // Broadcast to inform the plugins that the file system listener is
							   // available
	GetErrorOrWarning, // Request a component to provide the information of an error or warning in a
					   // particular document location
	GetDiagnostics,	   // Request the diagnostic information from a cursor position
	QueryPluginCapability, // Requests / queries if a plugin providers a capability
	UIReady,			   // Informs the Plugins that the UI is ready to be used
	UIThemeReloaded,	   // Informs the plugins that the UI theme has been reloaded
	FoldingRanges,		   // Request to the LSP server the folding ranges of a document
	WorkspaceDiagnostic,   // Informs the current workspace diagnostic
	LanguageServerReady,   // Informs that an LSP server is ready
	Undefined
};

enum class PluginMessageFormat {
	Empty,
	JSON,
	Diagnostics,
	CodeCompletion,
	LanguageServerCapabilities,
	SignatureHelp,
	ProjectSearchResult,
	ShowMessage,
	ShowDocument,
	SymbolInformation,
	DiagnosticsCodeAction,
	FoldingRanges,
	WorkspaceDiagnosticReport,
	LSPClientServer,
};

class PluginIDType {
  public:
	enum class Type { Integer, String, Invalid };

	PluginIDType() {}

	PluginIDType( Int64 val ) : mType( Type::Integer ), mInt( val ) {}

	PluginIDType( const std::string& val ) : mType( Type::String ), mString( val ) {}

	bool operator==( const PluginIDType& right ) {
		return mType == right.mType && ( ( mType == Type::Integer && mInt == right.mInt ) ||
										 ( mType == Type::String && mString == right.mString ) );
	}

	bool operator!=( const PluginIDType& right ) { return !( *this == right ); }

	bool is( const Type& type ) const { return type == mType; }

	bool isString() const { return Type::String == mType; }

	bool isInteger() const { return Type::Integer == mType; }

	bool isValid() const { return mType != Type::Invalid; }

	std::string toString() {
		if ( mType == Type::Integer )
			return String::toString( mInt );
		return mString;
	}

	operator Int64() const { return mInt; }

	operator std::string() { return mString; }

	const Int64& asInt() const { return mInt; }

	const std::string& asString() const { return mString; }

  protected:
	Type mType{ Type::Invalid };
	Int64 mInt{ std::numeric_limits<Int64>::max() };
	std::string mString;
};

class LSPClientPlugin;

struct PluginMessage {
	PluginMessageType type{ PluginMessageType::Undefined };
	PluginMessageFormat format{ PluginMessageFormat::Empty };
	const void* data{ nullptr };
	PluginIDType responseID{ 0 }; // 0 if it's not a response;

	const void* asData() const { return data; }

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

	const ProjectSearch::Result& asProjectSearchResult() const {
		return *static_cast<const ProjectSearch::Result*>( data );
	}

	const LSPShowMessageParams& asShowMessage() const {
		return *static_cast<const LSPShowMessageParams*>( data );
	}

	const LSPShowDocumentParams& asShowDocument() const {
		return *static_cast<const LSPShowDocumentParams*>( data );
	}

	const LSPSymbolInformationList& asSymbolInformation() const {
		return *static_cast<const LSPSymbolInformationList*>( data );
	}

	const LSPDiagnosticsCodeAction& asDiasnosticsCodeAction() const {
		return *static_cast<const LSPDiagnosticsCodeAction*>( data );
	}

	const LSPWorkspaceDiagnosticReport& asLSPWorkspaceDiagnosticReport() const {
		return *static_cast<const LSPWorkspaceDiagnosticReport*>( data );
	}

	const PluginIDType& asPluginID() const { return *static_cast<const PluginIDType*>( data ); }

	bool isResponse() const { return -1 != responseID && 0 != responseID; }

	bool isRequest() const { return 0 == responseID; }

	bool isBroadcast() const { return -1 == responseID; }
};

struct PluginInmediateResponse {
	PluginMessageType type{ PluginMessageType::Undefined };
	nlohmann::json data;
};

class PluginRequestHandle {
  public:
	static PluginRequestHandle broadcast() { return PluginRequestHandle( -1 ); }

	static PluginRequestHandle empty() { return PluginRequestHandle(); }

	PluginRequestHandle() {}

	PluginRequestHandle( PluginIDType id ) : mId( std::move( id ) ) {}

	explicit PluginRequestHandle( PluginInmediateResponse msg ) :
		mId( -2 ), mResponse( std::move( msg ) ) {}

	virtual const PluginIDType& id() const { return mId; }

	virtual void cancel() {}

	bool isEmpty() const { return mId == 0; }

	bool isBroadcast() const { return mId == -1; }

	const PluginInmediateResponse& getResponse() const { return mResponse; }

	bool isResponse() const { return mId == -2 && !mResponse.data.empty(); }

  protected:
	PluginIDType mId{ 0 };
	PluginInmediateResponse
		mResponse; //! Some requests can be responded inmediatly, so the message comes in the handle
};

class PluginManager {
  public:
	static constexpr int versionNumber( int major, int minor, int patch ) {
		return ( ( major ) * 1000 + ( minor ) * 100 + ( patch ) );
	}

	static std::string versionString( int major, int minor, int patch ) {
		return String::format( "%d.%d.%.d", major, minor, patch );
	}

	using OnFileLoadedCb = std::function<void( UICodeEditor*, const std::string& )>;
	using OnLoadFileCb = std::function<void( const std::string&, const OnFileLoadedCb& )>;

	PluginManager( const std::string& resourcesPath, const std::string& pluginsPath,
				   std::shared_ptr<ThreadPool> pool, const OnLoadFileCb& loadFileCb,
				   PluginContextProvider* context );

	~PluginManager();

	void registerPlugin( const PluginDefinition& def );

	void setUIReady();

	void setUIThemeReloaded();

	Plugin* get( const std::string& id );

	bool setEnabled( const std::string& id, bool enable, bool sync = false );

	bool isEnabled( const std::string& id ) const;

	bool reload( const std::string& id );

	const std::string& getResourcesPath() const;

	const std::string& getPluginsPath() const;

	const std::map<std::string, bool>& getPluginsEnabled() const;

	void onNewEditor( UICodeEditor* editor );

	void setPluginsEnabled( const std::map<std::string, bool>& pluginsEnabled, bool sync );

	const std::shared_ptr<ThreadPool>& getThreadPool() const;

	std::function<void( Plugin* )> onPluginEnabled;

	const std::map<std::string, PluginDefinition>& getDefinitions() const;

	const PluginDefinition* getDefinitionIndex( const Int64& index ) const;

	/** This is the code editor splitter. Where documents/terminals/etc are opened */
	UICodeEditorSplitter* getSplitter() const;

	/** This is the splitter between the code editor splitter and the bottom panel. */
	UISplitter* getMainSplitter() const;

	UISceneNode* getUISceneNode() const;

	const std::string& getWorkspaceFolder() const;

	void setWorkspaceFolder( const std::string& workspaceFolder );

	PluginRequestHandle sendRequest( PluginMessageType type, PluginMessageFormat format,
									 const void* data );

	PluginRequestHandle sendRequest( Plugin* pluginWho, PluginMessageType type,
									 PluginMessageFormat format, const void* data );

	void sendResponse( Plugin* pluginWho, PluginMessageType type, PluginMessageFormat format,
					   const void* data, const PluginIDType& responseID );

	void sendBroadcast( Plugin* pluginWho, PluginMessageType, PluginMessageFormat,
						const void* data );

	void sendBroadcast( const PluginMessageType& notification, const PluginMessageFormat& format,
						void* data );

	void subscribeMessages( Plugin* plugin,
							std::function<PluginRequestHandle( const PluginMessage& )> cb );

	void unsubscribeMessages( Plugin* plugin );

	void subscribeMessages( const std::string& uniqueComponentId,
							std::function<PluginRequestHandle( const PluginMessage& )> cb );

	void unsubscribeMessages( const std::string& uniqueComponentId );

	FileSystemListener* getFileSystemListener() const { return mFileSystemListener; };

	const OnLoadFileCb& getLoadFileFn() const;

	bool isPluginReloadEnabled() const;

	void setPluginReloadEnabled( bool pluginReloadEnabled );

	void subscribeFileSystemListener( Plugin* plugin );

	void unsubscribeFileSystemListener( Plugin* plugin );

	bool isClosing() const;

	PluginContextProvider* getPluginContext() const { return mPluginContext; }

  protected:
	using SubscribedPlugins =
		std::map<std::string, std::function<PluginRequestHandle( const PluginMessage& )>>;
	friend class App;
	std::string mResourcesPath;
	std::string mPluginsPath;
	std::string mWorkspaceFolder;
	std::map<std::string, Plugin*> mPlugins;
	std::map<std::string, bool> mPluginsEnabled;
	std::map<std::string, PluginDefinition> mDefinitions;
	std::shared_ptr<ThreadPool> mThreadPool;
	UICodeEditorSplitter* mSplitter{ nullptr };
	UISplitter* mMainSplitter{ nullptr };
	FileSystemListener* mFileSystemListener{ nullptr };
	PluginContextProvider* mPluginContext{ nullptr };
	Mutex mSubscribedPluginsMutex;
	Mutex mPluginsFSSubsMutex;
	SubscribedPlugins mSubscribedPlugins;
	OnLoadFileCb mLoadFileFn;
	Uint64 mFileSystemListenerCb{ 0 };
	UnorderedSet<Plugin*> mPluginsFSSubs;
	bool mClosing{ false };
	bool mPluginReloadEnabled{ false };

	bool hasDefinition( const std::string& id );

	void setSplitter( UICodeEditorSplitter* splitter );

	void setMainSplitter( UISplitter* splitter );

	void setFileSystemListener( FileSystemListener* listener );

	void subscribeFileSystemListener();

	void unsubscribeFileSystemListener();
};

class PluginsModel : public Model {
  public:
	enum Columns { Id, Title, Enabled, Description, Version, Count };

	static std::shared_ptr<PluginsModel> New( PluginManager* manager );

	PluginsModel( PluginManager* manager );

	virtual ~PluginsModel() {}

	virtual size_t rowCount( const ModelIndex& ) const;

	virtual size_t columnCount( const ModelIndex& ) const { return mColumnNames.size(); }

	virtual std::string columnName( const size_t& col ) const;

	virtual void setColumnName( const size_t& index, const std::string& name ) {
		eeASSERT( index <= Columns::Version );
		mColumnNames[index] = name;
	}

	virtual Variant data( const ModelIndex& index, ModelRole role = ModelRole::Display ) const;

	PluginManager* getManager() const;

  protected:
	PluginManager* mManager;
	std::array<std::string, Columns::Count> mColumnNames{ "Id", "Title", "Enabled", "Description",
														  "Version" };
};

class UIPluginManager {
  public:
	static UIWindow* New( UISceneNode* sceneNode, PluginManager* manager,
						  std::function<void( const std::string& )> loadFileCb );
};

} // namespace ecode

#endif // ECODE_PLUGINMANAGER_HPP
