#include "discordRPCplugin.hpp"

using json = nlohmann::json;
#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN || defined( __EMSCRIPTEN_PTHREADS__ )
#define dcRPC_THREADED 1
#else
#define dcRPC_THREADED 0
#endif

namespace ecode {

static const auto DEFAULT_CLIENT_ID = "1339026777158455336";
static const auto DebounceUniqueId = String::hash( "DiscordRPCplugin::debounce" );
static const std::vector<std::string> DiscordRPCCommandList = { "discordrpc-reconnect" };

static const auto DISCORDRPC_DEFAULT_ICON =
	"https://github.com/SpartanJ/eepp/blob/develop/bin/assets/icon/ecode.png?raw=true";

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
	if ( mIPC.isConnected() )
		mIPC.clearActivity();
	mShuttingDown = true;

	for ( auto editor : mEditors ) {
		onBeforeUnregister( editor.first );
		onUnregisterEditor( editor.first );
	}
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

	if ( paths.empty() )
		return;

	for ( const auto& ipath : paths ) {
		try {
			loadDiscordRPCConfig( ipath, mConfigPath == ipath );
		} catch ( const json::exception& e ) {
			Log::error( "Parsing DiscordRPCplugin config \"%s\" failed:\n%s", ipath.c_str(),
						e.what() );
		}
	}

	if ( getUISceneNode() ) {
		mIPC.UIReady = true;
		initIPC();
	} else {
		mIPC.IsReconnectScheduled = true;
	}

	mReady = true;
	fireReadyCbs();
	setReady( clock.getElapsedTime() );
}

void DiscordRPCplugin::loadDiscordRPCConfig( const std::string& path, bool updateConfigFile ) {
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;

	if ( updateConfigFile )
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

	if ( j.contains( "config" ) ) {
		auto& config = j["config"];

		// Icon bindings are being provided thanks to vyfor icons: https://github.com/vyfor/icons
		// Icon licenses at: https://github.com/vyfor/icons/blob/master/LICENSE.md
		if ( config.contains( "iconBindings" ) ) {
			auto newConfig = config["iconBindings"];
			if ( mLangBindings.empty() )
				mLangBindings = std::move( newConfig );
			else if ( !newConfig.empty() )
				mLangBindings.update( newConfig );
		} else
			config["iconBindings"] = mLangBindings;

		if ( config.contains( "appID" ) )
			mIPC.ClientID = config.value( "appID", DEFAULT_CLIENT_ID );
		else
			config["appID"] = DEFAULT_CLIENT_ID;

		if ( config.contains( "doLanguageIcons" ) )
			mDoLangIcon = config.value( "doLanguageIcons", true );
		else
			config["doLanguageIcons"] = true;

		if ( config.contains( "doGitIntegration" ) )
			mDoGitIntegration = config.value( "doGitIntegration", true );
		else
			config["doGitIntegration"] = true;

		if ( updateConfigFile && config.contains( "iconBindings" ) &&
			 config["iconBindings"].is_null() )
			config["iconBindings"] = nlohmann::json::object();
	}

	if ( updateConfigFile ) {
		std::string newData = j.dump( 2 );
		if ( newData != data ) {
			FileSystem::fileWrite( path, newData );
			mConfigHash = String::hash( newData );
		}
	}
}

void DiscordRPCplugin::initIPC() {
	if ( mIPC.tryConnect() ) {
		DiscordIPCActivity a = mIPC.getActivity();
		a.largeImage = DISCORDRPC_DEFAULT_ICON;
		mIPC.setActivity( std::move( a ) );
	}
}

PluginRequestHandle DiscordRPCplugin::processMessage( const PluginMessage& msg ) {
	switch ( msg.type ) {
		case PluginMessageType::WorkspaceFolderChanged: {
			std::string rpath = FileSystem::getRealPath( msg.asJSON()["folder"] );
			FileSystem::dirAddSlashAtEnd( rpath );
			mProjectName = FileSystem::fileNameFromPath( rpath );
			mProjectPath = std::move( rpath );
			Log::debug( "Loaded new workspace: %s ; %s", rpath, mProjectName );
		}
		case PluginMessageType::UIReady: {
			mIPC.UIReady = true;

			if ( mIPC.IsReconnectScheduled ) {
				Log::debug( "Running scheduled reconnect" );

				getUISceneNode()->getThreadPool()->run( [this] { initIPC(); } );
			}
		}
		default:
			break;
	}

	return PluginRequestHandle::empty();
}

void DiscordRPCplugin::onRegisterEditor( UICodeEditor* editor ) {
	editor->addUnlockedCommands( DiscordRPCCommandList );
	editor->getDocument().setCommand( "discordrpc-reconnect", [this] { mIPC.reconnect(); } );

	PluginBase::onRegisterEditor( editor );
}

void DiscordRPCplugin::onUnregisterEditor( UICodeEditor* editor ) {
	editor->removeUnlockedCommands( DiscordRPCCommandList );
	editor->removeActionsByTag( DebounceUniqueId );
}

void DiscordRPCplugin::updateActivity( DiscordIPCActivity& a ) {
	Lock l( mDataMutex );
	Log::debug( "dcIPC: Activity in new file. lang = %s", mLastLang );

	if ( !mProjectName.empty() ) {
		a.details =
			String::format( i18n( "dc_workspace", "Working on %s" ).toUtf8(), mProjectName );

		if ( mDoGitIntegration ) {
			std::string url = getGitOriginUrl();

			if ( !url.empty() ) {
				if ( String::startsWith( url, "http" ) ) {
					a.buttons[0].url = String::rTrim( url, ".git" );
				} else {
					RegEx regex( R"(@(.*)\.git)" );
					PatternMatcher::Range matches[1];
					regex.matches( url, matches );

					if ( matches[0].isValid() ) {
						std::string giturl =
							url.substr( matches[0].start + 1, matches[0].end - matches[0].start );
						String::replaceAll( giturl, ":", "/" );

						// Most services/browsers automatically switch to TLS when available but not
						// the other way. Thus we use http://
						a.buttons[0].url = "http://" + String::rTrim( giturl, ".git" );
					}
				}
				a.buttons[0].label = "Repository";
			}
		}
	}

	a.state = String::format( i18n( "dc_editing", "Editing %s, a %s file" ).toUtf8(), mLastFile,
							  mLastLang );

	a.start = time( nullptr ); // Time spent in this specific file

	if ( !mLastLangName.empty() && mDoLangIcon && mLangBindings.contains( mLastLangName ) ) {
		a.largeImage = mLangBindings.value( mLastLangName, DISCORDRPC_DEFAULT_ICON );
	}
}

std::string DiscordRPCplugin::getGitOriginUrl() {
	IniFile Ini( mProjectPath + ".git/config" );
	return Ini.getValue( "remote \"origin\"", "url", "" );
}

void DiscordRPCplugin::onRegisterListeners( UICodeEditor* editor, std::vector<Uint32>& listeners ) {
	listeners.push_back( editor->on( Event::OnFocus, [this, editor]( const Event* ) {
		auto& doc = editor->getDocument();
		if ( !doc.hasFilepath() )
			return;

		auto filename = doc.getFilename();
		if ( filename == mLastFile )
			return;

		{
			Lock l( mDataMutex );
			mLastFile = std::move( filename );
			mLastLang = doc.getSyntaxDefinition().getLanguageName();
			mLastLangName = doc.getSyntaxDefinition().getLSPName();
		}

		getUISceneNode()->debounce(
			[this] {
				getUISceneNode()->getThreadPool()->run( [this] {
					DiscordIPCActivity a = mIPC.getActivity();
					updateActivity( a );
					mIPC.setActivity( std::move( a ) );
				} );
			},
			Seconds( 2 ), DebounceUniqueId );
	} ) );
}

} // namespace ecode
