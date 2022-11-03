#include "lspclientplugin.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace ecode {

UICodeEditorPlugin* LSPClientPlugin::New( const PluginManager* pluginManager ) {
	return eeNew( LSPClientPlugin, ( pluginManager ) );
}

LSPClientPlugin::LSPClientPlugin( const PluginManager* pluginManager ) :
	mManager( pluginManager ), mPool( pluginManager->getThreadPool() ) {
	mPool->run( [&, pluginManager] { load( pluginManager ); }, [] {} );
}

LSPClientPlugin::~LSPClientPlugin() {
	mClosing = true;
	Lock l( mDocMutex );
	for ( auto editor : mEditors ) {
		for ( auto& kb : mKeyBindings ) {
			editor.first->getKeyBindings().removeCommandKeybind( kb.first );
			if ( editor.first->hasDocument() )
				editor.first->getDocument().removeCommand( kb.first );
		}
		editor.first->unregisterPlugin( this );
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
		 FileSystem::fileWrite( path, "{\n\"config\":{},\n\"servers\":[]\n}\n" ) ) {
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

	mReady = mClientManager.clientCount() > 0;
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
	} catch ( ... ) {
		return;
	}

	if ( mKeyBindings.empty() )
		mKeyBindings["lsp-go-to-definition"] = "f2";
	if ( j.contains( "keybindings" ) && j["keybindings"].contains( "lsp-go-to-definition" ) )
		mKeyBindings["lsp-go-to-definition"] = j["keybindings"]["lsp-go-to-definition"];

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
			mClientManager.goToDocumentDefinition( editor->getDocumentRef().get() );
		} );
	}

	std::vector<Uint32> listeners;

	listeners.push_back(
		editor->addEventListener( Event::OnDocumentLoaded, [&, editor]( const Event* ) {
			mClientManager.run( editor->getDocumentRef() );
		} ) );

	mEditors.insert( { editor, listeners } );
	mEditorDocs[editor] = editor->getDocumentRef().get();

	if ( editor->hasDocument() && editor->getDocument().hasFilepath() )
		mClientManager.run( editor->getDocumentRef() );
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

	auto addFn = [editor, server, menu]( const std::string& txtKey, const std::string& txtVal,
										 const std::string& cmd ) {
		menu->add( editor->getUISceneNode()->i18n( txtKey, txtVal ) )
			->setId( txtKey )
			->on( Event::OnItemClicked, [server, editor, cmd]( const Event* event ) {
				if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
					return;
				UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
				if ( String::startsWith( item->getId(), "lsp" ) ) {
					server->getAndGoToLocation( editor->getDocument().getURI(),
												editor->getDocument().getSelection().start(), cmd );
				}
			} );
	};
	auto cap = server->getCapabilities();

	if ( cap.definitionProvider )
		addFn( "lsp-go-to-definition", "Go To Definition", "textDocument/definition" );

	if ( cap.declarationProvider )
		addFn( "lsp-go-to-declaration", "Go To Declaration", "textDocument/declaration" );

	if ( cap.typeDefinitionProvider )
		addFn( "lsp-go-to-type-definition", "Go To Type Definition",
			   "textDocument/typeDefinition" );

	if ( cap.implementationProvider )
		addFn( "lsp-go-to-implementation", "Go To Implementation", "textDocument/implementation" );

	return false;
}

} // namespace ecode
