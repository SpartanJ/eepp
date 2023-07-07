#include "lspclientplugin.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/tools/uicodeeditorsplitter.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uilistview.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>
#include <nlohmann/json.hpp>

using namespace EE::Window;
using json = nlohmann::json;

namespace ecode {

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

	void update() override { onModelUpdate(); }

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

	void update() override { onModelUpdate(); }

	const LSPCodeAction& getCodeAction( size_t row ) const { return mCodeActions[row]; }

  protected:
	UISceneNode* mUISceneNode;
	std::vector<LSPCodeAction> mCodeActions;
};

UICodeEditorPlugin* LSPClientPlugin::New( PluginManager* pluginManager ) {
	return eeNew( LSPClientPlugin, ( pluginManager, false ) );
}

UICodeEditorPlugin* LSPClientPlugin::NewSync( PluginManager* pluginManager ) {
	return eeNew( LSPClientPlugin, ( pluginManager, true ) );
}

LSPClientPlugin::LSPClientPlugin( PluginManager* pluginManager, bool sync ) :
	Plugin( pluginManager ) {
	if ( sync ) {
		load( pluginManager );
	} else {
		mThreadPool->run( [&, pluginManager] { load( pluginManager ); } );
	}
}

LSPClientPlugin::~LSPClientPlugin() {
	mShuttingDown = true;
	mManager->unsubscribeMessages( this );
	unsubscribeFileSystemListener();
	Lock l( mDocMutex );
	for ( const auto& editor : mEditors ) {
		for ( auto& kb : mKeyBindings ) {
			editor.first->getKeyBindings().removeCommandKeybind( kb.first );
			if ( editor.first->hasDocument() )
				editor.first->getDocument().removeCommand( kb.first );
		}
		editor.first->unregisterPlugin( this );
	}
	if ( nullptr == mManager->getSplitter() )
		return;
	for ( const auto& editor : mEditorsTags ) {
		if ( mManager->getSplitter()->editorExists( editor.first ) ) {
			for ( const auto& tag : editor.second )
				editor.first->removeActionsByTag( tag );
		}
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

static void sanitizeCommand( std::string& cmd ) {
	String::replaceAll( cmd, "$NPROC", String::toString( Sys::getCPUCount() ) );
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
		[&, server]( const PluginIDType&, const std::vector<LSPTextEdit>& edits ) {
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

	std::string query = msg.asJSON().value( "query", "" );
	for ( const auto server : servers ) {
		server->workspaceSymbol(
			query, [&, query]( const PluginIDType& id, LSPSymbolInformationList&& info ) {
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

	return {};
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
	if ( !symbols.empty() ) {
		mManager->sendResponse( this, msg.type, PluginMessageFormat::SymbolInformation, &symbols,
								uri.toString() );
		return { uri.toString() };
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
	mDocSymbols[docURI] = std::move( res );
}

void LSPClientPlugin::setDocumentSymbolsFromResponse( const PluginIDType& id, const URI& docURI,
													  LSPSymbolInformationList&& res ) {
	setDocumentSymbols( docURI, std::move( res ) );
	mManager->sendResponse( this, PluginMessageType::TextDocumentSymbol,
							PluginMessageFormat::SymbolInformation, &getDocumentSymbols( docURI ),
							id );
	mManager->sendResponse( this, PluginMessageType::TextDocumentFlattenSymbol,
							PluginMessageFormat::SymbolInformation,
							&getDocumentFlattenSymbols( docURI ), id );
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
	Input* input = editor->getUISceneNode()->getWindow()->getInput();
	Uint32 mod = input->getSanitizedModState();
	if ( mod != ( KEYMOD_LALT | KEYMOD_CTRL ) || ( flags & EE_BUTTON_LMASK ) == 0 )
		return false;

	auto docPos = editor->resolveScreenPosition( pos.asFloat() );
	if ( !docPos.isValid() || !editor->getDocument().isValidPosition( docPos ) )
		return false;

	editor->getDocument().execute( "lsp-go-to-definition" );

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
		lv->setModel( model );
		lv->setSelection( model->index( 0, 0 ) );
		Float colWidth = lv->getMaxColumnContentWidth( 0 ) + PixelDensity::dpToPx( 4 );
		lv->setColumnWidth( 0, colWidth );
		lv->setPixelsSize(
			{ colWidth, std::min( lv->getContentSize().y, lv->getRowHeight() * 8 ) } );
		lv->setPixelsPosition( { pos.x, pos.y + editor->getLineHeight() } );
		if ( !lv->getParent()->getLocalBounds().contains(
				 lv->getLocalBounds().setPosition( lv->getPixelsPosition() ) ) ) {
			lv->setPixelsPosition( { pos.x, pos.y - lv->getPixelsSize().getHeight() } );
		}
		lv->setVisible( true );
		if ( onCreateCb )
			onCreateCb( lv );
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
		lv->on( Event::OnModelEvent, [&, onModelEventCb]( const Event* event ) {
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
		default:
			break;
	}
	return PluginRequestHandle::empty();
}

void LSPClientPlugin::load( PluginManager* pluginManager ) {
	BoolScopedOp loading( mLoading, true );
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

	mClientManager.load( this, pluginManager, std::move( lsps ) );

	mReady = mClientManager.lspCount() > 0;
	for ( const auto& doc : mDelayedDocs )
		if ( mDocs.find( doc.first ) != mDocs.end() )
			mClientManager.tryRunServer( doc.second );
	mDelayedDocs.clear();
	if ( mReady )
		fireReadyCbs();

	subscribeFileSystemListener();
}

static std::string parseCommand( nlohmann::json cmd ) {
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
	sanitizeCommand( command );
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
			mSemanticHighlighting = config.value( "semantic_highlighting", false );
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
	}

	if ( mKeyBindings.empty() ) {
		mKeyBindings["lsp-go-to-definition"] = "f2";
		mKeyBindings["lsp-go-to-implementation"] = "shift+f2";
		mKeyBindings["lsp-symbol-info"] = "f1";
		mKeyBindings["lsp-symbol-code-action"] = "alt+return";
		mKeyBindings["lsp-rename-symbol-under-cursor"] = "mod+shift+r";
		mKeyBindings["lsp-symbol-references"] = "mod+shift+u";
		mKeyBindings["lsp-format-range"] = "alt+shift+f";
	}

	if ( j.contains( "keybindings" ) ) {
		auto& kb = j["keybindings"];
		auto list = { "lsp-go-to-definition",
					  "lsp-go-to-declaration",
					  "lsp-go-to-implementation",
					  "lsp-go-to-type-definition",
					  "lsp-switch-header-source",
					  "lsp-symbol-info",
					  "lsp-symbol-references",
					  "lsp-memory-usage",
					  "lsp-symbol-code-action",
					  "lsp-rename-symbol-under-cursor",
					  "lsp-refresh-semantic-highlighting",
					  "lsp-format-range" };
		for ( const auto& key : list ) {
			if ( kb.contains( key ) ) {
				if ( !kb[key].empty() )
					mKeyBindings[key] = kb[key];
			} else if ( updateConfigFile )
				kb[key] = mKeyBindings[key];
		}
	}

	if ( updateConfigFile ) {
		FileSystem::fileWrite( path, j.dump( 2 ) );
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
						lspR.command = parseCommand( obj["command"] );
					}
					if ( !obj.value( "command_parameters", "" ).empty() ) {
						std::string cmdParam( obj.value( "command_parameters", "" ) );
						if ( !cmdParam.empty() && cmdParam.front() != ' ' )
							cmdParam = " " + cmdParam;
						lspR.commandParameters += cmdParam;
						sanitizeCommand( lspR.commandParameters );
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
					lsp.command = parseCommand( tlsp.command );
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
			lsp.command = parseCommand( obj["command"] );
			lsp.name = obj["name"];
			lsp.url = obj.value( "url", "" );
			lsp.host = obj.value( "host", "" );
			lsp.port = obj.value( "port", 0 );
			lsp.shareProcessWithOtherDefinition = obj.value( "share_process", false );
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

		sanitizeCommand( lsp.command );
		sanitizeCommand( lsp.commandParameters );

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
		[&, editor]( const LSPClientServer::IdType&, const std::vector<LSPLocation>& res ) {
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
		[&, editor]( const LSPClientServer::IdType&, const std::vector<LSPCodeAction>& res ) {
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
	TextPosition pos = editor->getDocument().getSelection().start();
	msgBox->getTextInput()->setText(
		editor->getDocument().getWordInPosition( editor->getDocument().getSelection().start() ) );
	msgBox->getTextInput()->getDocument().selectAll();
	msgBox->showWhenReady();
	msgBox->addEventListener( Event::OnConfirm, [this, pos, editor, msgBox]( const Event* ) {
		String newName( msgBox->getTextInput()->getText() );
		mClientManager.renameSymbol( editor->getDocumentRef()->getURI(), pos, newName );
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

		doc.setCommand( "lsp-go-to-definition", [this, editor]() {
			getAndGoToLocation( editor, "textDocument/definition" );
		} );

		doc.setCommand( "lsp-rename-symbol-under-cursor",
						[this, editor]() { renameSymbol( editor ); } );

		doc.setCommand( "lsp-go-to-declaration", [this, editor]() {
			getAndGoToLocation( editor, "textDocument/declaration" );
		} );

		doc.setCommand( "lsp-go-to-implementation", [this, editor]() {
			getAndGoToLocation( editor, "textDocument/implementation" );
		} );

		doc.setCommand( "lsp-go-to-type-definition", [this, editor]() {
			getAndGoToLocation( editor, "textDocument/typeDefinition" );
		} );

		doc.setCommand( "lsp-switch-header-source",
						[this, editor]() { switchSourceHeader( editor ); } );

		doc.setCommand( "lsp-symbol-info", [this, editor]() { getSymbolInfo( editor ); } );

		doc.setCommand( "lsp-symbol-references", [this, editor] {
			mClientManager.getSymbolReferences( editor->getDocumentRef() );
		} );

		doc.setCommand( "lsp-symbol-code-action", [this, editor] { codeAction( editor ); } );

		doc.setCommand( "lsp-memory-usage", [this, editor] {
			mClientManager.memoryUsage( editor->getDocumentRef() );
		} );

		doc.setCommand( "lsp-refresh-semantic-highlighting", [this, editor] {
			mClientManager.requestSymanticHighlighting( editor->getDocumentRef() );
		} );

		doc.setCommand( "lsp-format-range", [this, editor] {
			mClientManager.rangeFormatting( editor->getDocumentRef() );
		} );
	}

	std::vector<Uint32> listeners;

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentLoaded, [&, editor]( const Event* ) {
			mClientManager.run( editor->getDocumentRef() );
		} ) );

	listeners.push_back(
		editor->addEventListener( Event::OnCursorPosChange, [&, editor]( const Event* ) {
			if ( mSymbolInfoShowing )
				hideTooltip( editor );
		} ) );

	mEditors.insert( { editor, listeners } );
	mEditorsTags.insert( { editor, {} } );
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
		[&, editor]( const Int64&, const LSPHover& resp ) {
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
			}
		} );
}

void LSPClientPlugin::onUnregister( UICodeEditor* editor ) {
	for ( auto& kb : mKeyBindings )
		editor->getKeyBindings().removeCommandKeybind( kb.first );

	if ( mShuttingDown )
		return;

	Lock l( mDocMutex );
	TextDocument* doc = &editor->getDocument();
	const auto& cbs = mEditors[editor];
	for ( auto listener : cbs )
		editor->removeEventListener( listener );
	mEditors.erase( editor );
	mEditorsTags.erase( editor );
	mEditorDocs.erase( editor );
	for ( const auto& ieditor : mEditorDocs ) {
		if ( ieditor.second == doc )
			return;
	}

	if ( editor->hasDocument() ) {
		for ( auto& kb : mKeyBindings )
			editor->getDocument().removeCommand( kb.first );
	}

	{
		Lock lds( mDocSymbolsMutex );
		mDocSymbols.erase( doc->getURI() );
		mDocFlatSymbols.erase( doc->getURI() );
	}

	mDocs.erase( doc );
}

bool LSPClientPlugin::onCreateContextMenu( UICodeEditor* editor, UIPopUpMenu* menu,
										   const Vector2i& /*position*/, const Uint32& /*flags*/ ) {
	auto* server = mClientManager.getOneLSPClientServer( editor );
	if ( !server )
		return false;

	menu->addSeparator();

	auto addFn = [this, editor, menu]( const std::string& txtKey, const std::string& txtVal,
									   const std::string& icon = "" ) {
		menu->add( editor->getUISceneNode()->i18n( txtKey, txtVal ),
				   !icon.empty() ? mManager->getUISceneNode()->findIcon( icon )->getSize(
									   PixelDensity::dpToPxI( 12 ) )
								 : nullptr,
				   KeyBindings::keybindFormat( mKeyBindings[txtKey] ) )
			->setId( txtKey );
	};
	auto& cap = server->getCapabilities();

	addFn( "lsp-symbol-info", "Symbol Info" );

	if ( cap.definitionProvider )
		addFn( "lsp-go-to-definition", "Go To Definition" );

	if ( cap.declarationProvider )
		addFn( "lsp-go-to-declaration", "Go To Declaration" );

	if ( cap.typeDefinitionProvider )
		addFn( "lsp-go-to-type-definition", "Go To Type Definition" );

	if ( cap.implementationProvider )
		addFn( "lsp-go-to-implementation", "Go To Implementation" );

	if ( cap.renameProvider )
		addFn( "lsp-rename-symbol-under-cursor", "Rename Symbol Under Cursor" );

	if ( cap.referencesProvider )
		addFn( "lsp-symbol-references", "Find References to Symbol Under Cursor" );

	if ( cap.codeActionProvider )
		addFn( "lsp-symbol-code-action", "Code Action", "lightbulb-autofix" );

	if ( cap.documentRangeFormattingProvider &&
		 editor->getDocument().getSelection().hasSelection() )
		addFn( "lsp-format-range", "Format Selected Range" );

	if ( cap.semanticTokenProvider.full || cap.semanticTokenProvider.fullDelta )
		addFn( "lsp-refresh-semantic-highlighting", "Refresh Semantic Highlighting", "refresh" );

	if ( server->getDefinition().language == "cpp" || server->getDefinition().language == "c" )
		addFn( "lsp-switch-header-source", "Switch Header/Source", "filetype-hpp" );

#ifdef EE_DEBUG
	if ( server->getDefinition().name == "clangd" )
		addFn( "lsp-memory-usage", "LSP Memory Usage" );
#endif

	return false;
}

void LSPClientPlugin::hideTooltip( UICodeEditor* editor ) {
	mSymbolInfoShowing = false;
	UITooltip* tooltip = nullptr;
	if ( editor && ( tooltip = editor->getTooltip() ) && tooltip->isVisible() ) {
		editor->setTooltipText( "" );
		tooltip->hide();
		// Restore old tooltip state
		tooltip->setFontStyle( mOldTextStyle );
		tooltip->setHorizontalAlign( mOldTextAlign );
		tooltip->setUsingCustomStyling( mOldUsingCustomStyling );
		tooltip->setDontAutoHideOnMouseMove( mOldDontAutoHideOnMouseMove );
	}
}

TextPosition currentMouseTextPosition( UICodeEditor* editor ) {
	return editor->resolveScreenPosition(
		editor->getUISceneNode()->getWindow()->getInput()->getMousePosf() );
}

void LSPClientPlugin::tryHideTooltip( UICodeEditor* editor, const Vector2i& position ) {
	TextPosition cursorPosition = editor->resolveScreenPosition( position.asFloat() );
	if ( !mCurrentHover.range.isValid() ||
		 ( mCurrentHover.range.isValid() && !mCurrentHover.range.contains( cursorPosition ) ) )
		hideTooltip( editor );
}

void LSPClientPlugin::displayTooltip( UICodeEditor* editor, const LSPHover& resp,
									  const Vector2f& position ) {
	editor->setTooltipText( resp.contents[0].value );
	// HACK: Gets the old font style to restore it when the tooltip is hidden
	UITooltip* tooltip = editor->createTooltip();
	if ( tooltip == nullptr )
		return;
	mOldTextStyle = tooltip->getFontStyle();
	mOldTextAlign = tooltip->getHorizontalAlign();
	mOldDontAutoHideOnMouseMove = tooltip->dontAutoHideOnMouseMove();
	mOldUsingCustomStyling = tooltip->getUsingCustomStyling();
	tooltip->setHorizontalAlign( UI_HALIGN_LEFT );
	tooltip->setPixelsPosition( tooltip->getTooltipPosition( position ) );
	tooltip->setDontAutoHideOnMouseMove( true );
	tooltip->setUsingCustomStyling( true );
	tooltip->setFontStyle( Text::Regular );

	const auto& syntaxDef = resp.contents[0].kind == LSPMarkupKind::MarkDown
								? SyntaxDefinitionManager::instance()->getByLSPName( "markdown" )
								: editor->getSyntaxDefinition();

	SyntaxTokenizer::tokenizeText( syntaxDef, editor->getColorScheme(), *tooltip->getTextCache(), 0,
								   0xFFFFFFFF, true, "\n\t " );

	tooltip->notifyTextChangedFromTextCache();

	if ( editor->hasFocus() && !tooltip->isVisible() )
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
	if ( flags != 0 ) {
		tryHideTooltip( editor, position );
		return false;
	}
	String::HashType tag = String::hash( editor->getDocument().getFilePath() );
	editor->removeActionsByTag( tag );
	mEditorsTags[editor].insert( tag );
	editor->runOnMainThread(
		[&, editor, position, tag]() {
			mEditorsTags[editor].erase( tag );
			if ( !editorExists( editor ) )
				return;
			auto server = mClientManager.getOneLSPClientServer( editor );
			if ( server == nullptr )
				return;
			server->documentHover(
				editor->getDocument().getURI(), currentMouseTextPosition( editor ),
				[&, editor, position]( const Int64&, const LSPHover& resp ) {
					if ( editorExists( editor ) && !resp.contents.empty() &&
						 !resp.contents[0].value.empty() ) {
						editor->runOnMainThread( [editor, resp, position, this]() {
							tryDisplayTooltip( editor, resp, position );
						} );
					}
				} );
		},
		mHoverDelay, tag );
	tryHideTooltip( editor, position );
	return editor->getTooltip() && editor->getTooltip()->isVisible();
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

const LSPClientServerManager& LSPClientPlugin::getClientManager() const {
	return mClientManager;
}

} // namespace ecode
