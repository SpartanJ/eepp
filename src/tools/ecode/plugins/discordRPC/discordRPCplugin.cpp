#include "discordRPCplugin.hpp"
#include <nlohmann/json.hpp>
#include <eepp/system/filesystem.hpp>

using json = nlohmann::json;

namespace ecode {

Plugin* DiscordRPCplugin::New( PluginManager* pluginManager) {
	return eeNew( DiscordRPCplugin, ( pluginManager, false ) );
}

Plugin* DiscordRPCplugin::NewSync( PluginManager* pluginManager) {
	return eeNew( DiscordRPCplugin, ( pluginManager, true ) );
}

DiscordRPCplugin::DiscordRPCplugin( PluginManager* pluginManager, bool sync) : Plugin( pluginManager ) {
	//if ( sync ) { // Does not need to be multithreaded
	load( pluginManager );
	//} 
}

DiscordRPCplugin::~DiscordRPCplugin() {
	waitUntilLoaded();
	mShuttingDown = true;
	mManager->unsubscribeMessages( this );
	unsubscribeFileSystemListener();
	
	for ( const auto& editor : mEditors ) {
		for ( auto& kb : mKeyBindings ) {
			editor.first->getKeyBindings().removeCommandKeybind( kb.first );
			if ( editor.first->hasDocument() )
				editor.first->getDocument().removeCommand( kb.first );
		}
		for ( auto listener : editor.second )
			editor.first->removeEventListener( listener );
		editor.first->unregisterPlugin( this );
	}
}

void DiscordRPCplugin::load( PluginManager* pluginManager ) {
	Clock clock;
	//mClock = Clock();
	AtomicBoolScopedOp loading( mLoading, true );
	pluginManager->subscribeMessages( this,
									  [this]( const auto& notification ) -> PluginRequestHandle {
										  return processMessage( notification );
									  } );
									  
	std::vector<std::string> paths;
	std::string path( pluginManager->getResourcesPath() + "plugins/discordRPC.json" );
	if ( FileSystem::fileExists( path ) )
		paths.emplace_back( path );
	path = pluginManager->getPluginsPath() + "discordRPC.json";
	if ( FileSystem::fileExists( path ) ||
		 FileSystem::fileWrite(path, "{\n\"config\":{},\n\"keybindings\":{}\n}\n") ) {
	   mConfigPath = path;
	   paths.emplace_back( path );	 	
    }
    if (paths.empty() )
    	return;
    for ( const auto& tpath : paths ) {
		try {
			loadConfig( tpath, mConfigPath == tpath );
		} catch ( const json::exception& e ) {
			Log::error( "DiscordRPCplugin::load - Parsing config \"%s\" failed:\n%s", tpath.c_str(), e.what() );
		}
	}
	
	mReady = true;
	fireReadyCbs();
	setReady( clock.getElapsedTime() );
}

void DiscordRPCplugin::loadConfig( const std::string& path, bool updateConfigFile ) {
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;
	json j;
	try {
		j = json::parse( data, nullptr, true, true);
	} catch ( const json::exception& e) {
		Log::error( "DiscordRPCplugin::loadConfig - Error parsing config from",
					"path %s, error: %s, config file content:\n%s",
					path.c_str(), e.what(), data.c_str());
		if ( !updateConfigFile )
			return;
		// TODO: Create a default config
		// See: LinterPlugin:L100
	}
	
	if ( updateConfigFile ) {
		mConfigHash = String::hash( data );
	}
	
	if (j.contains( "config" ) ) {
		auto& config = j["config"];
		// TODO: Figure out config options
	}
	
	if ( mKeyBindings.empty() ) {
		// mKeyBindings["command-name"] = "mod+x"; // Syntax example
		
	}
	
	auto& kb = j["keybindings"];
	auto list = {"command-name"}; // { "command-name", "command2-name" }
	for (const auto& key : list ) {
		if ( kb.contains( key ) ) {
			if ( !kb[key].empty() )
				mKeyBindings[key] = kb[key];
		} else if ( updateConfigFile )
			kb[key] = mKeyBindings[key];
	}
}

void DiscordRPCplugin::onRegister( UICodeEditor* editor ) {
	Lock l( mDocMutex );
	std::vector<Uint32> listeners;
	
	// Use this pattern to register your event listeners
	// TODO: Add a link to where all the events are defined
// 	listeners.push_back(
// 		editor->addEventListener( Event::OnX, [this]( const Event* event ) {
// 		
// 		}
// 	)

	if ( editor->hasDocument() ) {
		auto& doc = editor->getDocument();
		
// 		doc.setCommand( "command-name",  [this]( TextDocument::Client* client ) {
// 			// Action behavior
// 		}
	}
	
	mEditors.insert( { editor, listeners } );
	mDocs.insert( editor->getDocumentRef().get() );
	mEditorDocs[editor] = editor->getDocumentRef().get();
}

void DiscordRPCplugin::onUnregister( UICodeEditor* editor ) {
	if ( mShuttingDown )
		return;
		
	Lock l( mDocMutex );
	TextDocument* doc = mEditorDocs[editor];
	auto cbs = mEditors[editor];
	for ( auto listener : cbs )
		editor->removeEventListener( listener );
	mEditors.erase( editor );
	mEditorDocs.erase( editor );
	for ( auto editorIt : mEditorDocs )
		if ( editorIt.second == doc )
			return;
	
	for ( auto& kb : mKeyBindings ) {
		editor->getKeyBindings().removeCommandKeybind( kb.first );
		if ( editor->hasDocument() )
			editor->getDocument().removeCommand( kb.first );
	}

	mDocs.erase( doc );
	mDirtyDoc.erase( doc );
}

void DiscordRPCplugin::update( UICodeEditor* editor ) {
    if (mClock.getElapsedTime().asMilliseconds() >= 500) {
        
        mClock.restart();
    }
}

PluginRequestHandle DiscordRPCplugin::processMessage( const PluginMessage& notification ) {
	return {};
}

} // namespace ecode