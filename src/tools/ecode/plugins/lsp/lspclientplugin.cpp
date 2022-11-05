#include "lspclientplugin.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitooltip.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>
#include <nlohmann/json.hpp>

using namespace EE::Window;
using json = nlohmann::json;

namespace ecode {

UICodeEditorPlugin* LSPClientPlugin::New( const PluginManager* pluginManager ) {
	return eeNew( LSPClientPlugin, ( pluginManager ) );
}

LSPClientPlugin::LSPClientPlugin( const PluginManager* pluginManager ) :
	mManager( pluginManager ), mThreadPool( pluginManager->getThreadPool() ) {
	mThreadPool->run( [&, pluginManager] { load( pluginManager ); }, [] {} );
}

LSPClientPlugin::~LSPClientPlugin() {
	mClosing = true;
	Lock l( mDocMutex );
	for ( const auto &editor : mEditors ) {
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

void LSPClientPlugin::processNotification( PluginManager::Notification noti,
										   const nlohmann::json& obj ) {
	switch ( noti ) {
		case PluginManager::WorkspaceFolderChanged: {
			mClientManager.didChangeWorkspaceFolders( obj["folder"] );
			break;
		}
		default:
			break;
	}
}

void LSPClientPlugin::load( const PluginManager* pluginManager ) {
	pluginManager->subscribeNotifications(
		this, [&]( auto noti, auto obj ) { processNotification( noti, obj ); } );
	std::vector<std::string> paths;
	std::string path( pluginManager->getResourcesPath() + "plugins/lspclient.json" );
	if ( FileSystem::fileExists( path ) )
		paths.emplace_back( path );
	path = pluginManager->getPluginsPath() + "lspclient.json";
	if ( FileSystem::fileExists( path ) ||
		 FileSystem::fileWrite(
			 path, "{\n  \"config\":{},\n  \"keybindings\":{},\n  \"formatters\":[]\n}\n" ) ) {
		mConfigPath = path;
		paths.emplace_back( path );
	}
	if ( paths.empty() )
		return;

	std::vector<LSPDefinition> lsps;

	for ( const auto& path : paths ) {
		try {
			loadLSPConfig( lsps, path );
		} catch ( const json::exception& e ) {
			Log::error( "Parsing LSP \"%s\" failed:\n%s", path.c_str(), e.what() );
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
}

void LSPClientPlugin::loadLSPConfig( std::vector<LSPDefinition>& lsps, const std::string& path ) {
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;
	json j;
	try {
		j = json::parse( data, nullptr, true, true );
	} catch ( const json::exception& e ) {
		Log::error( "LSPClientPlugin::loadLSPConfig - Error parsing LSP config from "
					"path %s, error: ",
					path.c_str(), e.what() );
		return;
	}

	if ( j.contains( "config" ) ) {
		auto& config = j["config"];
		if ( config.contains( "hover_delay" ) )
			setHoverDelay( Time::fromString( config["hover_delay"].get<std::string>() ) );
	}

	if ( mKeyBindings.empty() )
		mKeyBindings["lsp-go-to-definition"] = "f2";

	if ( j.contains( "keybindings" ) ) {
		auto kb = j["keybindings"];
		auto bindKey = [this, kb]( const std::string& key ) {
			if ( kb.contains( key ) )
				mKeyBindings[key] = kb[key];
		};
		auto list = { "lsp-go-to-definition", "lsp-go-to-declaration", "lsp-go-to-implementation",
					  "lsp-go-to-type-definition", "lsp-switch-header-source" };
		for ( const auto& key : list )
			bindKey( key );
	}

	if ( !j.contains( "servers" ) )
		return;
	auto& servers = j["servers"];

	for ( auto& obj : servers ) {
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
					lsp.command = tlsp.command;
					lsp.name = tlsp.name;
					lsp.rootIndicationFileNames = tlsp.rootIndicationFileNames;
					lsp.url = tlsp.url;
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
			lsp.command = obj["command"];
			lsp.name = obj["name"];
		}

		if ( obj.contains( "url" ) )
			lsp.url = obj["url"];

		auto fp = obj["file_patterns"];

		for ( auto& pattern : fp )
			lsp.filePatterns.push_back( pattern.get<std::string>() );

		if ( obj.contains( "rootIndicationFileNames" ) ) {
			lsp.rootIndicationFileNames.clear();
			auto fnms = obj["rootIndicationFileNames"];
			for ( auto& fn : fnms )
				lsp.rootIndicationFileNames.push_back( fn );
		}

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

void LSPClientPlugin::onRegister( UICodeEditor* editor ) {
	Lock l( mDocMutex );
	mDocs.insert( editor->getDocumentRef().get() );

	for ( auto& kb : mKeyBindings ) {
		editor->getKeyBindings().addKeybindString( kb.second, kb.first );
	}

	if ( editor->hasDocument() ) {
		editor->getDocument().setCommand( "lsp-go-to-definition", [&, editor]() {
			mClientManager.getAndGoToLocation( editor->getDocumentRef(),
											   "textDocument/definition" );
		} );

		editor->getDocument().setCommand( "lsp-go-to-declaration", [&, editor]() {
			mClientManager.getAndGoToLocation( editor->getDocumentRef(),
											   "textDocument/declaration" );
		} );

		editor->getDocument().setCommand( "lsp-go-to-implementation", [&, editor]() {
			mClientManager.getAndGoToLocation( editor->getDocumentRef(),
											   "textDocument/implementation" );
		} );

		editor->getDocument().setCommand( "lsp-go-to-type-definition", [&, editor]() {
			mClientManager.getAndGoToLocation( editor->getDocumentRef(),
											   "textDocument/typeDefinition" );
		} );

		editor->getDocument().setCommand( "lsp-switch-header-source", [&, editor]() {
			auto* server = mClientManager.getOneLSPClientServer( editor );
			if ( server )
				server->switchSourceHeader( editor->getDocument().getURI() );
		} );
	}

	std::vector<Uint32> listeners;

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentLoaded, [&, editor]( const Event* ) {
			mClientManager.run( editor->getDocumentRef() );
		} ) );

	mEditors.insert( { editor, listeners } );
	mEditorsTags.insert( { editor, {} } );
	mEditorDocs[editor] = editor->getDocumentRef().get();

	if ( mReady && editor->hasDocument() && editor->getDocument().hasFilepath() )
		mClientManager.run( editor->getDocumentRef() );
	if ( !mReady )
		mDelayedDocs[&editor->getDocument()] = editor->getDocumentRef();
}

void LSPClientPlugin::onUnregister( UICodeEditor* editor ) {
	for ( auto& kb : mKeyBindings ) {
		editor->getKeyBindings().removeCommandKeybind( kb.first );
		if ( editor->hasDocument() )
			editor->getDocument().removeCommand( kb.first );
	}

	if ( mClosing )
		return;
	Lock l( mDocMutex );
	TextDocument* doc = mEditorDocs[editor];
	auto cbs = mEditors[editor];
	for ( auto listener : cbs )
		editor->removeEventListener( listener );
	mEditors.erase( editor );
	mEditorsTags.erase( editor );
	mEditorDocs.erase( editor );
	for ( auto editor : mEditorDocs )
		if ( editor.second == doc )
			return;
	mDocs.erase( doc );
}

const PluginManager* LSPClientPlugin::getManager() const {
	return mManager;
}

bool LSPClientPlugin::onCreateContextMenu( UICodeEditor* editor, UIPopUpMenu* menu,
										   const Vector2i& /*position*/, const Uint32& /*flags*/ ) {
	auto* server = mClientManager.getOneLSPClientServer( editor );
	if ( !server )
		return false;

	menu->addSeparator();

	auto addFn = [this, editor, menu]( const std::string& txtKey, const std::string& txtVal ) {
		menu->add( editor->getUISceneNode()->i18n( txtKey, txtVal ), nullptr,
				   KeyBindings::keybindFormat( mKeyBindings[txtKey] ) )
			->setId( txtKey );
	};
	auto cap = server->getCapabilities();

	if ( cap.definitionProvider )
		addFn( "lsp-go-to-definition", "Go To Definition" );

	if ( cap.declarationProvider )
		addFn( "lsp-go-to-declaration", "Go To Declaration" );

	if ( cap.typeDefinitionProvider )
		addFn( "lsp-go-to-type-definition", "Go To Type Definition" );

	if ( cap.implementationProvider )
		addFn( "lsp-go-to-implementation", "Go To Implementation" );

	if ( server->getDefinition().language == "cpp" || server->getDefinition().language == "c" )
		addFn( "lsp-switch-header-source", "Switch Header/Source" );

	return false;
}

static void hideTooltip( UICodeEditor* editor ) {
	if ( editor->getTooltip() && editor->getTooltip()->isVisible() ) {
		editor->setTooltipText( "" );
		editor->getTooltip()->hide();
	}
}

TextPosition currentMouseTextPosition( UICodeEditor* editor ) {
	return editor->resolveScreenPosition(
		editor->getUISceneNode()->getWindow()->getInput()->getMousePosf() );
}

bool LSPClientPlugin::onMouseMove( UICodeEditor* editor, const Vector2i& position, const Uint32& ) {
	String::HashType tag = String::hash( editor->getDocument().getFilePath() );
	editor->removeActionsByTag( tag );
	mEditorsTags[editor].insert( tag );
	editor->runOnMainThread(
		[&, editor, position, tag]() {
			mEditorsTags[editor].erase( tag );
			mThreadPool->run( [&, editor, position]() {
				auto server = mClientManager.getOneLSPClientServer( editor );
				if ( server == nullptr )
					return;
				server->documentHover(
					editor->getDocument().getURI(), currentMouseTextPosition( editor ),
					[&, editor, position]( const LSPHover& resp ) {
						if ( resp.range.isValid() && !resp.contents.empty() ) {
							editor->runOnMainThread( [editor, resp, position, this]() {
								TextPosition startCursorPosition =
									editor->resolveScreenPosition( position.asFloat() );
								TextPosition currentMousePosition =
									currentMouseTextPosition( editor );
								if ( startCursorPosition != currentMousePosition )
									return;
								if ( resp.range.isValid() && !resp.contents.empty() &&
									 resp.range.contains( startCursorPosition ) ) {
									mCurrentHover = resp;
									editor->setTooltipText( resp.contents[0].value );
									editor->getTooltip()->setHorizontalAlign( UI_HALIGN_LEFT );
									editor->getTooltip()->setPixelsPosition( position.asFloat() );
									editor->getTooltip()->setDontAutoHideOnMouseMove( true );
									if ( editor->hasFocus() && !editor->getTooltip()->isVisible() )
										editor->getTooltip()->show();
								}
							} );
						}
					} );
			} );
		},
		mHoverDelay, tag );
	TextPosition cursorPosition = editor->resolveScreenPosition( position.asFloat() );
	if ( !mCurrentHover.range.isValid() || !mCurrentHover.range.contains( cursorPosition ) )
		hideTooltip( editor );
	return mCurrentHover.range.isValid() && editor->getTooltip() &&
		   editor->getTooltip()->isVisible();
}

void LSPClientPlugin::onFocusLoss( UICodeEditor* editor ) {
	hideTooltip( editor );
}

bool LSPClientPlugin::onKeyDown( UICodeEditor* editor, const KeyEvent& event ) {
	if ( event.getSanitizedMod() == 0 && event.getKeyCode() == KEY_ESCAPE && editor->getTooltip() &&
		 editor->getTooltip()->isVisible() ) {
		editor->getTooltip()->hide();
	}

	return false;
}

const Time& LSPClientPlugin::getHoverDelay() const {
	return mHoverDelay;
}

void LSPClientPlugin::setHoverDelay( const Time& hoverDelay ) {
	mHoverDelay = hoverDelay;
}

} // namespace ecode
