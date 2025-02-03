#include "discordRPCplugin.hpp"

#include "sdk/ipc.hpp"

using json = nlohmann::json;
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined ( __EMSCRIPTEN_PTHREADS__ )
#define dcRPC_THREADED 1
#else
#define dcRPC_THREADED 0
#endif

namespace ecode {

Plugin* DiscordRPCplugin::New( PluginManager* pluginManager) {
	return eeNew( DiscordRPCplugin, ( pluginManager, false ) );
}

Plugin* DiscordRPCplugin::NewSync( PluginManager* pluginManager) {
	return eeNew( DiscordRPCplugin, ( pluginManager, true ) );
}

DiscordRPCplugin::DiscordRPCplugin( PluginManager* pluginManager, bool sync ) :
	PluginBase( pluginManager ) {
	if ( sync ) {
		load( pluginManager );
	} else {
#if defined( dcRPC_THREADED ) && dcRPC_THREADED == 1
		mThreadPool->run( [this, pluginManager] { load( pluginManager ); } );
#else
		load( pluginManager );
#endif
	}
}

DiscordRPCplugin::~DiscordRPCplugin() {
	waitUntilLoaded();
	mShuttingDown = true;

}
void DiscordRPCplugin::load( PluginManager* pluginManager ) {
	Clock clock;
	AtomicBoolScopedOp loading( mLoading, true );
									  
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
    std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;
	mConfigHash = String::hash( data );

	json j;
	try {
		j = json::parse( data, nullptr, true, true );
	} catch ( const json::exception& e ) {
		Log::error( "DiscordRPCplugin::load - Error parsing config from path %s, error: %s, config "
					"file content:\n%s",
					path.c_str(), e.what(), data.c_str() );
		// Recreate it
		j = json::parse( "{\n  \"config\":{},\n  \"keybindings\":{},\n}\n", nullptr, true, true );
	}

	bool updateConfigFile = false;

	if ( j.contains( "config" ) ) {
		auto& config = j["config"];
		
	}

	if ( updateConfigFile ) {
		std::string newData = j.dump( 2 );
		if ( newData != data ) {
			FileSystem::fileWrite( path, newData );
			mConfigHash = String::hash( newData );
		}
	}
	
	DiscordIPC ipc;
	ipc.tryConnect();
	
	mReady = true;
	fireReadyCbs();
	setReady( clock.getElapsedTime() );
}

} // namespace ecode