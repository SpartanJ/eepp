#include "debuggerplugin.hpp"
#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>
#include <nlohmann/json.hpp>

using namespace EE::UI;
using namespace EE::UI::Doc;

using namespace std::literals;

using json = nlohmann::json;

#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
#define DEBUGGER_THREADED 1
#else
#define DEBUGGER_THREADED 0
#endif

namespace ecode {

Plugin* DebuggerPlugin::New( PluginManager* pluginManager ) {
	return eeNew( DebuggerPlugin, ( pluginManager, false ) );
}

Plugin* DebuggerPlugin::NewSync( PluginManager* pluginManager ) {
	return eeNew( DebuggerPlugin, ( pluginManager, true ) );
}

DebuggerPlugin::DebuggerPlugin( PluginManager* pluginManager, bool sync ) :
	PluginBase( pluginManager ) {
	if ( sync ) {
		load( pluginManager );
	} else {
#if defined( DEBUGGER_THREADED ) && DEBUGGER_THREADED == 1
		mThreadPool->run( [this, pluginManager] { load( pluginManager ); } );
#else
		load( pluginManager );
#endif
	}
}

DebuggerPlugin::~DebuggerPlugin() {
	waitUntilLoaded();
	mShuttingDown = true;
}

void DebuggerPlugin::load( PluginManager* pluginManager ) {
	Clock clock;
	AtomicBoolScopedOp loading( mLoading, true );
	pluginManager->subscribeMessages( this,
									  [this]( const auto& notification ) -> PluginRequestHandle {
										  return processMessage( notification );
									  } );

	std::string path = pluginManager->getPluginsPath() + "debugger.json";
	if ( FileSystem::fileExists( path ) ||
		 FileSystem::fileWrite( path, "{\n  \"config\":{},\n  \"keybindings\":{}\n}\n" ) ) {
		mConfigPath = path;
	}
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;
	mConfigHash = String::hash( data );

	json j;
	try {
		j = json::parse( data, nullptr, true, true );
	} catch ( const json::exception& e ) {
		Log::error( "DebuggerPlugin::load - Error parsing config from path %s, error: %s, config "
					"file content:\n%s",
					path.c_str(), e.what(), data.c_str() );
		// Recreate it
		j = json::parse( "{\n  \"config\":{},\n  \"keybindings\":{},\n}\n", nullptr, true, true );
	}

	bool updateConfigFile = false;

	if ( j.contains( "config" ) ) {
	}

	if ( mKeyBindings.empty() ) {
	}

	if ( j.contains( "keybindings" ) ) {
		auto& kb = j["keybindings"];
		std::initializer_list<std::string> list = {};
		for ( const auto& key : list ) {
			if ( kb.contains( key ) ) {
				if ( !kb[key].empty() )
					mKeyBindings[key] = kb[key];
			} else {
				kb[key] = mKeyBindings[key];
				updateConfigFile = true;
			}
		}
	}

	if ( updateConfigFile ) {
		std::string newData = j.dump( 2 );
		if ( newData != data ) {
			FileSystem::fileWrite( path, newData );
			mConfigHash = String::hash( newData );
		}
	}

	if ( getUISceneNode() ) {
		// Init UI
	}

	subscribeFileSystemListener();
	mReady = true;
	fireReadyCbs();
	setReady( clock.getElapsedTime() );
}

PluginRequestHandle DebuggerPlugin::processMessage( const PluginMessage& msg ) {
	switch ( msg.type ) {
		case PluginMessageType::WorkspaceFolderChanged: {

			break;
		}
		case ecode::PluginMessageType::UIReady: {
			// Init UI if not initialized
			break;
		}
		default:
			break;
	}
	return PluginRequestHandle::empty();
}

} // namespace ecode
