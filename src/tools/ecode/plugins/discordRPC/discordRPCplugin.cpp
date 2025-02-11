#include "discordRPCplugin.hpp"

using json = nlohmann::json;
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
#define dcRPC_THREADED 1
#else
#define dcRPC_THREADED 0
#endif

namespace ecode {

Plugin* DiscordRPCplugin::New( PluginManager* pluginManager ) {
	return eeNew( DiscordRPCplugin, ( pluginManager, false ) );
}

Plugin* DiscordRPCplugin::NewSync( PluginManager* pluginManager ) {
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
		 FileSystem::fileWrite( path, "{\n  \"config\":{},\n  \"keybindings\":{}\n}\n" ) ) {
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
		mcLangBindings = config["iconBindings"];
		if ( config.contains( "appID" ) ) {
			mIPC.ClientID = config.value( "appID", "1335730393948749898" );
		} else {
			mIPC.ClientID = "1335730393948749898";
			updateConfigFile = true;
		}
		if ( config.contains( "doLanguageIcons" ) ) {
			mcDoLangIcon = config.value( "doLanguageIcons", true );
		} else {
			mIPC.ClientID = true;
			updateConfigFile = true;
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
		mIPC.UIReady = true;
		if ( mIPC.tryConnect() ) {
			DiscordIPCActivity a = mIPC.getActivity();
			a.largeImage = DISCORDRPC_DEFAULT_ICON;
			mIPC.setActivity( std::move( a ) );
		}
	}

	mReady = true;
	fireReadyCbs();
	setReady( clock.getElapsedTime() );
}

PluginRequestHandle DiscordRPCplugin::processMessage( const PluginMessage& msg ) {
	switch ( msg.type ) {
		case PluginMessageType::WorkspaceFolderChanged: {
			std::string rpath = FileSystem::getRealPath( msg.asJSON()["folder"] );
			FileSystem::dirAddSlashAtEnd( rpath );
			mProjectName = FileSystem::fileNameFromPath( rpath );
			Log::debug( "Loaded new workspace: %s ; %s", rpath, mProjectName );
		}
		case PluginMessageType::UIReady: {
			mIPC.UIReady = true;

			if ( mIPC.IsReconnectScheduled ) {
				Log::debug( "Running scheduled reconnect" );
				mIPC.tryConnect();
			}
		}
		default:
			break;
	}

	return PluginRequestHandle::empty();
}

void DiscordRPCplugin::onRegisterEditor( UICodeEditor* editor ) {
	editor->addUnlockedCommands( { "discordrpc-reconnect" } );
	editor->getDocument().setCommand( "discordrpc-reconnect", [this] { mIPC.reconnect(); } );

	PluginBase::onRegisterEditor( editor );
}

void DiscordRPCplugin::onUnregisterEditor( UICodeEditor* editor ) {
	editor->removeUnlockedCommands( { "discordrpc-reconnect" } );
}

void DiscordRPCplugin::onRegisterListeners( UICodeEditor* editor, std::vector<Uint32>& listeners ) {
	listeners.push_back( editor->on( Event::OnFocus, [this, editor]( const Event* ) {
		// `this` in the scope of the lambda is the parent `DiscordRPCplugin`

		auto& doc = editor->getDocument();
		if ( !doc.hasFilepath() ) {
			return;
		}

		auto filename = doc.getFilename();

		if ( filename != mLastFile ) {
			this->mLastFile = filename;

			Log::debug( "dcIPC: Activity in new file. lang = %s",
						doc.getSyntaxDefinition().getLanguageName() );

			DiscordIPCActivity a = mIPC.getActivity();

			if ( !mProjectName.empty() )
				a.details = String::format( i18n( "dc_workspace", "Working on %s" ).toUtf8(),
											mProjectName );
			a.state = String::format( i18n( "dc_editing", "Editing %s, a %s file" ).toUtf8(),
									  filename, doc.getSyntaxDefinition().getLanguageName() );
			a.start = time( nullptr ); // Time spent in this specific file

			// TODO: Implement github/gitlab remote button (integrate with git plugin)
			// a.buttons[0].label = "Repository";
			// a.buttons[0].url = "https://github.com/name/repo";

			std::string name = doc.getSyntaxDefinition().getLSPName();
			if ( !name.empty() && mcDoLangIcon && mcLangBindings.contains( name ) ) {
				a.largeImage = mcLangBindings.value( name, DISCORDRPC_DEFAULT_ICON );
			}

			mIPC.setActivity( std::move( a ) );
		}
	} ) );
}

} // namespace ecode
