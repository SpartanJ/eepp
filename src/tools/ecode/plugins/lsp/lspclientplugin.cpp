#include "lspclientplugin.hpp"
#include "../../version.hpp"
#include <eepp/graphics/primitives.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiscrollbar.hpp>
#include <eepp/ui/uistyle.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/ui/uitreeview.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>
#include <nlohmann/json.hpp>
#include <utf8cpp/utf8/unchecked.h>

using namespace EE::Window;
using json = nlohmann::json;

namespace ecode {

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
#define LSPCLIENT_THREADED 1
#else
#define LSPCLIENT_THREADED 0
#endif

static Action::UniqueID getMouseMoveHash( UICodeEditor* editor ) {
	return hashCombine( String::hash( "LSPClientPlugin::onMouseMove-" ),
						reinterpret_cast<Action::UniqueID>( editor ) );
}

static json getURIAndPositionJSON( UICodeEditor* editor ) {
	json data;
	auto doc = editor->getDocumentRef();
	auto sel = doc->getSelection();
	data["uri"] = doc->getURI().toString();
	data["position"] = { { "line", sel.start().line() }, { "character", sel.start().column() } };
	return data;
}

class LSPSymbolInfoTreeModel : public Model {
  public:
	static std::shared_ptr<LSPSymbolInfoTreeModel> create( UISceneNode* uiSceneNode,
														   LSPSymbolInformationList&& data,
														   const std::string& query = "" ) {
		return std::make_shared<LSPSymbolInfoTreeModel>( uiSceneNode, std::move( data ), query );
	}

	explicit LSPSymbolInfoTreeModel( UISceneNode* uiSceneNode, LSPSymbolInformationList&& info,
									 const std::string& query = "" ) :
		mUISceneNode( uiSceneNode ), mInfo( std::move( info ) ) {
		for ( auto& info : mInfo )
			info.updateParentsRefs();
		if ( !query.empty() )
			filter( String::toLower( query ) );
	}

	size_t rowCount( const ModelIndex& index ) const override {
		if ( !index.isValid() )
			return mInfo.size();
		if ( index.internalId() == -1 )
			return mInfo[index.row()].children.size();
		if ( index.internalData() )
			return static_cast<LSPSymbolInformation*>( index.internalData() )->children.size();
		return 0;
	};

	size_t columnCount( const ModelIndex& ) const override { return 1; }

	ModelIndex index( int row, int column, const ModelIndex& parent ) const override {
		if ( !parent.isValid() )
			return createIndex( row, column, &mInfo[row], -1 );
		if ( parent.internalData() ) {
			LSPSymbolInformation* parentNode =
				static_cast<LSPSymbolInformation*>( parent.internalData() );
			return createIndex( row, column, &parentNode->children[row], parent.row() );
		}
		return {};
	}

	ModelIndex parentIndex( const ModelIndex& index ) const override {
		if ( !index.isValid() || index.internalId() == -1 )
			return {};
		if ( index.internalData() ) {
			LSPSymbolInformation* node = static_cast<LSPSymbolInformation*>( index.internalData() );
			if ( !node->parent )
				return {};
			return createIndex( index.internalId(), 0, node->parent );
		}
		return {};
	}

	Variant data( const ModelIndex& index, ModelRole role ) const override {
		LSPSymbolInformation* node = static_cast<LSPSymbolInformation*>( index.internalData() );
		if ( node == nullptr )
			return {};

		if ( role == ModelRole::Display ) {
			return Variant( node->name.c_str() );
		} else if ( role == ModelRole::Icon ) {
			return mUISceneNode->findIcon( LSPSymbolKindHelper::toIconString( node->kind ) );
		}
		return {};
	}

  protected:
	UISceneNode* mUISceneNode{ nullptr };
	LSPSymbolInformationList mInfo;

	std::optional<LSPSymbolInformation> filterNode( const LSPSymbolInformation& node,
													const std::string& query ) {
		LSPSymbolInformation filteredNode( node.name, node.kind, node.range, node.detail );

		for ( const auto& child : node.children ) {
			if ( auto filteredChild = filterNode( child, query ); filteredChild ) {
				filteredChild->parent = &filteredNode;
				filteredNode.children.push_back( std::move( *filteredChild ) );
			}
		}

		if ( String::toLower( node.name ).find( query ) != std::string::npos ||
			 !filteredNode.children.empty() )
			return filteredNode;

		return std::nullopt;
	}

	void filter( const std::string& query ) {
		std::vector<LSPSymbolInformation> filteredRoots;

		for ( const auto& rootNode : mInfo )
			if ( auto filteredRoot = filterNode( rootNode, query ); filteredRoot )
				filteredRoots.push_back( std::move( *filteredRoot ) );

		mInfo = std::move( filteredRoots );
		for ( auto& i : mInfo )
			i.updateParentsRefs();
	}
};

class LSPLocationModel : public Model {
  public:
	enum CustomInfo { URI, TextRange };

	static std::shared_ptr<LSPLocationModel> create( const std::string& workspaceFolder,
													 const std::vector<LSPLocation>& data ) {
		return std::make_shared<LSPLocationModel>( workspaceFolder, data );
	}

	LSPLocationModel( const std::string& workspaceFolder, const std::vector<LSPLocation>& locs ) {
		createLocs( workspaceFolder, locs );
	}

	size_t rowCount( const ModelIndex& ) const override { return mLocs.size(); };

	size_t columnCount( const ModelIndex& ) const override { return 1; }

	Variant data( const ModelIndex& index, ModelRole role ) const override {
		if ( index.row() >= (Int64)mLocs.size() )
			return {};
		if ( role == ModelRole::Display ) {
			return Variant( mLocs[index.row()].display.c_str() );
		} else if ( role == ModelRole::Custom ) {
			if ( index.column() == CustomInfo::URI ) {
				return Variant( mLocs[index.row()].loc.uri.toString() );
			} else if ( index.column() == CustomInfo::TextRange ) {
				return Variant( mLocs[index.row()].loc.range.toString() );
			}
		}
		return {};
	}

  protected:
	struct Location {
		LSPLocation loc;
		std::string display;
	};
	std::vector<Location> mLocs;

	void createLocs( std::string workspaceFolder, const std::vector<LSPLocation>& locs ) {
		FileSystem::dirAddSlashAtEnd( workspaceFolder );

		for ( const auto& loc : locs ) {
			std::string display = loc.uri.getFSPath();
			FileSystem::filePathRemoveBasePath( workspaceFolder, display );
			display += " - L" + String::toString( loc.range.start().line() );
			mLocs.push_back( { loc, display } );
		}
	}
};

class LSPCodeActionModel : public Model {
  public:
	static std::shared_ptr<LSPCodeActionModel> create( UISceneNode* sceneNode,
													   const std::vector<LSPCodeAction>& data ) {
		return std::make_shared<LSPCodeActionModel>( sceneNode, data );
	}

	explicit LSPCodeActionModel( UISceneNode* sceneNode, const std::vector<LSPCodeAction>& cas ) :
		mUISceneNode( sceneNode ), mCodeActions( cas ) {}

	size_t rowCount( const ModelIndex& ) const override {
		return mCodeActions.empty() ? 1 : mCodeActions.size();
	};

	size_t columnCount( const ModelIndex& ) const override { return 1; }

	Variant data( const ModelIndex& index, ModelRole role ) const override {
		if ( role == ModelRole::Display ) {
			if ( index.row() < (Int64)mCodeActions.size() ) {
				return Variant( mCodeActions[index.row()].title.c_str() );
			} else {
				return Variant( mUISceneNode->i18n( "no_code_action", "No Code Action" ) );
			}
		}
		return {};
	}

	bool hasCodeActions() const { return !mCodeActions.empty(); }

	const LSPCodeAction& getCodeAction( size_t row ) const { return mCodeActions[row]; }

  protected:
	UISceneNode* mUISceneNode;
	std::vector<LSPCodeAction> mCodeActions;
};

Plugin* LSPClientPlugin::New( PluginManager* pluginManager ) {
	return eeNew( LSPClientPlugin, ( pluginManager, false ) );
}

Plugin* LSPClientPlugin::NewSync( PluginManager* pluginManager ) {
	return eeNew( LSPClientPlugin, ( pluginManager, true ) );
}

LSPClientPlugin::LSPClientPlugin( PluginManager* pluginManager, bool sync ) :
	Plugin( pluginManager ) {
	if ( sync ) {
		load( pluginManager );
	} else {
#if defined( LSPCLIENT_THREADED ) && LSPCLIENT_THREADED == 1
		mThreadPool->run( [this, pluginManager] { load( pluginManager ); } );
#else
		load( pluginManager );
#endif
	}
}

LSPClientPlugin::~LSPClientPlugin() {
	waitUntilLoaded();
	mShuttingDown = true;
	mManager->unsubscribeMessages( this );
	unsubscribeFileSystemListener();
	{
		Lock l( mDocMutex );
		for ( const auto& editor : mEditors ) {
			UICodeEditor* codeEditor = editor.first;
			for ( auto& kb : mKeyBindings ) {
				codeEditor->getKeyBindings().removeCommandKeybind( kb.first );
				if ( codeEditor->hasDocument() )
					codeEditor->getDocument().removeCommand( kb.first );
			}
			for ( auto listener : editor.second )
				codeEditor->removeEventListener( listener );
			if ( mBreadcrumb )
				codeEditor->unregisterTopSpace( this );
			codeEditor->unregisterPlugin( this );
			if ( mManager->getSplitter()->editorExists( codeEditor ) )
				codeEditor->removeActionsByTag( getMouseMoveHash( codeEditor ) );
		}
		if ( nullptr == mManager->getSplitter() )
			return;
	}
}

void LSPClientPlugin::update( UICodeEditor* ) {
	mClientManager.updateDirty();
}

struct LSPPositionAndServer {
	LSPPosition loc;
	LSPClientServer* server{ nullptr };
};

struct LSPURIAndServer {
	URI uri;
	LSPClientServer* server{ nullptr };
};

static LSPClientServer* getServerURIFromURI( LSPClientServerManager& manager, const json& data ) {
	URI uri( data["uri"] );
	return manager.getOneLSPClientServer( uri );
}

static LSPURIAndServer getServerURIFromTextDocumentURI( LSPClientServerManager& manager,
														const json& data ) {
	if ( !data.contains( "textDocument" ) || !data["textDocument"].contains( "uri" ) )
		return {};
	URI uri( data["textDocument"]["uri"] );
	return { uri, manager.getOneLSPClientServer( uri ) };
}

static void sanitizeCommand( std::string& cmd, const std::string& workspaceFolder ) {
	static std::string cpucount( String::toString( Sys::getCPUCount() ) );
	static std::string userdir = Sys::getUserDirectory();
	String::replaceAll( cmd, "$NPROC", cpucount );
	String::replaceAll( cmd, "${nproc}", cpucount );
	String::replaceAll( cmd, "$PROJECTPATH", workspaceFolder );
	String::replaceAll( cmd, "${project_root}", workspaceFolder );
	String::replaceAll( cmd, "$HOME", userdir );
	String::replaceAll( cmd, "${home}", userdir );
}

LSPPositionAndServer getLSPLocationFromJSON( LSPClientServerManager& manager, const json& data ) {
	if ( !data.contains( "uri" ) || !data.contains( "position" ) )
		return {};

	TextPosition position( LSPConverter::parsePosition( data["position"] ) );
	if ( !position.isValid() )
		return {};

	URI uri( data["uri"] );
	auto server = manager.getOneLSPClientServer( uri );
	if ( !server )
		return {};
	return { { uri, position }, server };
}

PluginRequestHandle LSPClientPlugin::processCodeCompletionRequest( const PluginMessage& msg ) {
	if ( !msg.isRequest() || !msg.isJSON() )
		return {};

	auto res = getLSPLocationFromJSON( mClientManager, msg.asJSON() );
	if ( !res.server )
		return {};

	auto ret = res.server->documentCompletion(
		res.loc.uri, res.loc.pos,
		[this]( const PluginIDType& id, const LSPCompletionList& completionList ) {
			mManager->sendResponse( this, PluginMessageType::CodeCompletion,
									PluginMessageFormat::CodeCompletion, &completionList, id );
		} );

	return ret;
}

PluginRequestHandle LSPClientPlugin::processSignatureHelpRequest( const PluginMessage& msg ) {
	if ( !msg.isRequest() || !msg.isJSON() )
		return {};

	auto res = getLSPLocationFromJSON( mClientManager, msg.asJSON() );
	if ( !res.server )
		return {};

	auto ret = res.server->signatureHelp(
		res.loc.uri, res.loc.pos, [this]( const PluginIDType& id, const LSPSignatureHelp& data ) {
			mManager->sendResponse( this, PluginMessageType::SignatureHelp,
									PluginMessageFormat::SignatureHelp, &data, id );
		} );

	return ret;
}

PluginRequestHandle LSPClientPlugin::processDocumentFormatting( const PluginMessage& msg ) {
	if ( !msg.isBroadcast() || !msg.isJSON() )
		return {};

	auto server = getServerURIFromTextDocumentURI( mClientManager, msg.asJSON() );
	if ( !server.server )
		return {};

	if ( !msg.asJSON().contains( "options" ) )
		return {};

	auto ret = server.server->documentFormatting(
		server.uri, msg.asJSON()["options"],
		[this, server]( const PluginIDType&, const std::vector<LSPTextEdit>& edits ) {
			mManager->getSplitter()->getUISceneNode()->runOnMainThread(
				[this, server, edits] { processDocumentFormattingResponse( server.uri, edits ); } );
		} );

	return ret;
}

PluginRequestHandle LSPClientPlugin::processWorkspaceSymbol( const PluginMessage& msg ) {
	if ( !msg.isRequest() || !msg.isJSON() )
		return {};

	auto servers = mClientManager.getFilteredServers( []( LSPClientServer* server ) {
		return server->getCapabilities().workspaceSymbolProvider;
	} );
	if ( servers.empty() )
		return {};

	json jmsg = msg.asJSON();
	std::string query = jmsg.value( "query", "" );
	PluginRequestHandle hdl;

	for ( const auto server : servers ) {
		if ( Engine::instance()->isMainThread() ) {
			server->workspaceSymbolAsync(
				query, [this, query]( const PluginIDType& id, LSPSymbolInformationList&& info ) {
					if ( !query.empty() ) {
						for ( auto& i : info ) {
							if ( i.score == 0.f )
								i.score = String::fuzzyMatch( i.name, query );
						}
					}
					mManager->sendResponse( this, PluginMessageType::WorkspaceSymbol,
											PluginMessageFormat::SymbolInformation, &info, id );
				} );
		} else {
			hdl = server->workspaceSymbol(
				query, [this, query]( const PluginIDType& id, LSPSymbolInformationList&& info ) {
					if ( !query.empty() ) {
						for ( auto& i : info ) {
							if ( i.score == 0.f )
								i.score = String::fuzzyMatch( i.name, query );
						}
					}
					mManager->sendResponse( this, PluginMessageType::WorkspaceSymbol,
											PluginMessageFormat::SymbolInformation, &info, id );
				} );
		}
	}

	return hdl;
}

PluginRequestHandle LSPClientPlugin::processWorkspaceDiagnostic( const PluginMessage& msg ) {
	if ( !( ( msg.isBroadcast() && msg.type == PluginMessageType::LanguageServerReady && msg.data &&
			  msg.format == PluginMessageFormat::LSPClientServer ) ||
			( msg.isRequest() && msg.type == PluginMessageType::WorkspaceDiagnostic &&
			  msg.format == PluginMessageFormat::LSPClientServer ) ) )
		return {};
	/* NOTE: I couldn't find a single LSP server supporting this feature so I cannot test it.
	 * I'll leave the current implementation here to continue with it later in the future.
	 * Since I started implementing it assuming it was commonly supported...

		LSPClientServer* server =
			const_cast<LSPClientServer*>( reinterpret_cast<const LSPClientServer*>( msg.data ) );

		if ( !server->getCapabilities().diagnosticProvider.workspaceDiagnostics )
			return {};

		server->workspaceDiagnosticAsync(
			[this]( const PluginIDType&, LSPWorkspaceDiagnosticReport&& report ) {
				mManager->sendBroadcast( this, PluginMessageType::WorkspaceDiagnostic,
										 PluginMessageFormat::WorkspaceDiagnosticReport, &report );
			} );
	*/
	return PluginRequestHandle::broadcast();
}

PluginRequestHandle LSPClientPlugin::processTextDocumentSymbol( const PluginMessage& msg ) {
	if ( !msg.isRequest() ||
		 ( msg.type != PluginMessageType::TextDocumentSymbol &&
		   msg.type != PluginMessageType::TextDocumentFlattenSymbol ) ||
		 msg.format != PluginMessageFormat::JSON || !msg.asJSON().contains( "uri" ) )
		return {};

	URI uri( msg.asJSON().value( "uri", "" ) );
	if ( uri.empty() )
		return {};

	const LSPSymbolInformationList& symbols = msg.type == PluginMessageType::TextDocumentSymbol
												  ? getDocumentSymbols( uri )
												  : getDocumentFlattenSymbols( uri );
	{
		Lock l( mDocSymbolsMutex );
		if ( !symbols.empty() ) {
			mManager->sendResponse( this, msg.type, PluginMessageFormat::SymbolInformation,
									&symbols, uri.toString() );
			return { uri.toString() };
		}
	}

	LSPClientServer* server = mClientManager.getOneLSPClientServer( uri );
	if ( !server || !server->getCapabilities().documentSymbolProvider )
		return {};
	auto handler = [uri, this]( const PluginIDType& id, LSPSymbolInformationList&& res ) {
		setDocumentSymbolsFromResponse( id, uri, std::move( res ) );
	};
	if ( Engine::instance()->isMainThread() ) {
		server->getThreadPool()->run(
			[server, uri, handler]() { server->documentSymbols( uri, handler ); } );
	} else {
		server->documentSymbols( uri, handler );
	}

	return { uri.toString() };
}

PluginRequestHandle LSPClientPlugin::processFoldingRanges( const PluginMessage& msg ) {
	if ( !msg.isRequest() || msg.type != PluginMessageType::FoldingRanges ||
		 msg.format != PluginMessageFormat::JSON || !msg.asJSON().contains( "uri" ) )
		return {};

	URI uri( msg.asJSON().value( "uri", "" ) );
	if ( uri.empty() )
		return {};

	LSPClientServer* server = mClientManager.getOneLSPClientServer( uri );
	if ( !server || !server->getCapabilities().foldingRangeProvider )
		return {};

	auto handler = [uri, this]( const PluginIDType& id, const std::vector<LSPFoldingRange>& res ) {
		mManager->sendResponse( this, PluginMessageType::FoldingRanges,
								PluginMessageFormat::FoldingRanges, &res, id );
	};

	if ( Engine::instance()->isMainThread() ) {
		server->getThreadPool()->run(
			[server, uri, handler]() { server->documentFoldingRange( uri, handler ); } );
	} else {
		server->documentFoldingRange( uri, handler );
	}

	return { uri.toString() };
}

void processFormattingResponse( const std::shared_ptr<TextDocument>& doc,
								std::vector<LSPTextEdit> edits ) {
	TextRanges ranges = doc->getSelections();

	doc->resetCursor();
	doc->setRunningTransaction( true );

	// Sort from bottom to top, this way we don't have to compute any position deltas
	std::sort( edits.begin(), edits.end(), []( const LSPTextEdit& left, const LSPTextEdit& right ) {
		return left.range > right.range;
	} );

	for ( const auto& edit : edits ) {
		doc->setSelection( edit.range );
		if ( edit.text.empty() ) {
			doc->deleteSelection( 0 );
		} else {
			if ( edit.range.hasSelection() )
				doc->deleteTo( 0, 0 );
			doc->insert( 0, doc->getSelectionIndex( 0 ).start(), edit.text );
		}
	}

	doc->setSelection( ranges );

	doc->setRunningTransaction( false );
}

bool LSPClientPlugin::processDocumentFormattingResponse( const URI& uri,
														 std::vector<LSPTextEdit> edits ) {
	for ( const auto& edit : edits )
		if ( !edit.range.isValid() )
			return false;

	auto doc = mManager->getSplitter()->findDocFromURI( uri );
	if ( !doc ) {
		mManager->getLoadFileFn()( uri.getFSPath(), [this, edits]( UICodeEditor* editor, auto ) {
			auto documentRef = editor->getDocumentRef();
			mManager->getSplitter()->getUISceneNode()->runOnMainThread(
				[documentRef, edits]() { processFormattingResponse( documentRef, edits ); } );
		} );
		return false;
	}

	processFormattingResponse( doc, edits );

	return true;
}

void LSPClientPlugin::setDocumentSymbols( const URI& docURI, LSPSymbolInformationList&& res ) {
	Lock l( mDocSymbolsMutex );
	mDocFlatSymbols[docURI] = !LSPSymbolInformationListHelper::isFlat( res )
								  ? LSPSymbolInformationListHelper::flatten( res )
								  : res;

	for ( auto& r : res )
		r.updateParentsRefs();

	mDocSymbols[docURI] = std::move( res );

	getManager()->getSplitter()->forEachDoc( [docURI, this]( TextDocument& doc ) {
		if ( doc.getURI() == docURI )
			updateCurrentSymbol( doc );
	} );
}

void LSPClientPlugin::setDocumentSymbolsFromResponse( const PluginIDType& id, const URI& docURI,
													  LSPSymbolInformationList&& res ) {
	setDocumentSymbols( docURI, std::move( res ) );

	{
		const auto& docSymbols = getDocumentSymbols( docURI );
		Lock l( mDocSymbolsMutex );
		mManager->sendResponse( this, PluginMessageType::TextDocumentSymbol,
								PluginMessageFormat::SymbolInformation, &docSymbols, id );
	}

	{
		const auto& docFlattenSymbols = getDocumentFlattenSymbols( docURI );
		Lock l( mDocSymbolsMutex );
		mManager->sendResponse( this, PluginMessageType::TextDocumentFlattenSymbol,
								PluginMessageFormat::SymbolInformation, &docFlattenSymbols, id );
	}
}

const LSPSymbolInformationList& LSPClientPlugin::getDocumentSymbols( const URI& docURI ) {
	Lock l( mDocSymbolsMutex );
	auto foundIt = mDocSymbols.find( docURI );
	if ( foundIt != mDocSymbols.end() )
		return foundIt->second;
	mDocSymbols[docURI] = {};
	return mDocSymbols[docURI];
}

const LSPSymbolInformationList& LSPClientPlugin::getDocumentFlattenSymbols( const URI& docURI ) {
	Lock l( mDocSymbolsMutex );
	auto foundIt = mDocFlatSymbols.find( docURI );
	if ( foundIt != mDocFlatSymbols.end() )
		return foundIt->second;
	mDocFlatSymbols[docURI] = {};
	return mDocFlatSymbols[docURI];
}

TextDocument* LSPClientPlugin::getDocumentFromURI( const URI& uri ) {
	Lock l( mDocMutex );
	for ( const auto client : mDocs )
		if ( client->getURI() == uri )
			return client;
	return nullptr;
}

bool LSPClientPlugin::onMouseClick( UICodeEditor* editor, const Vector2i& pos,
									const Uint32& flags ) {
	const bool breadcrumbClick = mBreadcrumb && ( flags & EE_BUTTON_LMASK );
	const Vector2f localPos( breadcrumbClick ? editor->convertToNodeSpace( pos.asFloat() )
											 : Vector2f::Zero );
	if ( breadcrumbClick && localPos.y < mPluginTopSpace &&
		 localPos.x < editor->getTopAreaWidth() ) {
		const Vector2f downLocalPos( editor->convertToNodeSpace(
			editor->getEventDispatcher()->getMouseDownPos().asFloat() ) );
		if ( downLocalPos.y < mPluginTopSpace && downLocalPos.x < editor->getTopAreaWidth() ) {
			editor->getDocument().execute( "lsp-show-document-symbols", editor );
			return true;
		}
	}

	Input* input = editor->getUISceneNode()->getWindow()->getInput();
	Uint32 mod = input->getSanitizedModState();
	if ( mod != ( KEYMOD_LALT | KeyMod::getDefaultModifier() ) || ( flags & EE_BUTTON_LMASK ) == 0 )
		return false;

	auto docPos = editor->resolveScreenPosition( pos.asFloat() );
	if ( !docPos.isValid() || !editor->getDocument().isValidPosition( docPos ) )
		return false;

	editor->getDocument().execute( "lsp-go-to-definition", editor );

	return true;
}

bool LSPClientPlugin::semanticHighlightingEnabled() const {
	return mSemanticHighlighting;
}

void LSPClientPlugin::setSemanticHighlighting( bool semanticHighlighting ) {
	mSemanticHighlighting = semanticHighlighting;
}

bool LSPClientPlugin::langSupportsSemanticHighlighting( const std::string& lspLang ) {
	return !std::any_of( mSemanticHighlightingDisabledLangs.begin(),
						 mSemanticHighlightingDisabledLangs.end(),
						 [&lspLang]( const auto& other ) { return lspLang == other; } );
}

bool LSPClientPlugin::isSilent() const {
	return mSilence;
}

void LSPClientPlugin::setSilent( bool silence ) {
	mSilence = silence;
}

bool LSPClientPlugin::trimLogs() const {
	return mTrimLogs;
}

void LSPClientPlugin::setTrimLogs( bool trimLogs ) {
	mTrimLogs = trimLogs;
}

bool LSPClientPlugin::editorExists( UICodeEditor* editor ) {
	return mManager->getSplitter()->editorExists( editor );
}

void LSPClientPlugin::createListView( UICodeEditor* editor, const std::shared_ptr<Model>& model,
									  const ModelEventCallback& onModelEventCb,
									  const std::function<void( UIListView* )> onCreateCb ) {
	UICodeEditorSplitter* splitter = getManager()->getSplitter();
	if ( nullptr == splitter || !editorExists( editor ) )
		return;
	editor->runOnMainThread( [model, editor, splitter, onModelEventCb, onCreateCb] {
		auto lvs = editor->findAllByClass( "editor_listview" );
		for ( auto* ilv : lvs )
			ilv->close();

		UIListView* lv = UIListView::New();
		lv->setParent( editor );
		lv->addClass( "editor_listview" );
		auto pos =
			editor->getRelativeScreenPosition( editor->getDocumentRef()->getSelection().start() );
		lv->setPixelsPosition( { pos.x, pos.y + editor->getLineHeight() } );
		if ( !lv->getParent()->getLocalBounds().contains(
				 lv->getLocalBounds().setPosition( lv->getPixelsPosition() ) ) ) {
			lv->setPixelsPosition( { pos.x, pos.y - lv->getPixelsSize().getHeight() } );
		}
		lv->setVisible( true );
		lv->getVerticalScrollBar()->reloadStyle( true, true, true );
		lv->setAutoExpandOnSingleColumn( false );
		lv->setModel( model );
		Float height = std::min( lv->getContentSize().y, lv->getRowHeight() * 8 );
		Float colWidth = lv->getMaxColumnContentWidth( 0 ) + PixelDensity::dpToPx( 4 );
		bool needsVScroll = lv->getContentSize().y > lv->getRowHeight() * 8;
		Float width = colWidth + lv->getPixelsPadding().getWidth() +
					  ( needsVScroll ? lv->getVerticalScrollBar()->getPixelsSize().getWidth() : 0 );
		lv->setPixelsSize( { width, height } );
		lv->setColumnWidth( 0, colWidth );
		lv->setScrollMode( needsVScroll ? ScrollBarMode::Auto : ScrollBarMode::AlwaysOff,
						   ScrollBarMode::AlwaysOff );
		if ( onCreateCb )
			onCreateCb( lv );
		lv->setSelection( model->index( 0 ) );
		lv->setFocus();
		Uint32 focusCb = lv->getUISceneNode()->getUIEventDispatcher()->addFocusEventCallback(
			[lv]( const auto&, Node* focus, Node* ) {
				if ( !lv->inParentTreeOf( focus ) && !lv->isClosing() )
					lv->close();
			} );
		Uint32 cursorCb =
			editor->on( Event::OnCursorPosChange, [lv, editor, splitter]( const Event* ) {
				if ( !lv->isClosing() ) {
					lv->close();
					if ( splitter->editorExists( editor ) )
						editor->setFocus();
				}
			} );
		lv->on( Event::KeyDown, [lv, splitter, editor]( const Event* event ) {
			if ( event->asKeyEvent()->getKeyCode() == EE::Window::KEY_ESCAPE && !lv->isClosing() )
				lv->close();
			if ( splitter->editorExists( editor ) )
				editor->setFocus();
		} );
		lv->on( Event::OnModelEvent, [onModelEventCb]( const Event* event ) {
			const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
			if ( onModelEventCb )
				onModelEventCb( modelEvent );
		} );
		lv->on( Event::OnClose, [lv, editor, cursorCb, focusCb]( const Event* ) {
			lv->getUISceneNode()->getUIEventDispatcher()->removeFocusEventCallback( focusCb );
			editor->removeEventListener( cursorCb );
		} );
	} );
}

void LSPClientPlugin::createLocationsView( UICodeEditor* editor,
										   const std::vector<LSPLocation>& res ) {
	auto model = LSPLocationModel::create( mManager->getWorkspaceFolder(), res );
	createListView( editor, model, [this]( const ModelEvent* modelEvent ) {
		if ( modelEvent->getModelEventType() != ModelEventType::Open )
			return;
		auto r = modelEvent->getModelIndex().row();
		Variant uri( modelEvent->getModel()->data(
			modelEvent->getModel()->index( r, LSPLocationModel::CustomInfo::URI ),
			ModelRole::Custom ) );
		Variant range( modelEvent->getModel()->data(
			modelEvent->getModel()->index( r, LSPLocationModel::CustomInfo::TextRange ),
			ModelRole::Custom ) );
		LSPLocation loc;
		loc.uri = URI( uri.asStdString() );
		loc.range = TextRange::fromString( range.asStdString() );
		mClientManager.goToLocation( loc );
		modelEvent->getNode()->close();
	} );
}

void LSPClientPlugin::createCodeActionsView( UICodeEditor* editor,
											 const std::vector<LSPCodeAction>& cas ) {
	bool casEmpty = cas.empty();
	std::shared_ptr<LSPCodeActionModel> model;
	if ( mQuickFix.title.empty() && mQuickFix.kind.empty() &&
		 mQuickFix.edit.documentChanges.empty() && mQuickFix.edit.changes.empty() ) {
		model = LSPCodeActionModel::create( mManager->getSplitter()->getUISceneNode(), cas );
	} else {
		casEmpty = false;
		auto casN( cas );
		bool foundRepeated = false;
		for ( const auto& c : casN ) {
			if ( mQuickFix.title == c.title ) {
				foundRepeated = true;
				break;
			}
		}
		if ( !foundRepeated ) {
			LSPCodeAction action{ mQuickFix.title,		mQuickFix.kind, {}, mQuickFix.edit, {},
								  mQuickFix.isPreferred };
			casN.insert( casN.begin(), action );
		}
		model = LSPCodeActionModel::create( mManager->getSplitter()->getUISceneNode(), casN );
	}

	createListView(
		editor, model,
		[this, editor]( const ModelEvent* modelEvent ) {
			if ( modelEvent->getModelEventType() != ModelEventType::Open )
				return;
			auto r = modelEvent->getModelIndex().row();
			const auto cam = static_cast<const LSPCodeActionModel*>( modelEvent->getModel() );
			if ( cam->hasCodeActions() && editorExists( editor ) ) {
				const auto& ca = cam->getCodeAction( r );

				if ( !ca.edit.changes.empty() || !ca.edit.documentChanges.empty() )
					mClientManager.applyWorkspaceEdit( ca.edit, []( const auto& ) {} );

				auto server = mClientManager.getOneLSPClientServer( editor->getDocumentRef() );
				if ( server && server->getCapabilities().executeCommandProvider )
					mClientManager.executeCommand( editor->getDocumentRef(), ca.command );
				editor->setFocus();
			} else if ( editorExists( editor ) ) {
				editor->setFocus();
			}
			modelEvent->getNode()->close();
		},
		[this, casEmpty, editor]( UIListView* lv ) {
			if ( casEmpty ) {
				lv->runOnMainThread(
					[this, editor] {
						if ( editorExists( editor ) &&
							 getManager()->getSplitter()->isCurEditor( editor ) ) {
							editor->setFocus();
						}
					},
					Seconds( 1 ) );
			}
		} );
}

PluginRequestHandle LSPClientPlugin::processCancelRequest( const PluginMessage& msg ) {
	if ( !msg.isBroadcast() || !msg.isJSON() )
		return {};

	auto server = getServerURIFromURI( mClientManager, msg.asJSON() );
	if ( !server )
		return {};

	return server->cancel( LSPClientServer::getID( msg.asJSON() ) );
}

PluginRequestHandle LSPClientPlugin::processMessage( const PluginMessage& msg ) {
	switch ( msg.type ) {
		case PluginMessageType::FileSystemListenerReady: {
			subscribeFileSystemListener();
			break;
		}
		case PluginMessageType::WorkspaceFolderChanged: {
			mClientManager.didChangeWorkspaceFolders( msg.asJSON()["folder"] );
			break;
		}
		case PluginMessageType::CodeCompletion: {
			auto ret = processCodeCompletionRequest( msg );
			if ( !ret.isEmpty() )
				return ret;
			break;
		}
		case PluginMessageType::SignatureHelp: {
			auto ret = processSignatureHelpRequest( msg );
			if ( !ret.isEmpty() )
				return ret;
			break;
		}
		case PluginMessageType::DocumentFormatting: {
			auto ret = processDocumentFormatting( msg );
			if ( !ret.isEmpty() )
				return ret;
			break;
		}
		case PluginMessageType::QueryPluginCapability: {
			if ( msg.isRequest() && msg.isJSON() ) {
				const auto& data = msg.asJSON();
				if ( !data.contains( "capability" ) || !data["capability"].is_number_integer() ||
					 data["capability"].get<int>() < 0 ||
					 data["capability"].get<int>() >= static_cast<int>( PluginCapability::Max ) )
					return {};
				PluginCapability capability = data["capability"].get<PluginCapability>();
				URI uri;
				if ( data.contains( "uri" ) && data["uri"].is_string() )
					uri = data["uri"];
				PluginInmediateResponse rmsg;
				rmsg.type = PluginMessageType::QueryPluginCapability;
				auto servers = mClientManager.getFilteredServers(
					[capability, uri]( LSPClientServer* server ) {
						if ( !uri.empty() && !server->hasDocument( uri ) )
							return false;
						switch ( capability ) {
							case PluginCapability::WorkspaceSymbol:
								return server->getCapabilities().workspaceSymbolProvider;
							case PluginCapability::TextDocumentSymbol:
								return server->getCapabilities().documentSymbolProvider;
							case PluginCapability::FoldingRange:
								return server->getCapabilities().foldingRangeProvider;
							default: {
							}
						}
						return false;
					} );
				rmsg.data = !servers.empty();
				return PluginRequestHandle( rmsg );
			}
			break;
		}
		case PluginMessageType::LanguageServerCapabilities: {
			if ( msg.isRequest() && msg.isJSON() ) {
				const auto& data = msg.asJSON();
				if ( !data.contains( "language" ) ) {
					return {};
				}
				auto server = mClientManager.getOneLSPClientServer( msg.asJSON()["language"] );
				if ( server ) {
					mManager->sendBroadcast( this, PluginMessageType::LanguageServerCapabilities,
											 PluginMessageFormat::LanguageServerCapabilities,
											 &server->getCapabilities() );
					return PluginRequestHandle::broadcast();
				}
			}
			break;
		}
		case PluginMessageType::CancelRequest: {
			processCancelRequest( msg );
			break;
		}
		case PluginMessageType::WorkspaceSymbol: {
			auto ret = processWorkspaceSymbol( msg );
			if ( !ret.isEmpty() )
				return ret;
			break;
		}
		case PluginMessageType::TextDocumentSymbol:
		case PluginMessageType::TextDocumentFlattenSymbol: {
			processTextDocumentSymbol( msg );
			break;
		}
		case PluginMessageType::DiagnosticsCodeAction: {
			processDiagnosticsCodeAction( msg );
			break;
		}
		case PluginMessageType::FoldingRanges: {
			processFoldingRanges( msg );
			break;
		}
		case PluginMessageType::WorkspaceDiagnostic:
		case PluginMessageType::LanguageServerReady: {
			processWorkspaceDiagnostic( msg );
			break;
		}
		default:
			break;
	}
	return PluginRequestHandle::empty();
}

void LSPClientPlugin::load( PluginManager* pluginManager ) {
	Clock clock;
	AtomicBoolScopedOp loading( mLoading, true );
	pluginManager->subscribeMessages( this,
									  [this]( const auto& notification ) -> PluginRequestHandle {
										  return processMessage( notification );
									  } );
	std::vector<std::string> paths;
	std::string path( pluginManager->getResourcesPath() + "plugins/lspclient.json" );
	if ( FileSystem::fileExists( path ) )
		paths.emplace_back( path );
	path = pluginManager->getPluginsPath() + "lspclient.json";
	if ( FileSystem::fileExists( path ) ||
		 FileSystem::fileWrite(
			 path, "{\n  \"config\":{},\n  \"keybindings\":{},\n  \"servers\":[]\n}\n" ) ) {
		mConfigPath = path;
		paths.emplace_back( path );
	}
	if ( paths.empty() )
		return;

	std::vector<LSPDefinition> lsps;

	for ( const auto& ipath : paths ) {
		try {
			loadLSPConfig( lsps, ipath, mConfigPath == ipath );
		} catch ( const json::exception& e ) {
			Log::error( "Parsing LSP \"%s\" failed:\n%s", ipath.c_str(), e.what() );
		}
	}

	std::unordered_map<std::string, std::unordered_map<std::string, std::string>> lspPatternsLangId;

	for ( auto& lsp : lsps ) {
		if ( lsp.shareProcessWithOtherDefinition && !lsp.usesLSP.empty() ) {
			for ( const auto& ptrn : lsp.filePatterns )
				lspPatternsLangId[lsp.usesLSP][ptrn] = lsp.language;
		}
	}

	for ( auto& lsp : lsps ) {
		auto ptrnsLangIds = lspPatternsLangId.find( lsp.name );
		if ( ptrnsLangIds != lspPatternsLangId.end() ) {
			lsp.languageIdsForFilePatterns.merge( ptrnsLangIds->second );
			for ( const auto& ptrn : lsp.filePatterns )
				lsp.languageIdsForFilePatterns[ptrn] = lsp.language;
		}
	}

	mClientManager.load( this, pluginManager, std::move( lsps ) );

	subscribeFileSystemListener();
	mReady = mClientManager.lspCount() > 0;
	for ( const auto& doc : mDelayedDocs )
		if ( mDocs.find( doc.first ) != mDocs.end() )
			mClientManager.tryRunServer( doc.second );
	mDelayedDocs.clear();
	if ( mReady ) {
		fireReadyCbs();
		setReady( clock.getElapsedTime() );
	}
}

static std::string parseCommand( nlohmann::json cmd, const std::string& workspaceFolder ) {
	std::string command;
	if ( cmd.is_string() ) {
		command = cmd.get<std::string>();
	} else if ( !cmd.is_object() ) {
		return "";
	} else {
		std::string platform( String::toLower( Sys::getPlatform() ) );
		if ( cmd.contains( platform ) && cmd[platform].is_string() ) {
			command = cmd[platform].get<std::string>();
		} else {
			platform = "other";
			if ( cmd.contains( platform ) && cmd[platform].is_string() )
				command = cmd[platform].get<std::string>();
		}
	}
	sanitizeCommand( command, workspaceFolder );
	return command;
}

static void tryAddEnv( const json& obj, LSPDefinition& lsp ) {
	if ( obj.contains( "env" ) && obj.is_array() ) {
		for ( const auto& obje : obj ) {
			if ( !obje.is_string() )
				continue;
			std::string envStr = obje.get<std::string>();
			if ( !envStr.empty() && envStr.find_first_of( "=" ) != std::string::npos ) {
				auto envp = String::split( envStr, '=' );
				if ( envp.size() == 2 ) {
					lsp.env[envp[0]] = envp[1];
				}
			}
		}
	}
}

void LSPClientPlugin::loadLSPConfig( std::vector<LSPDefinition>& lsps, const std::string& path,
									 bool updateConfigFile ) {
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;
	json j;
	try {
		j = json::parse( data, nullptr, true, true );
	} catch ( const json::exception& e ) {
		Log::error( "LSPClientPlugin::loadLSPConfig - Error parsing LSP config from "
					"path %s, error: %s, config file content:\n%s",
					path.c_str(), e.what(), data.c_str() );
		if ( !updateConfigFile )
			return;
		// Recreate it
		j = json::parse( "{\n  \"config\":{},\n  \"keybindings\":{},\n  \"servers\":[]\n}\n",
						 nullptr, true, true );
	}

	if ( updateConfigFile ) {
		mConfigHash = String::hash( data );
	}

	if ( j.contains( "config" ) ) {
		auto& config = j["config"];

		if ( config.contains( "silent" ) )
			mSilence = config.value( "silent", false );
		else if ( updateConfigFile )
			config["silent"] = mSilence;

		if ( config.contains( "trim_logs" ) )
			mTrimLogs = config.value( "trim_logs", false );
		else if ( updateConfigFile )
			config["trim_logs"] = mTrimLogs;

		if ( config.contains( "hover_delay" ) )
			setHoverDelay( Time::fromString( config["hover_delay"].get<std::string>() ) );
		else if ( updateConfigFile )
			config["hover_delay"] = getHoverDelay().toString();

		if ( config.contains( "server_close_after_idle_time" ) )
			mClientManager.setLSPDecayTime(
				Time::fromString( config["server_close_after_idle_time"].get<std::string>() ) );
		else if ( updateConfigFile )
			config["server_close_after_idle_time"] = mClientManager.getLSPDecayTime().toString();

		if ( config.contains( "semantic_highlighting" ) )
			mSemanticHighlighting = config.value( "semantic_highlighting", true );
		else if ( updateConfigFile )
			config["semantic_highlighting"] = mSemanticHighlighting;

		if ( config.contains( "disable_semantic_highlighting_lang" ) ) {
			try {
				mSemanticHighlightingDisabledLangs.clear();
				const auto& langs = config["disable_semantic_highlighting_lang"];
				for ( const auto& lang : langs ) {
					if ( lang.is_string() ) {
						std::string lg = lang.get<std::string>();
						if ( !lg.empty() )
							mSemanticHighlightingDisabledLangs.insert( lg );
					}
				}
			} catch ( const json::exception& e ) {
				Log::debug( "LSPClientPlugin::loadLSPConfig: Error parsing "
							"disable_semantic_highlighting_lang: %s",
							e.what() );
			}
		} else {
			config["disable_semantic_highlighting_lang"] = json::array();
		}

		if ( config.contains( "breadcrumb_navigation" ) )
			mBreadcrumb = config.value( "breadcrumb_navigation", true );
		else if ( updateConfigFile )
			config["breadcrumb_navigation"] = mBreadcrumb;

		if ( config.contains( "breadcrumb_height" ) )
			mBreadcrumbHeight = config.value( "breadcrumb_height", "20dp" );
		else if ( updateConfigFile )
			config["breadcrumb_height"] = mBreadcrumbHeight.toString();
	}

	if ( mKeyBindings.empty() ) {
		mKeyBindings["lsp-go-to-definition"] = "f2";
		mKeyBindings["lsp-go-to-implementation"] = "shift+f2";
		mKeyBindings["lsp-symbol-info"] = "f1";
		mKeyBindings["lsp-symbol-code-action"] = "alt+return";
		mKeyBindings["lsp-rename-symbol-under-cursor"] = "mod+shift+r";
		mKeyBindings["lsp-symbol-references"] = "mod+shift+u";
		mKeyBindings["lsp-format-range"] = "alt+shift+f";
		mKeyBindings["lsp-show-document-symbols"] = "mod+shift+l";
	}

	if ( j.contains( "keybindings" ) ) {
		auto& kb = j["keybindings"];
		auto list = { "lsp-go-to-definition",
					  "lsp-go-to-declaration",
					  "lsp-go-to-implementation",
					  "lspz-go-to-type-definition",
					  "lsp-switch-header-source",
					  "lsp-symbol-info",
					  "lsp-symbol-references",
					  "lsp-memory-usage",
					  "lsp-symbol-code-action",
					  "lsp-rename-symbol-under-cursor",
					  "lsp-refresh-semantic-highlighting",
					  "lsp-format-range",
					  "lsp-plugin-restart",
					  "lsp-show-document-symbols" };
		for ( const auto& key : list ) {
			if ( kb.contains( key ) ) {
				if ( !kb[key].empty() )
					mKeyBindings[key] = kb[key];
			} else if ( updateConfigFile )
				kb[key] = mKeyBindings[key];
		}
	}

	if ( updateConfigFile ) {
		std::string newData( j.dump( 2 ) );
		if ( newData != data ) {
			FileSystem::fileWrite( path, newData );
			mConfigHash = String::hash( newData );
		}
	}

	if ( !j.contains( "servers" ) )
		return;
	auto& servers = j["servers"];

	for ( auto& obj : servers ) {
		bool lspOverwritten = false;
		// Allow disabling an LSP by redeclaring it in the user configuration file.
		if ( updateConfigFile && ( obj.contains( "name" ) || obj.contains( "use" ) ) &&
			 obj.contains( "disabled" ) && obj.at( "disabled" ).is_boolean() ) {
			std::string name = obj.contains( "name" ) ? obj["name"] : obj["use"];
			for ( auto& lspD : lsps ) {
				if ( lspD.name == name ) {
					lspD.disabled = obj["disabled"].get<bool>();
					lspOverwritten = true;
				}
			}
		}

		// Allow overriding the command for already defined LSP
		// And allow adding parameters to the already defined LSP
		if ( updateConfigFile && ( obj.contains( "name" ) || obj.contains( "use" ) ) &&
			 ( obj.contains( "command" ) ||
			   ( obj.contains( "command_parameters" ) &&
				 obj.at( "command_parameters" ).is_string() ) ||
			   ( obj.contains( "host" ) && obj.at( "host" ).is_string() &&
				 obj.contains( "port " ) && obj.at( "port" ).is_number_integer() ) ) ) {
			for ( auto& lspR : lsps ) {
				std::string name = obj.contains( "name" ) ? obj["name"] : obj["use"];
				if ( lspR.name == name ) {
					lspOverwritten = true;
					if ( obj.contains( "use" ) && obj["use"].is_string() )
						lspR.usesLSP = obj["use"].get<std::string>();
					if ( obj.contains( "share_process" ) && obj["share_process"].is_boolean() ) {
						lspR.shareProcessWithOtherDefinition = obj["share_process"].get<bool>();
					}
					if ( obj.contains( "command" ) ) {
						lspR.command =
							parseCommand( obj["command"], mManager->getWorkspaceFolder() );
					}
					if ( !obj.value( "command_parameters", "" ).empty() ) {
						std::string cmdParam( obj.value( "command_parameters", "" ) );
						if ( !cmdParam.empty() && cmdParam.front() != ' ' )
							cmdParam = " " + cmdParam;
						lspR.commandParameters += cmdParam;
						sanitizeCommand( lspR.commandParameters, mManager->getWorkspaceFolder() );
					}
					if ( obj.contains( "host" ) ) {
						lspR.host = obj.value( "host", "" );
						lspR.port = obj.value( "port", 0 );
					}
					tryAddEnv( obj, lspR );
				}
			}
		}

		if ( lspOverwritten )
			continue;

		if ( !obj.contains( "language" ) || !obj.contains( "file_patterns" ) ) {
			Log::warning( "LSP server without language or file_patterns, ignored..." );
			continue;
		}

		if ( !obj.contains( "use" ) && !( obj.contains( "command" ) && obj.contains( "name" ) ) ) {
			Log::warning( "LSP server without name+command or use, ignored..." );
			continue;
		}

		LSPDefinition lsp;
		if ( obj.contains( "use" ) ) {
			std::string use = obj["use"];
			bool foundTlsp = false;
			for ( const auto& tlsp : lsps ) {
				if ( tlsp.name == use ) {
					lsp.language = obj["language"];
					foundTlsp = true;
					lsp.command = parseCommand( tlsp.command, mManager->getWorkspaceFolder() );
					lsp.name = tlsp.name;
					lsp.rootIndicationFileNames = tlsp.rootIndicationFileNames;
					lsp.url = tlsp.url;
					lsp.host = tlsp.host;
					lsp.port = tlsp.port;
					lsp.initializationOptions = tlsp.initializationOptions;
					lsp.usesLSP = use;
					if ( obj.contains( "share_process" ) && obj["share_process"].is_boolean() ) {
						lsp.shareProcessWithOtherDefinition = obj["share_process"].get<bool>();
					}
					break;
				}
			}
			if ( !foundTlsp ) {
				Log::warning( "LSP server trying to use an undeclared LSP. Father LSP must be "
							  "declared first." );
				continue;
			}
		} else {
			lsp.language = obj["language"];
			lsp.command = parseCommand( obj["command"], mManager->getWorkspaceFolder() );
			lsp.name = obj["name"];
			lsp.url = obj.value( "url", "" );
			lsp.host = obj.value( "host", "" );
			lsp.port = obj.value( "port", 0 );
			lsp.shareProcessWithOtherDefinition = obj.value( "share_process", false );
			if ( obj.contains( "vars" ) && obj.is_object() ) {
				auto vars = obj["vars"];
				for ( const auto& var : vars.items() ) {
					if ( var.value().is_string() )
						lsp.cmdVars[var.key()] = var.value().get<std::string>();
				}
			}
		}

		lsp.commandParameters = obj.value( "command_parameters", lsp.commandParameters );
		if ( obj.contains( "initializationOptions" ) )
			lsp.initializationOptions = obj["initializationOptions"];

		auto& fp = obj["file_patterns"];

		for ( auto& pattern : fp )
			lsp.filePatterns.push_back( pattern.get<std::string>() );

		if ( obj.contains( "rootIndicationFileNames" ) ) {
			lsp.rootIndicationFileNames.clear();
			auto& fnms = obj["rootIndicationFileNames"];
			for ( auto& fn : fnms )
				lsp.rootIndicationFileNames.push_back( fn );
		}

		sanitizeCommand( lsp.command, mManager->getWorkspaceFolder() );
		sanitizeCommand( lsp.commandParameters, mManager->getWorkspaceFolder() );

		tryAddEnv( obj, lsp );

		// If the file pattern is repeated, we will overwrite the previous LSP.
		// The previous LSP should be the "default" LSP that comes with ecode.
		size_t pos = lspFilePatternPosition( lsps, lsp.filePatterns );
		if ( pos != std::string::npos ) {
			lsps[pos] = lsp;
		} else {
			lsps.emplace_back( std::move( lsp ) );
		}
	}
}

size_t LSPClientPlugin::lspFilePatternPosition( const std::vector<LSPDefinition>& lsps,
												const std::vector<std::string>& patterns ) {
	for ( size_t i = 0; i < lsps.size(); ++i ) {
		for ( const std::string& filePattern : lsps[i].filePatterns ) {
			for ( const std::string& pattern : patterns ) {
				if ( filePattern == pattern ) {
					return i;
				}
			}
		}
	}
	return std::string::npos;
}

void LSPClientPlugin::getAndGoToLocation( UICodeEditor* editor, const std::string& search ) {
	mClientManager.getAndGoToLocation(
		editor->getDocumentRef(), search,
		[this, editor]( const LSPClientServer::IdType&, const std::vector<LSPLocation>& res ) {
			if ( res.empty() )
				return;
			if ( res.size() == 1 ) {
				mClientManager.goToLocation( res[0] );
			} else {
				createLocationsView( editor, res );
			}
		} );
}

void LSPClientPlugin::processDiagnosticsCodeAction( const PluginMessage& msg ) {
	if ( !( msg.isResponse() && msg.type == PluginMessageType::DiagnosticsCodeAction &&
			msg.format == PluginMessageFormat::DiagnosticsCodeAction ) )
		return;

	mQuickFix = msg.asDiasnosticsCodeAction();
}

void LSPClientPlugin::codeAction( UICodeEditor* editor ) {
	json j;
	j["uri"] = editor->getDocument().getURI().toString();
	j["pos"] = editor->getDocument().getSelection().start().toString();
	mQuickFix = {};
	auto req = mManager->sendRequest( PluginMessageType::DiagnosticsCodeAction,
									  PluginMessageFormat::JSON, &j );

	json j2;
	j2["uri"] = editor->getDocument().getURI().toString();
	j2["line"] = editor->getDocument().getSelection().start().line();
	j2["character"] = editor->getDocument().getSelection().start().column();

	auto resp =
		mManager->sendRequest( PluginMessageType::GetDiagnostics, PluginMessageFormat::JSON, &j2 );
	nlohmann::json diagnostics;
	if ( resp.isResponse() )
		diagnostics = std::move( resp.getResponse().data );

	mClientManager.codeAction(
		editor->getDocumentRef(), diagnostics,
		[this, editor]( const LSPClientServer::IdType&, const std::vector<LSPCodeAction>& res ) {
			createCodeActionsView( editor, res );
		} );
}

void LSPClientPlugin::renameSymbol( UICodeEditor* editor ) {
	UIMessageBox* msgBox = UIMessageBox::New(
		UIMessageBox::INPUT, editor->getUISceneNode()->i18n(
								 "new_symbol_under_cursor_name",
								 "New name (caution: not all references may be replaced)" ) );
	msgBox->setTitle( editor->getUISceneNode()->i18n( "rename", "Rename" ) );
	msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
	auto selection = editor->getDocument().getSelection();
	TextPosition selStart = selection.start();
	msgBox->getTextInput()->setText( editor->getDocument().getWordInPosition(
		selection.hasSelection() ? selection.end() : selStart ) );
	msgBox->getTextInput()->getDocument().selectAll();
	msgBox->showWhenReady();
	msgBox->addEventListener( Event::OnConfirm, [this, selStart, editor, msgBox]( const Event* ) {
		String newName( msgBox->getTextInput()->getText() );
		mClientManager.renameSymbol( editor->getDocumentRef()->getURI(), selStart, newName );
		msgBox->closeWindow();
	} );
	msgBox->addEventListener( Event::OnClose, [this]( const Event* ) {
		if ( mManager->getSplitter() && mManager->getSplitter()->getCurWidget() )
			mManager->getSplitter()->getCurWidget()->setFocus();
	} );
}

void LSPClientPlugin::onRegister( UICodeEditor* editor ) {
	Lock l( mDocMutex );
	mDocs.insert( editor->getDocumentRef().get() );

	for ( auto& kb : mKeyBindings ) {
		if ( !kb.second.empty() )
			editor->getKeyBindings().addKeybindString( kb.second, kb.first );
	}

	if ( editor->hasDocument() ) {
		auto& doc = editor->getDocument();

		doc.setCommand( "lsp-go-to-definition", [this]( TextDocument::Client* client ) {
			getAndGoToLocation( static_cast<UICodeEditor*>( client ), "textDocument/definition" );
		} );

		doc.setCommand( "lsp-rename-symbol-under-cursor", [this]( TextDocument::Client* client ) {
			renameSymbol( static_cast<UICodeEditor*>( client ) );
		} );

		doc.setCommand( "lsp-go-to-declaration", [this]( TextDocument::Client* client ) {
			getAndGoToLocation( static_cast<UICodeEditor*>( client ), "textDocument/declaration" );
		} );

		doc.setCommand( "lsp-go-to-implementation", [this]( TextDocument::Client* client ) {
			getAndGoToLocation( static_cast<UICodeEditor*>( client ),
								"textDocument/implementation" );
		} );

		doc.setCommand( "lsp-go-to-type-definition", [this]( TextDocument::Client* client ) {
			getAndGoToLocation( static_cast<UICodeEditor*>( client ),
								"textDocument/typeDefinition" );
		} );

		doc.setCommand( "lsp-switch-header-source", [this]( TextDocument::Client* client ) {
			switchSourceHeader( static_cast<UICodeEditor*>( client ) );
		} );

		doc.setCommand( "lsp-symbol-info", [this]( TextDocument::Client* client ) {
			getSymbolInfo( static_cast<UICodeEditor*>( client ) );
		} );

		doc.setCommand( "lsp-symbol-references", [this]( TextDocument::Client* client ) {
			mClientManager.getSymbolReferences(
				static_cast<UICodeEditor*>( client )->getDocumentRef() );
		} );

		doc.setCommand( "lsp-symbol-code-action", [this]( TextDocument::Client* client ) {
			codeAction( static_cast<UICodeEditor*>( client ) );
		} );

		doc.setCommand( "lsp-memory-usage", [this]( TextDocument::Client* client ) {
			mClientManager.memoryUsage( static_cast<UICodeEditor*>( client )->getDocumentRef() );
		} );

		doc.setCommand( "lsp-refresh-semantic-highlighting",
						[this]( TextDocument::Client* client ) {
							mClientManager.requestSymanticHighlighting(
								static_cast<UICodeEditor*>( client )->getDocumentRef() );
						} );

		doc.setCommand( "lsp-format-range", [this]( TextDocument::Client* client ) {
			mClientManager.rangeFormatting(
				static_cast<UICodeEditor*>( client )->getDocumentRef() );
		} );

		doc.setCommand( "lsp-plugin-restart", [this] { mManager->reload( getId() ); } );

		doc.setCommand( "lsp-show-document-symbols", [this]( TextDocument::Client* client ) {
			showDocumentSymbols( static_cast<UICodeEditor*>( client ) );
		} );
	}

	std::vector<Uint32> listeners;

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentLoaded, [this, editor]( const Event* ) {
			mEditorDocs[editor] = editor->getDocumentRef().get();
			mClientManager.run( editor->getDocumentRef() );
			updateCurrentSymbol( editor->getDocument() );
		} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnCursorPosChange, [this, editor]( const Event* ) {
			if ( mSymbolInfoShowing )
				hideTooltip( editor );
			updateCurrentSymbol( editor->getDocument() );
		} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentChanged, [this, editor]( const Event* event ) {
			const DocChangedEvent* docChangedEvent = static_cast<const DocChangedEvent*>( event );
			TextDocument* oldDoc = mEditorDocs[editor];
			TextDocument* newDoc = editor->getDocumentRef().get();
			{
				Lock l( mDocMutex );
				mDocs.erase( oldDoc );
				mEditorDocs[editor] = newDoc;
			}

			{
				Lock l( mDocCurrentSymbolsMutex );
				mDocCurrentSymbols.erase( docChangedEvent->getOldDocURI() );
			}

			updateCurrentSymbol( editor->getDocument() );
		} ) );

	if ( mBreadcrumb ) {
		mPluginTopSpace = mBreadcrumbHeight.asPixels(
			getUISceneNode()->getPixelsSize().getWidth(), getUISceneNode()->getPixelsSize(),
			getUISceneNode()->getDPI(), getUISceneNode()->getUIThemeManager()->getDefaultFontSize(),
			getUISceneNode()->getUIThemeManager()->getDefaultFontSize() );
		editor->registerTopSpace( this, mPluginTopSpace, 0 );
	}

	mEditors.insert( { editor, listeners } );
	mEditorDocs[editor] = editor->getDocumentRef().get();

	if ( mReady && editor->hasDocument() && editor->getDocument().hasFilepath() )
		mClientManager.run( editor->getDocumentRef() );
	if ( !mReady )
		mDelayedDocs[&editor->getDocument()] = editor->getDocumentRef();
}

void LSPClientPlugin::switchSourceHeader( UICodeEditor* editor ) {
	auto* server = mClientManager.getOneLSPClientServer( editor );
	if ( server )
		server->switchSourceHeader( editor->getDocument().getURI() );
}

void LSPClientPlugin::getSymbolInfo( UICodeEditor* editor ) {
	auto server = mClientManager.getOneLSPClientServer( editor );
	if ( server == nullptr )
		return;
	server->documentHover(
		editor->getDocument().getURI(), editor->getDocument().getSelection().start(),
		[this, editor]( const Int64&, const LSPHover& resp ) {
			json j;
			auto& doc = editor->getDocument();
			j["uri"] = doc.getURI().toString();
			j["line"] = doc.getSelection().start().line();
			j["character"] = doc.getSelection().start().column();
			auto errResp = mManager->sendRequest( PluginMessageType::GetErrorOrWarning,
												  PluginMessageFormat::JSON, &j );
			if ( !resp.contents.empty() && !resp.contents[0].value.empty() ) {
				editor->runOnMainThread( [editor, resp, errResp, this]() {
					mSymbolInfoShowing = true;
					if ( errResp.isResponse() ) {
						LSPHover cresp( resp );
						cresp.contents[0].value =
							errResp.getResponse().data["text"].get<std::string>() + "\n\n" +
							cresp.contents[0].value;
						displayTooltip(
							editor, cresp,
							editor
								->getScreenPosition( editor->getDocument().getSelection().start() )
								.getPosition() );
					} else {
						displayTooltip(
							editor, resp,
							editor
								->getScreenPosition( editor->getDocument().getSelection().start() )
								.getPosition() );
					}
				} );
			} else if ( errResp.isResponse() ) {
				LSPHover tresp;
				tresp.range =
					TextRange::fromString( errResp.getResponse().data["range"].get<std::string>() );
				tresp.contents.push_back(
					{ LSPMarkupKind::MarkDown,
					  errResp.getResponse().data["text"].get<std::string>() } );
				editor->runOnMainThread( [editor, tresp, this]() {
					mSymbolInfoShowing = true;
					displayTooltip(
						editor, tresp,
						editor->getScreenPosition( editor->getDocument().getSelection().start() )
							.getPosition() );
				} );
			} else {
				json data = getURIAndPositionJSON( editor );
				mManager->sendRequest( PluginMessageType::SignatureHelp, PluginMessageFormat::JSON,
									   &data );
			}
		} );
}

void LSPClientPlugin::onUnregister( UICodeEditor* editor ) {
	for ( auto& kb : mKeyBindings )
		editor->getKeyBindings().removeCommandKeybind( kb.first );

	if ( mShuttingDown )
		return;

	URI docURI;
	{
		Lock l( mDocMutex );
		TextDocument* doc = &editor->getDocument();
		docURI = doc->getURI();
		const auto& cbs = mEditors[editor];
		for ( auto listener : cbs )
			editor->removeEventListener( listener );

		if ( mBreadcrumb )
			editor->unregisterTopSpace( this );

		mEditors.erase( editor );
		mEditorDocs.erase( editor );
		for ( const auto& ieditor : mEditorDocs ) {
			if ( ieditor.second == doc )
				return;
		}

		if ( editor->hasDocument() )
			for ( auto& kb : mKeyBindings )
				editor->getDocument().removeCommand( kb.first );

		{
			Lock lds( mDocSymbolsMutex );
			mDocSymbols.erase( doc->getURI() );
			mDocFlatSymbols.erase( doc->getURI() );
		}

		mDocs.erase( doc );
	}

	{
		Lock l2( mDocCurrentSymbolsMutex );
		mDocCurrentSymbols.erase( docURI );
	}
}

bool LSPClientPlugin::onCreateContextMenu( UICodeEditor* editor, UIPopUpMenu* menu,
										   const Vector2i& /*position*/, const Uint32& /*flags*/ ) {
	auto* server = mClientManager.getOneLSPClientServer( editor );
	if ( !server )
		return false;

	menu->addSeparator();

	auto addFn = [this, menu]( const std::string& cmd, const std::string& text,
							   const std::string& icon = "" ) {
		menu->add( text, iconDrawable( icon, 12 ), KeyBindings::keybindFormat( mKeyBindings[cmd] ) )
			->setId( cmd );
	};
	auto& cap = server->getCapabilities();

	addFn( "lsp-symbol-info", i18n( "lsp_symbol_info", "Symbol Info" ) );

	if ( cap.definitionProvider )
		addFn( "lsp-go-to-definition", i18n( "lsp_go_to_definition", "Go To Definition" ) );

	if ( cap.declarationProvider )
		addFn( "lsp-go-to-declaration", i18n( "lsp_go_to_declaration", "Go To Declaration" ) );

	if ( cap.typeDefinitionProvider )
		addFn( "lsp-go-to-type-definition",
			   i18n( "lsp_go_to_type_definition", "Go To Type Definition" ) );

	if ( cap.implementationProvider )
		addFn( "lsp-go-to-implementation",
			   i18n( "lsp_go_to_implementation", "Go To Implementation" ) );

	if ( cap.renameProvider )
		addFn( "lsp-rename-symbol-under-cursor",
			   i18n( "lsp_rename_symbol_under_cursor", "Rename Symbol Under Cursor" ) );

	if ( cap.referencesProvider )
		addFn( "lsp-symbol-references", i18n( "lsp_find_references_to_symbol_under_cursor",
											  "Find References to Symbol Under Cursor" ) );

	if ( cap.codeActionProvider )
		addFn( "lsp-symbol-code-action", i18n( "lsp_code_action", "Code Action" ),
			   "lightbulb-autofix" );

	if ( cap.documentRangeFormattingProvider &&
		 editor->getDocument().getSelection().hasSelection() )
		addFn( "lsp-format-range", i18n( "lsp_format_selected_range", "Format Selected Range" ) );

	if ( cap.semanticTokenProvider.full || cap.semanticTokenProvider.fullDelta )
		addFn( "lsp-refresh-semantic-highlighting",
			   i18n( "lsp_refresh_semantic_highlighting", "Refresh Semantic Highlighting" ),
			   "refresh" );

	if ( cap.documentSymbolProvider )
		addFn( "lsp-show-document-symbols",
			   i18n( "show-document-symbols", "Show Document Symbols" ), "symbol-function" );

	if ( server->getDefinition().language == "cpp" || server->getDefinition().language == "c" )
		addFn( "lsp-switch-header-source",
			   i18n( "lsp_switch_header_source", "Switch Header/Source" ), "filetype-hpp" );

	addFn( "lsp-plugin-restart", i18n( "lsp_restart_lsp_client", "Restart LSP Client" ),
		   "refresh" );

#ifdef EE_DEBUG
	if ( server->getDefinition().name == "clangd" )
		addFn( "lsp-memory-usage", i18n( "lsp_memory_usage", "LSP Memory Usage" ) );
#endif

	return false;
}

void LSPClientPlugin::hideTooltip( UICodeEditor* editor ) {
	mSymbolInfoShowing = false;
	UITooltip* tooltip = nullptr;
	if ( editor && ( tooltip = editor->getTooltip() ) && tooltip->isVisible() &&
		 tooltip->getData() == String::hash( "lsp" ) ) {
		editor->setTooltipText( "" );
		tooltip->hide();
		// Restore old tooltip state
		tooltip->setData( 0 );
		tooltip->setFontStyle( mOldTextStyle );
		tooltip->setHorizontalAlign( mOldTextAlign );
		tooltip->setUsingCustomStyling( mOldUsingCustomStyling );
		tooltip->setDontAutoHideOnMouseMove( mOldDontAutoHideOnMouseMove );
		tooltip->setBackgroundColor( mOldBackgroundColor );
		tooltip->setWordWrap( mOldWordWrap );
		tooltip->setMaxWidthEq( mOldMaxWidth );
	}
}

TextPosition currentMouseTextPosition( UICodeEditor* editor ) {
	return editor->resolveScreenPosition(
		editor->getUISceneNode()->getWindow()->getInput()->getMousePos().asFloat() );
}

void LSPClientPlugin::tryHideTooltip( UICodeEditor* editor, const Vector2i& position ) {
	if ( !mCurrentHover.range.isValid() ||
		 ( mCurrentHover.range.isValid() &&
		   !mCurrentHover.range.contains( editor->resolveScreenPosition( position.asFloat() ) ) ) )
		hideTooltip( editor );
}

void LSPClientPlugin::displayTooltip( UICodeEditor* editor, const LSPHover& resp,
									  const Vector2f& position ) {
	// HACK: Gets the old font style to restore it when the tooltip is hidden
	UITooltip* tooltip = editor->createTooltip();
	if ( tooltip == nullptr )
		return;
	mOldWordWrap = tooltip->isWordWrap();
	mOldMaxWidth = tooltip->getMaxWidthEq();
	tooltip->setWordWrap( true );
	tooltip->setMaxWidthEq( "50vw" );
	editor->setTooltipText( resp.contents[0].value );
	mOldTextStyle = tooltip->getFontStyle();
	mOldTextAlign = tooltip->getHorizontalAlign();
	mOldDontAutoHideOnMouseMove = tooltip->dontAutoHideOnMouseMove();
	mOldUsingCustomStyling = tooltip->getUsingCustomStyling();
	mOldBackgroundColor = tooltip->getBackgroundColor();
	if ( Color::Transparent == mOldBackgroundColor ) {
		tooltip->reloadStyle( true, true, true, true );
		mOldBackgroundColor = tooltip->getBackgroundColor();
	}
	tooltip->setHorizontalAlign( UI_HALIGN_LEFT );
	tooltip->setPixelsPosition( tooltip->getTooltipPosition( position ) );
	tooltip->setDontAutoHideOnMouseMove( true );
	tooltip->setUsingCustomStyling( true );
	tooltip->setFontStyle( Text::Regular );
	tooltip->setData( String::hash( "lsp" ) );
	tooltip->setBackgroundColor( editor->getColorScheme().getEditorColor( "background"_sst ) );
	tooltip->getUIStyle()->setStyleSheetProperty( StyleSheetProperty(
		"background-color",
		editor->getColorScheme().getEditorColor( "background"_sst ).toHexString(), true,
		StyleSheetSelectorRule::SpecificityImportant ) );

	if ( tooltip->getText().empty() )
		return;

	const auto& syntaxDef = resp.contents[0].kind == LSPMarkupKind::MarkDown
								? SyntaxDefinitionManager::instance()->getByLSPName( "markdown" )
								: editor->getSyntaxDefinition();

	SyntaxTokenizer::tokenizeText( syntaxDef, editor->getColorScheme(), *tooltip->getTextCache(), 0,
								   0xFFFFFFFF, true, "\n\t " );

	tooltip->notifyTextChangedFromTextCache();

	if ( editor->hasFocus() && !tooltip->isVisible() &&
		 !tooltip->getTextCache()->getString().empty() )
		tooltip->show();
}

void LSPClientPlugin::tryDisplayTooltip( UICodeEditor* editor, const LSPHover& resp,
										 const Vector2i& position ) {
	TextPosition startCursorPosition = editor->resolveScreenPosition( position.asFloat() );
	TextPosition currentMousePosition = currentMouseTextPosition( editor );
	if ( startCursorPosition != currentMousePosition ||
		 !( !resp.contents.empty() && !resp.contents[0].value.empty() &&
			( ( resp.range.isValid() && resp.range.contains( startCursorPosition ) ) ||
			  !resp.range.isValid() ) ) )
		return;
	mCurrentHover = resp;
	displayTooltip( editor, resp, position.asFloat() );
}

bool LSPClientPlugin::onMouseMove( UICodeEditor* editor, const Vector2i& position,
								   const Uint32& flags ) {
	if ( mBreadcrumb ) {
		auto localPos( editor->convertToNodeSpace( position.asFloat() ) );
		if ( localPos.y < mPluginTopSpace && localPos.x < editor->getTopAreaWidth() &&
			 localPos.x >= 0 ) {
			if ( !mHoveringBreadcrumb ) {
				mHoveringBreadcrumb = editor;
				editor->invalidateDraw();
			}
			getUISceneNode()->setCursor( Cursor::Hand );
			return false;
		}
	}
	if ( mHoveringBreadcrumb ) {
		mHoveringBreadcrumb = nullptr;
		editor->invalidateDraw();
	}

	if ( flags != 0 ) {
		tryHideTooltip( editor, position );
		return false;
	}

	editor->debounce(
		[this, editor]() {
			if ( !editorExists( editor ) )
				return;
			auto server = mClientManager.getOneLSPClientServer( editor );
			if ( server == nullptr )
				return;
			server->documentHover(
				editor->getDocument().getURI(), currentMouseTextPosition( editor ),
				[this, editor]( const Int64&, const LSPHover& resp ) {
					if ( editorExists( editor ) && !resp.contents.empty() &&
						 !resp.contents[0].value.empty() ) {
						editor->runOnMainThread( [editor, resp, this]() {
							auto mousePos =
								editor->getUISceneNode()->getWindow()->getInput()->getMousePos();
							if ( !editor->getScreenRect().contains( mousePos.asFloat() ) )
								return;
							tryDisplayTooltip( editor, resp, mousePos );
						} );
					}
				} );
		},
		mHoverDelay, getMouseMoveHash( editor ) );
	tryHideTooltip( editor, position );
	return editor->getTooltip() && editor->getTooltip()->isVisible() && mSymbolInfoShowing;
}

bool LSPClientPlugin::onMouseLeave( UICodeEditor* editor, const Vector2i&, const Uint32& ) {
	if ( mHoveringBreadcrumb ) {
		mHoveringBreadcrumb = nullptr;
		editor->invalidateDraw();
	}

	return false;
}

void LSPClientPlugin::onFocusLoss( UICodeEditor* editor ) {
	hideTooltip( editor );
}

bool LSPClientPlugin::onKeyDown( UICodeEditor* editor, const KeyEvent& event ) {
	if ( event.getSanitizedMod() == 0 && event.getKeyCode() == KEY_ESCAPE && editor->getTooltip() &&
		 editor->getTooltip()->isVisible() ) {
		hideTooltip( editor );
	}

	return false;
}

const Time& LSPClientPlugin::getHoverDelay() const {
	return mHoverDelay;
}

void LSPClientPlugin::setHoverDelay( const Time& hoverDelay ) {
	mHoverDelay = hoverDelay;
}

void LSPClientPlugin::onVersionUpgrade( Uint32 oldVersion, Uint32 ) {
	if ( oldVersion <= ECODE_VERSIONNUM( 0, 5, 0, 0 ) ) {
		mSemanticHighlighting = true;
	}
}

void LSPClientPlugin::drawTop( UICodeEditor* editor, const Vector2f& screenStart, const Sizef& size,
							   const Float& /*fontSize*/ ) {
	Float width = editor->getTopAreaWidth();
	Primitives p;
	Color backColor( editor->getColorScheme().getEditorColor( SyntaxStyleTypes::Background ) );
	p.setColor( backColor );
	p.drawRectangle( Rectf( screenStart, Sizef( width, mPluginTopSpace ) ) );

	Color lineColor( editor->getColorScheme().getEditorColor( SyntaxStyleTypes::LineBreakColumn ) );
	p.setColor( lineColor );
	Float lineHeight = eefloor( PixelDensity::dpToPxI( 1 ) );
	p.drawRectangle( { { screenStart.x, screenStart.y + size.getHeight() - lineHeight },
					   Sizef( width, lineHeight ) } );

	Font* font = getUISceneNode()->getUIThemeManager()->getDefaultFont();
	if ( !font )
		return;

	Float fontSize = editor->getUISceneNode()->getUIThemeManager()->getDefaultFontSize();

	// Avoid heap allocating
	static constexpr auto MAX_PATH_SIZE = 512;
	String::StringBaseType utf32Path[MAX_PATH_SIZE];
	std::memset( utf32Path, 0, MAX_PATH_SIZE * sizeof( String::StringBaseType ) );

	const auto& workspace = getManager()->getWorkspaceFolder();
	std::string_view path( editor->getDocument().getFilePath() );
	if ( !workspace.empty() && String::startsWith( path, workspace ) )
		path = path.substr( workspace.size() );
	if ( path.size() > MAX_PATH_SIZE )
		path = path.substr( path.size() - MAX_PATH_SIZE );

	size_t pathLen = String::toUtf32( path, &utf32Path[0], MAX_PATH_SIZE );

	Color textColor( editor->getColorScheme().getEditorColor(
		mHoveringBreadcrumb == editor ? SyntaxStyleTypes::Text : SyntaxStyleTypes::LineNumber2 ) );
	Float textOffsetY = eefloor( ( size.getHeight() - font->getLineSpacing( fontSize ) ) * 0.5f );

	Vector2f pos( screenStart.x + eefloor( PixelDensity::dpToPx( 8 ) ),
				  screenStart.y + textOffsetY );

	auto drawn = Text::draw( String::View( utf32Path, pathLen ), pos, font, fontSize, textColor );

	Lock l( mDocCurrentSymbolsMutex );
	auto symbolsInfoIt = mDocCurrentSymbols.find( editor->getDocument().getURI() );
	if ( symbolsInfoIt == mDocCurrentSymbols.end() )
		return;

	pos.x += drawn.getWidth();
	if ( mDrawSepIcon == nullptr )
		mDrawSepIcon = getUISceneNode()->findIcon( "chevron-right" );
	Float textHeight = drawn.getHeight();

	const auto& symbolsInfo = symbolsInfoIt->second;

	for ( const auto& info : symbolsInfo ) {
		if ( mDrawSepIcon ) {
			pos.x += eefloor( PixelDensity::dpToPx( 8 ) );
			Float iconSize = PixelDensity::dpToPxI( drawn.getHeight() * 0.5f );
			auto iconDrawable = mDrawSepIcon->getSize( iconSize );
			Color c = iconDrawable->getColor();
			iconDrawable->setColor( textColor );
			Float iconHeight = iconDrawable->getPixelsSize().getHeight();
			Vector2f iconPos( { pos.x, screenStart.y + textOffsetY +
										   eefloor( ( textHeight - iconHeight ) * 0.5f ) } );
			iconDrawable->draw( iconPos );
			pos.x +=
				iconDrawable->getPixelsSize().getWidth() + eefloor( PixelDensity::dpToPx( 8 ) );
			iconDrawable->setColor( c );
		} else {
			pos.x += eefloor( PixelDensity::dpToPx( 16 ) );
		}

		UIIcon* iconKind = getUISceneNode()->findIcon( info.icon );
		if ( iconKind ) {
			auto iconDrawable = iconKind->getSize( fontSize );
			Color c = iconDrawable->getColor();
			iconDrawable->setColor( textColor );
			Float iconHeight = iconDrawable->getPixelsSize().getHeight();
			iconDrawable->draw( { pos.x, screenStart.y + textOffsetY +
											 eefloor( ( textHeight - iconHeight ) * 0.5f ) } );
			pos.x += iconDrawable->getPixelsSize().getWidth() + PixelDensity::dpToPxI( 4 );
			iconDrawable->setColor( c );
		}

		drawn = Text::draw( info.name, pos, font, fontSize, textColor );
		pos.x += drawn.getWidth();
	}
}

void LSPClientPlugin::updateCurrentSymbol( TextDocument& doc ) {
	if ( !mBreadcrumb )
		return;

	std::vector<DisplaySymbolInfo> symbolsInfo;
	URI uri = doc.getURI();

	{
		mDocSymbolsMutex.lock();

		auto symbolsIt = mDocSymbols.find( uri );
		if ( symbolsIt == mDocSymbols.end() ) {
			mDocSymbolsMutex.unlock();
			Lock l( mDocCurrentSymbolsMutex );
			mDocCurrentSymbols[uri] = {};
			return;
		}

		LSPSymbolInformationList* list = &symbolsIt->second;
		auto sel = doc.getSelection();
		LSPSymbolInformationList::iterator foundIt;

		bool found = false;
		do {
			foundIt = std::lower_bound( list->begin(), list->end(), sel,
										[]( const LSPSymbolInformation& cur,
											const TextRange& sel ) { return cur.range < sel; } );
			found = foundIt != list->end() && foundIt->range.contains( sel );
			if ( found ) {
				symbolsInfo.push_back( { String::fromUtf8( foundIt->name ),
										 LSPSymbolKindHelper::toIconString( foundIt->kind ) } );
				if ( foundIt->children.empty() )
					break;
				list = &foundIt->children;
			} else
				break;
		} while ( found );

		mDocSymbolsMutex.unlock();
	}

	Lock l( mDocCurrentSymbolsMutex );
	auto prevSymbols = mDocCurrentSymbols.find( uri );
	if ( prevSymbols == mDocCurrentSymbols.end() || prevSymbols->second != symbolsInfo ) {
		mDocCurrentSymbols[uri] = std::move( symbolsInfo );
		getManager()->getSplitter()->forEachEditor( [&uri]( UICodeEditor* editor ) {
			if ( editor->isVisible() && editor->getDocument().getURI() == uri )
				editor->invalidateDraw();
		} );
	}
}

void LSPClientPlugin::showDocumentSymbols( UICodeEditor* editor ) {
	TextDocument& doc = editor->getDocument();
	URI uri = doc.getURI();

	auto model = createDocSymbolsModel( uri );
	if ( !model )
		return;

	getUISceneNode()->getRoot()->setFocus();
	UIWindow* win = UIWindow::NewOpt( UIMessageBox::WindowBaseContainerType::LINEAR_LAYOUT );
	win->setMinWindowSize( 400, getUISceneNode()->getSize().getHeight() * 0.7f );
	win->setKeyBindingCommand( "closeWindow", [win, editor] {
		win->closeWindow();
		editor->setFocus();
	} );
	win->getKeyBindings().addKeybind( { KEY_ESCAPE }, "closeWindow" );
	win->setWindowFlags( UI_WIN_NO_DECORATION | UI_WIN_SHADOW | UI_WIN_MODAL | UI_WIN_EPHEMERAL );
	win->center();
	UITextInput* input = UITextInput::New();
	input->setParent( win->getContainer() );
	input->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::WrapContent );
	input->setHint( i18n( "search_symbols_ellipsis", "Search Symbols..." ) );

	UITreeView* tv = UITreeView::New();
	tv->setHeadersVisible( false );
	tv->setAutoExpandOnSingleColumn( true );
	tv->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::Fixed );
	tv->setLayoutWeight( 1 );
	tv->setParent( win->getContainer() );
	tv->setModel( model );
	tv->expandAll();
	tv->setFocusOnSelection( false );

	{
		Lock l( mDocCurrentSymbolsMutex );
		auto symbolsIt = mDocCurrentSymbols.find( uri );
		ModelIndex index;
		if ( symbolsIt != mDocCurrentSymbols.end() ) {
			auto docSymbols = symbolsIt->second;
			std::vector<std::string> path;
			for ( const auto& sym : docSymbols )
				path.emplace_back( sym.name );
			tv->selectRowWithPath( path );
		}
	}

	win->show();
	input->setFocus();

	input->on( Event::KeyDown, [tv, input]( const Event* event ) {
		tv->forceKeyDown( *event->asKeyEvent() );
		input->setFocus();
	} );

	input->on( Event::OnTextChanged, [tv, input, uri, this]( const Event* ) {
		tv->setModel( createDocSymbolsModel( uri, input->getText().toUtf8() ) );
		tv->expandAll();
	} );

	tv->setOnSelection( [editor]( const ModelIndex& index ) {
		LSPSymbolInformation* node = static_cast<LSPSymbolInformation*>( index.internalData() );
		editor->getDocument().setSelection( node->range.start() );
		editor->scrollTo( node->range.start(), true );
	} );

	input->on( Event::OnPressEnter,
			   [win]( const Event* ) { win->executeKeyBindingCommand( "closeWindow" ); } );

	tv->on( Event::OnModelEvent, [win]( const Event* event ) {
		const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
		if ( modelEvent->getModelEventType() == ModelEventType::Open )
			win->executeKeyBindingCommand( "closeWindow" );
	} );
}

std::shared_ptr<LSPSymbolInfoTreeModel>
LSPClientPlugin::createDocSymbolsModel( URI uri, const std::string& query ) {
	LSPSymbolInformationList docSymbolsCopy;
	{
		Lock l( mDocSymbolsMutex );
		auto docSymbols = mDocSymbols.find( uri );
		if ( docSymbols == mDocSymbols.end() )
			return nullptr;
		docSymbolsCopy = docSymbols->second;
	}
	return LSPSymbolInfoTreeModel::create( getUISceneNode(), std::move( docSymbolsCopy ), query );
}

const LSPClientServerManager& LSPClientPlugin::getClientManager() const {
	return mClientManager;
}

} // namespace ecode
