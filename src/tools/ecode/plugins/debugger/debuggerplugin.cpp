#include "debuggerplugin.hpp"
#include "../../jsonhelper.hpp"
#include "../../notificationcenter.hpp"
#include "../../terminalmanager.hpp"
#include "../../uistatusbar.hpp"
#include "../../widgetcommandexecuter.hpp"
#include "busprocess.hpp"
#include "bussocket.hpp"
#include "bussocketprocess.hpp"
#include "dap/debuggerclientdap.hpp"
#include "models/breakpointsmodel.hpp"
#include "models/processesmodel.hpp"
#include "models/variablesmodel.hpp"
#include "statusdebuggercontroller.hpp"
#include <eepp/graphics/primitives.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uitooltip.hpp>

using namespace EE::UI;
using namespace EE::UI::Doc;

using namespace std::literals;

using json = nlohmann::json;

static constexpr auto REQUEST_TYPE_LAUNCH = "launch";
static constexpr auto REQUEST_TYPE_ATTACH = "attach";

namespace ecode {

static constexpr auto INPUT_PATTERN = "%$%{input%:([%w_]+)%}"sv;
static constexpr auto ENV_PATTERN = "%$%{env%:([%w_]+)%}"sv;
static constexpr auto COMMAND_PATTERN = "%$%{command%:([%w_]+)%}"sv;
static constexpr auto CMD_PICK_PROCESS = "${command:pickProcess}"sv;
static constexpr auto CMD_PROMPT_STRING = "${command:promptString}"sv;
static constexpr auto CMD_PICK_FILE = "${command:pickFile}"sv;

static LuaPattern inputPtrn( INPUT_PATTERN );
static LuaPattern commandPtrn( COMMAND_PATTERN );
static LuaPattern envPtrn( ENV_PATTERN );
static constexpr auto KEY_FILE = "${file}";
static constexpr auto KEY_ARGS = "${args}";
static constexpr auto KEY_CWD = "${cwd}";
static constexpr auto KEY_ENV = "${env}";
static constexpr auto KEY_STOPONENTRY = "${stopOnEntry}";
static constexpr auto KEY_WORKSPACEFOLDER = "${workspaceFolder}";
static constexpr auto KEY_WORKSPACEROOT = "${workspaceRoot}";
static constexpr auto KEY_FILEDIRNAME = "${fileDirname}";
static constexpr auto KEY_RANDPORT = "${randPort}";
static constexpr auto KEY_PID = "${pid}";

static constexpr auto KEY_USER_HOME = "${userHome}";
static constexpr auto KEY_WORKSPACEFOLDER_BASENAME = "${workspaceFolderBasename}";
static constexpr auto KEY_FILE_WORKSPACEFOLDER = "${fileWorkspaceFolder}";
static constexpr auto KEY_RELATIVE_FILE = "${relativeFile}";
static constexpr auto KEY_RELATIVE_FILE_DIRNAME = "${relativeFileDirname}";
static constexpr auto KEY_FILE_BASENAME = "${fileBasename}";
static constexpr auto KEY_FILE_BASENAME_NOEXTENSION = "${fileBasenameNoExtension}";
static constexpr auto KEY_FILE_EXTNAME = "${fileExtname}";
static constexpr auto KEY_FILE_DIRNAME_BASENAME = "${fileDirnameBasename}";
static constexpr auto KEY_LINE_NUMBER = "${lineNumber}";
static constexpr auto KEY_SELECTED_TEXT = "${selectedText}";
static constexpr auto KEY_EXEC_PATH = "${execPath}";
static constexpr auto KEY_DEFAULT_BUILD_TASK = "${defaultBuildTask}";
static constexpr auto KEY_PATH_SEPARATOR = "${pathSeparator}";
static constexpr auto KEY_PATH_SEPARATOR_ABBR = "${/}";

static constexpr auto KEY_UUID = "${uuid}";
static constexpr auto KEY_TIMESTAMP = "${timestamp}";

static constexpr auto KEY_DEBUG_SERVER = "debugServer";

static void replaceExecutableArgs( std::string& arg ) {
	LuaPattern ptrn( "%$%b()" );
	PatternMatcher::Range match[4];
	while ( ptrn.matches( arg, match ) ) {
		auto cmd = arg.substr( match[0].start, match[0].end - match[0].start );
		if ( cmd.size() > 3 ) {
			auto rcmd = cmd.substr( 2, cmd.size() - 3 );
			std::string out;
			Process p;
			if ( p.create( rcmd ) ) {
				p.readAllStdOut( out );
				String::trimInPlace( out, '\n' );
				String::trimInPlace( out, '\t' );
				String::replaceAll( arg, cmd, out );
			}
		}
	};
}

static bool replaceEnvVars( std::string& arg, PluginContextProvider* ctx ) {
	PatternMatcher::Range match[4];
	while ( envPtrn.matches( arg, match ) ) {
		auto envCall = arg.substr( match[0].start, match[0].end - match[0].start );
		if ( envCall.size() > 6 ) {
			auto envName = envCall.substr( 6, envCall.size() - 7 );
			const char* env = getenv( envName.c_str() );
			if ( env && strlen( env ) != 0 ) {
				std::string out{ env };
				String::trimInPlace( out, '\n' );
				String::trimInPlace( out, '\t' );
				String::replaceAll( arg, envCall, out );
			} else {
				arg = "";
				ctx->getNotificationCenter()->addNotification( String::format(
					ctx->i18n( "command_parameter_env_var_not_found",
							   "The environment variable \"%s\" was not found but it's needed in "
							   "order to run the debugger." )
						.toUtf8(),
					envName ) );
				return false;
			}
		}
	};
	return true;
}

// Mouse Hover Tooltip
static Action::UniqueID getMouseMoveHash( UICodeEditor* editor ) {
	return hashCombine( String::hash( "DebuggerPlugin::onMouseMove-" ),
						reinterpret_cast<Action::UniqueID>( editor ) );
}

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
		mThreadPool->run( [this, pluginManager] { load( pluginManager ); } );
	}
}

DebuggerPlugin::~DebuggerPlugin() {
	waitUntilLoaded();
	mShuttingDown = true;

	{
		Lock l( mClientsMutex );
		for ( const auto& client : mClients )
			client.first->unregisterClient( client.second.get() );
	}

	if ( mSidePanel && mTab ) {
		if ( Engine::isMainThread() )
			mSidePanel->removeTab( mTab );
		else {
			auto sidePanel = mSidePanel;
			auto tab = mTab;
			mSidePanel->runOnMainThread( [sidePanel, tab] { sidePanel->removeTab( tab ); } );
		}
	}

	if ( getPluginContext()->getStatusBar() )
		getPluginContext()->getStatusBar()->removeStatusBarElement( "status_app_debugger" );

	mManager->unsubscribeMessages( this );

	for ( auto editor : mEditors ) {
		onBeforeUnregister( editor.first );
		onUnregisterEditor( editor.first );
	}

	mDebugger.reset();
	mListener.reset();

	if ( SceneManager::existsSingleton() && !SceneManager::instance()->isShuttingDown() &&
		 getPluginContext() && getPluginContext()->getMainLayout() ) {
		getPluginContext()->getMainLayout()->unsetCommands( mRegisteredCommands );

		for ( const auto& kb : mKeyBindings )
			getPluginContext()->getMainLayout()->getKeyBindings().removeCommandKeybind( kb.first );
	}
}

void DebuggerPlugin::onSaveProject( const std::string& /*projectFolder*/,
									const std::string& projectStatePath,
									bool rewriteStateOnlyIfNeeded ) {
	std::string debugger( mCurDebugger );
	std::string configuration( mCurConfiguration );
	auto expressions( mExpressions );
	BreakpointsHolder breakpoints;
	{
		Lock l( mBreakpointsMutex );
		breakpoints = mBreakpoints;
	}
	mThreadPool->run(
		[this, debugger = std::move( debugger ), configuration = std::move( configuration ),
		 expressions = std::move( expressions ), breakpoints = std::move( breakpoints ),
		 projectStatePath, rewriteStateOnlyIfNeeded] {
			nlohmann::json j;
			auto& config = j["config"];
			config["debugger"] = std::move( debugger );
			config["configuration"] = std::move( configuration );
			auto expArr = nlohmann::json::array();
			for ( auto& expression : expressions )
				expArr.push_back( expression );
			config["expressions"] = std::move( expArr );

			nlohmann::json bpsArr;
			for ( auto& bpSet : breakpoints ) {
				auto bpArr = nlohmann::json::array();
				for ( auto& bp : bpSet.second ) {
					nlohmann::json jbp;
					jbp["line"] = bp.line;
					jbp["enabled"] = bp.enabled;
					bpArr.push_back( jbp );
				}
				if ( !bpArr.empty() )
					bpsArr[bpSet.first] = std::move( bpArr );
			}
			config["breakpoints"] = std::move( bpsArr );

			std::string stateString( j.dump() );
			if ( stateString == mLastStateJsonDump )
				return;

			std::string debuggerStatePath( projectStatePath + "debugger.json" );
			if ( rewriteStateOnlyIfNeeded && FileSystem::fileExists( debuggerStatePath ) &&
				 MD5::fromFile( debuggerStatePath ) == MD5::fromString( stateString ) )
				return;
			FileSystem::fileWrite( debuggerStatePath, stateString );
			mLastStateJsonDump = stateString;
		} );
}

void DebuggerPlugin::onLoadProject( const std::string& projectFolder,
									const std::string& projectStatePath ) {
	mProjectPath = projectFolder;

	ScopedOp op( [this] { closeProject(); },
				 [this] {
					 auto sdc = getStatusDebuggerController();
					 if ( sdc && sdc->getUIBreakpoints() ) {
						 sdc->getUIBreakpoints()->runOnMainThread( [this, sdc] {
							 sdc->getUIBreakpoints()->setModel( mBreakpointsModel );
						 } );
					 }
				 } );

	std::string debuggerStatePath( projectStatePath + "debugger.json" );
	std::string data;
	if ( !FileSystem::fileGet( debuggerStatePath, data ) )
		return;

	auto sdc = getStatusDebuggerController();

	json j;
	try {
		j = json::parse( data, nullptr, true, true );
		if ( j.contains( "config" ) && j["config"].is_object() ) {
			auto& config = j["config"];
			mCurDebugger = config.value( "debugger", "" );
			mCurConfiguration = config.value( "configuration", "" );
			mExpressions.clear();
			if ( config.contains( "expressions" ) && config["expressions"].is_array() ) {
				auto& exps = config["expressions"];
				if ( mExpressionsHolder )
					mExpressionsHolder->clear( true );
				for ( const auto& expression : exps ) {
					if ( expression.is_string() ) {
						mExpressions.push_back( expression.get<std::string>() );
						if ( mExpressionsHolder ) {
							mExpressionsHolder->addChild(
								std::make_shared<ModelVariableNode>( expression, 0 ) );
						}
					}
				}

				if ( sdc && sdc->getWidget() && sdc->getUIExpressions() )
					sdc->getUIExpressions()->setModel( mExpressionsHolder->getModel() );
			}

			if ( config.contains( "breakpoints" ) && config["breakpoints"].is_object() ) {
				BreakpointsHolder breakpoints;
				for ( auto& [key, value] : config["breakpoints"].items() ) {
					if ( value.is_array() ) {
						BreakpointsSet set;
						for ( auto& jbp : value ) {
							SourceBreakpointStateful bp;
							bp.line = jbp.value( "line", 1 );
							bp.enabled = jbp.value( "enabled", true );
							if ( bp.line > 0 )
								set.insert( std::move( bp ) );
						}
						if ( !set.empty() )
							breakpoints[key] = std::move( set );
					}
				}

				{
					Lock l( mBreakpointsMutex );
					mBreakpoints = std::move( breakpoints );
					mBreakpointsModel =
						std::make_shared<BreakpointsModel>( mBreakpoints, getUISceneNode() );
				}

				if ( sdc && sdc->getWidget() && sdc->getUIBreakpoints() )
					sdc->getUIBreakpoints()->setModel( mBreakpointsModel );
			}
		}
	} catch ( const json::exception& e ) {
		Log::error(
			"DebuggerPlugin::onLoadProject - Error parsing config from path %s, error: %s, config "
			"file content:\n%s",
			debuggerStatePath, e.what(), data );
	}
}

std::vector<DapTool> DebuggerPlugin::getDebuggersForLang( const std::string& language ) {
	std::vector<DapTool> tools;
	for ( const auto& dap : mDaps ) {
		auto found = std::find_if( dap.languagesSupported.begin(), dap.languagesSupported.end(),
								   [&language]( const auto& l ) { return l == language; } );
		if ( found != dap.languagesSupported.end() )
			tools.push_back( dap );
	}
	return tools;
}

void DebuggerPlugin::resetExpressions() {
	if ( !mExpressionsHolder )
		return;
	mExpressionsHolder->clear( true );
	std::vector<ModelVariableNode::NodePtr> children;
	children.reserve( mExpressions.size() );
	for ( const auto& expression : mExpressions )
		children.emplace_back( std::make_shared<ModelVariableNode>( expression, 0 ) );
	mExpressionsHolder->addChildren( children );
}

void DebuggerPlugin::closeProject() {
	exitDebugger( true );

	mDapInputs.clear();
	mExpressions.clear();
	if ( mExpressionsHolder )
		mExpressionsHolder->clear( true );

	{
		Lock l( mBreakpointsMutex );
		mBreakpointsModel.reset();
		mPendingBreakpoints.clear();
		mBreakpoints.clear();
	}

	auto sdc = getStatusDebuggerController();
	if ( sdc && sdc->getWidget() ) {
		sdc->getUIBreakpoints()->setModel( nullptr );
		sdc->getUIExpressions()->setModel( nullptr );
		sdc->getUIStack()->setModel( nullptr );
		sdc->getUIThreads()->setModel( nullptr );
		sdc->getUIVariables()->setModel( nullptr );
		sdc->getUIVariables()->clearViewMetadata();
	}
}

void DebuggerPlugin::load( PluginManager* pluginManager ) {
	Clock clock;
	AtomicBoolScopedOp loading( mLoading, true );
	pluginManager->subscribeMessages( this,
									  [this]( const auto& notification ) -> PluginRequestHandle {
										  return processMessage( notification );
									  } );
	std::vector<std::string> paths;
	std::string path( pluginManager->getResourcesPath() + "plugins/debugger.json" );
	if ( FileSystem::fileExists( path ) )
		paths.emplace_back( path );
	path = pluginManager->getPluginsPath() + "debugger.json";
	if ( FileSystem::fileExists( path ) ||
		 FileSystem::fileWrite(
			 path, "{\n  \"config\":{},\n  \"keybindings\":{},\n  \"dap\":[]\n}\n" ) ) {
		mConfigPath = path;
		paths.emplace_back( path );
	}
	if ( paths.empty() )
		return;

	for ( const auto& ipath : paths ) {
		try {
			loadDAPConfig( ipath, mConfigPath == ipath );
		} catch ( const json::exception& e ) {
			Log::error( "Parsing Debugger \"%s\" failed:\n%s", ipath.c_str(), e.what() );
		}
	}

	if ( getUISceneNode() )
		updateUI();

	subscribeFileSystemListener();
	mReady = true;
	fireReadyCbs();
	setReady( clock.getElapsedTime() );
}

static std::initializer_list<std::string> DebuggerCommandList = {
	"debugger-continue-interrupt",
	"debugger-breakpoint-toggle",
	"debugger-breakpoint-enable-toggle",
	"debugger-start",
	"debugger-stop",
	"debugger-start-stop",
	"debugger-step-over",
	"debugger-step-into",
	"debugger-step-out",
	"toggle-status-app-debugger",
};

void DebuggerPlugin::loadDAPConfig( const std::string& path, bool updateConfigFile ) {
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;

	if ( updateConfigFile )
		mConfigHash = String::hash( data );

	json j;
	try {
		j = json::parse( data, nullptr, true, true );
	} catch ( const json::exception& e ) {
		Log::error( "DebuggerPlugin::load - Error parsing config from path %s, error: %s, config "
					"file content:\n%s",
					path.c_str(), e.what(), data.c_str() );
		// Recreate it
		j = json::parse( "{\n  \"config\":{},\n  \"dap\":{},\n  \"keybindings\":{},\n}\n", nullptr,
						 true, true );
	}

	if ( j.contains( "config" ) ) {
		auto& config = j["config"];
		if ( config.contains( "fetch_registers" ) )
			mFetchRegisters = config.value( "fetch_registers", false );
		else if ( updateConfigFile )
			config["fetch_registers"] = mFetchRegisters;

		if ( config.contains( "fetch_globals" ) )
			mFetchGlobals = config.value( "fetch_globals", false );
		else if ( updateConfigFile )
			config["fetch_globals"] = mFetchGlobals;

		if ( config.contains( "silent" ) )
			mSilence = config.value( "silent", true );
		else if ( updateConfigFile )
			config["silent"] = mSilence;
	}

	if ( j.contains( "dap" ) ) {
		auto& dapArr = j["dap"];
		mDaps.reserve( dapArr.size() );
		for ( const auto& dap : dapArr ) {
			DapTool dapTool;
			dapTool.name = dap.value( "name", "" );
			dapTool.unstableFrameId = dap.value( "unstable_frame_id", false );

			if ( dap.contains( "type" ) && dap["type"].is_array() ) {
				for ( const auto& obj : dap["type"] )
					if ( obj.is_string() )
						dapTool.type.emplace_back( obj.get<std::string>() );
			} else if ( dap.contains( "type" ) && dap["type"].is_string() ) {
				dapTool.type.emplace_back( dap.value( "type", "" ) );
			}

			dapTool.url = dap.value( "url", "" );

			if ( dap.contains( "run" ) ) {
				auto& run = dap["run"];
				dapTool.run.command = run.value( "command", "" );
				dapTool.redirectStdout = run.value( "redirectStdout", false );
				dapTool.redirectStderr = run.value( "redirectStderr", false );
				dapTool.supportsSourceRequest = run.value( "supportsSourceRequest", true );
				dapTool.fallbackCommand = run.value( "command_fallback", "" );
				if ( run.contains( "command_arguments" ) ) {
					if ( run["command_arguments"].is_array() ) {
						auto& args = run["command_arguments"];
						dapTool.run.args.reserve( args.size() );
						for ( auto& arg : args ) {
							if ( arg.is_string() )
								dapTool.run.args.emplace_back( arg.get<std::string>() );
						}
					} else if ( run["command_arguments"].is_string() ) {
						dapTool.run.args.push_back( run["command_arguments"].get<std::string>() );
					}
				}
			}

			if ( dap.contains( "configurations" ) ) {
				auto& configs = dap["configurations"];
				dapTool.configurations.reserve( configs.size() );
				for ( auto& config : configs ) {
					DapConfig dapConfig;
					dapConfig.name = config.value( "name", "" );
					if ( !dapConfig.name.empty() ) {
						dapConfig.request = config.value( "request", REQUEST_TYPE_LAUNCH );
						dapConfig.runTarget = config.value( "runTarget", false );
						dapConfig.args = config["arguments"];
					}
					if ( config.contains( "command_arguments" ) ) {
						if ( config["command_arguments"].is_array() ) {
							auto& args = config["command_arguments"];
							dapConfig.cmdArgs.reserve( args.size() );
							for ( auto& arg : args ) {
								if ( arg.is_string() )
									dapConfig.cmdArgs.emplace_back( arg.get<std::string>() );
							}
						} else if ( config["command_arguments"].is_string() ) {
							dapConfig.cmdArgs.push_back(
								config["command_arguments"].get<std::string>() );
						}
					}
					dapTool.configurations.emplace_back( std::move( dapConfig ) );
				}
			}

			if ( dap.contains( "languages" ) && dap["languages"].is_array() ) {
				auto& languages = dap["languages"];
				dapTool.languagesSupported.reserve( languages.size() );
				for ( const auto& lang : languages ) {
					if ( lang.is_string() )
						dapTool.languagesSupported.emplace_back( lang.get<std::string>() );
				}
			}

			if ( dap.contains( "find" ) && dap["find"].is_object() ) {
				auto find = dap["find"].items();
				for ( const auto& [key, val] : find ) {
					if ( val.is_string() )
						dapTool.findBinary[key] = String::toLower( val.get<std::string>() );
				}
			}

			mDaps.emplace_back( std::move( dapTool ) );
		}
	}

	if ( mKeyBindings.empty() ) {
		mKeyBindings["debugger-start-stop"] = "mod+f5";
		mKeyBindings["debugger-continue-interrupt"] = "f5";
		mKeyBindings["debugger-breakpoint-toggle"] = "f9";
		mKeyBindings["debugger-breakpoint-enable-toggle"] = "mod+f9";
		mKeyBindings["debugger-step-over"] = "f10";
		mKeyBindings["debugger-step-into"] = "f11";
		mKeyBindings["debugger-step-out"] = "shift+f11";
		mKeyBindings["toggle-status-app-debugger"] = "alt+6";
	}

	if ( j.contains( "keybindings" ) ) {
		auto& kb = j["keybindings"];
		for ( const auto& key : DebuggerCommandList ) {
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
}

void DebuggerPlugin::loadProjectConfiguration( const std::string& path ) {
	std::string data;
	if ( !FileSystem::fileGet( path, data ) )
		return;

	json j;
	try {
		j = json::parse( data, nullptr, true, true );
	} catch ( const json::exception& e ) {
		Log::error( "DebuggerPlugin::loadProjectConfiguration - Error parsing config from path %s, "
					"error: %s, config "
					"file content:\n%s",
					path.c_str(), e.what(), data.c_str() );
		return;
	}

	if ( !j.contains( "configurations" ) || !j["configurations"].is_array() ) {
		Log::warning( "DebuggerPlugin::loadProjectConfiguration: wrong format in %s", path );
		return;
	}

	auto& confs = j["configurations"];

	for ( const auto& conf : confs ) {
		DapConfig dapConfig;
		auto type = conf.value( "type", "" );

		if ( !std::any_of( mDaps.begin(), mDaps.end(), [&type]( const DapTool& dap ) {
				 return dap.type.end() != std::find( dap.type.begin(), dap.type.end(), type );
			 } ) )
			continue;

		dapConfig.name = conf.value( "name", "" );
		if ( dapConfig.name.empty() )
			continue;
		dapConfig.request = conf.value( "request", "" );
		if ( dapConfig.request != REQUEST_TYPE_LAUNCH && dapConfig.request != REQUEST_TYPE_ATTACH )
			continue;
		dapConfig.args = std::move( conf );

		{
			Lock l( mDapsMutex );
			mDapConfigs.emplace_back( std::move( dapConfig ) );
		}
	}

	if ( !j.contains( "inputs" ) )
		return;

	auto& inputs = j["inputs"];

	for ( const auto& input : inputs ) {
		if ( !input.is_object() )
			continue;
		DapConfigurationInput in;
		std::string id = input.value( "id", "" );
		in.id = id;
		if ( in.id.empty() )
			continue;
		in.type = input.value( "type", "" );
		String::toLowerInPlace( in.type );
		if ( "promptstring" != in.type && "pickstring" != in.type )
			continue;
		in.description = input.value( "description", "" );
		in.defaultValue = input.value( "default", "" );
		if ( input.contains( "options" ) && input["options"].is_array() ) {
			auto& opts = input["options"];
			for ( const auto& opt : opts ) {
				if ( opt.is_string() )
					in.options.push_back( opt );
			}
		}
		mDapInputs[id] = std::move( in );
	}
}

void DebuggerPlugin::loadProjectConfigurations() {
	if ( mProjectPath.empty() )
		return;

	{
		Lock l( mDapsMutex );
		mDapConfigs.clear();
	}

	mThreadPool->run( [this] {
		std::string config = mProjectPath + ".vscode/launch.json";
		if ( FileSystem::fileExists( config ) )
			loadProjectConfiguration( config );
		config = mProjectPath + ".ecode/launch.json";
		if ( FileSystem::fileExists( config ) )
			loadProjectConfiguration( config );

		getUISceneNode()->runOnMainThread( [this] {
			updateDebuggerConfigurationList();
			updateSelectedDebugConfig();
		} );
	} );
}

PluginRequestHandle DebuggerPlugin::processMessage( const PluginMessage& msg ) {
	switch ( msg.type ) {
		case PluginMessageType::WorkspaceFolderChanged: {
			mProjectPath = msg.asJSON()["folder"];

			if ( getUISceneNode() && mSidePanel ) {
				getUISceneNode()->runOnMainThread( [this] {
					if ( mProjectPath.empty() ) {
						hideSidePanel();
						Lock l( mDapsMutex );
						mDapConfigs.clear();
					}
				} );
			}

			loadProjectConfigurations();

			updateUI();
			mInitialized = true;
			break;
		}
		case ecode::PluginMessageType::UIReady: {
			registerCommands( getPluginContext()->getMainLayout() );
			for ( const auto& kb : mKeyBindings ) {
				getPluginContext()->getMainLayout()->getKeyBindings().addKeybindString( kb.second,
																						kb.first );
			}

			if ( !mInitialized )
				updateUI();

			break;
		}
		default:
			break;
	}
	return PluginRequestHandle::empty();
}

void DebuggerPlugin::updateUI() {
	if ( !getUISceneNode() )
		return;

	getUISceneNode()->runOnMainThread( [this] {
		buildSidePanelTab();
		buildStatusBar();
	} );
}

void DebuggerPlugin::buildSidePanelTab() {
	if ( mTabContents && !mTab ) {
		if ( mProjectPath.empty() )
			return;
		UIIcon* icon = findIcon( "debug" );
		mTab = mSidePanel->add( i18n( "debugger", "Debugger" ), mTabContents,
								icon ? icon->getSize( PixelDensity::dpToPx( 12 ) ) : nullptr );
		mTab->setId( "debugger_tab" );
		mTab->setTextAsFallback( true );

		updateSidePanelTab();
		return;
	}
	if ( mTab )
		return;
	if ( mSidePanel == nullptr )
		getUISceneNode()->bind( "panel", mSidePanel );

	static constexpr auto STYLE = R"html(
	<style>
	#panel_debugger_buttons > PushButton {
		clip: none;
		border-radius: 0;
		transition-delay: 0s;
	}
	#panel_debugger_buttons > PushButton:first-of-type {
		border-top-left-radius: var(--button-radius);
		border-bottom-left-radius: var(--button-radius);
	}
	#panel_debugger_buttons > PushButton:last-of-type {
		border-top-right-radius: var(--button-radius);
		border-bottom-right-radius: var(--button-radius);
	}
	#panel_debugger_help {
		color: var(--primary);
		text-decoration: none;
		text-align: center;
		margin-top: 32dp;
	}
	#panel_debugger_help:hover {
		text-decoration: underline;
	}
	</style>
	<vbox id="debugger_panel" lw="mp" lh="wc" padding="4dp">
		<vbox id="debugger_config_view" lw="mp" lh="wc">
			<TextView text="@string(debugger, Debugger)" font-size="15dp" focusable="false" />
			<DropDownList id="debugger_list" layout_width="mp" layout_height="wrap_content" margin-top="2dp" />
			<TextView text="@string(debugger_configuration, Debugger Configuration)" focusable="false" margin-top="8dp" />
			<DropDownList id="debugger_conf_list" layout_width="mp" layout_height="wrap_content" margin-top="2dp" />
			<PushButton id="debugger_run_button" lw="mp" lh="wc" text="@string(debug, Debug)" margin-top="8dp" icon="icon(debug-alt, 12dp)" />
			<PushButton id="debugger_build_and_run_button" lw="mp" lh="wc" text="@string(build_and_debug, Build & Debug)" margin-top="8dp" icon="icon(debug-alt, 12dp)" />
			<hbox id="panel_debugger_buttons" lw="wc" lh="wc" layout_gravity="center_horizontal" visible="false" clip="none" margin-top="8dp">
				<PushButton id="panel_debugger_continue" class="debugger_continue" lw="24dp" lh="24dp" icon="icon(debug-continue, 12dp)" tooltip="@string(continue, Continue)" />
				<PushButton id="panel_debugger_pause" class="debugger_pause" lw="24dp" lh="24dp" icon="icon(debug-pause, 12dp)" tooltip="@string(pause, Pause)" />
				<PushButton id="panel_debugger_step_over" class="debugger_step_over" lw="24dp" lh="24dp" icon="icon(debug-step-over, 12dp)" tooltip="@string(step_over, Step Over)" />
				<PushButton id="panel_debugger_step_into" class="debugger_step_into" lw="24dp" lh="24dp" icon="icon(debug-step-into, 12dp)" tooltip="@string(step_into, Step Into)" />
				<PushButton id="panel_debugger_step_out" class="debugger_step_out" lw="24dp" lh="24dp" icon="icon(debug-step-out, 12dp)" tooltip="@string(step_out, Step Out)" />
			</hbox>
			<Anchor id="panel_debugger_help" lw="mp" text="@string(documentation, Documentation)" href="https://github.com/SpartanJ/ecode/blob/main/docs/debugger.md" />
		</vbox>
	</vbox>
	)html";
	UIIcon* icon = findIcon( "debug" );
	mTabContents = getUISceneNode()->loadLayoutFromString( STYLE );
	mTab = mSidePanel->add( i18n( "debugger", "Debugger" ), mTabContents,
							icon ? icon->getSize( PixelDensity::dpToPx( 12 ) ) : nullptr );
	mTab->setId( "debugger_tab" );
	mTab->setTextAsFallback( true );

	mTabContents->bind( "debugger_list", mUIDebuggerList );
	mTabContents->bind( "debugger_conf_list", mUIDebuggerConfList );

	mUIDebuggerList->on( Event::OnValueChange, [this]( const Event* event ) {
		auto cur = event->getNode()->asType<UIDropDownList>()->getText().toUtf8();
		if ( cur != mCurDebugger ) {
			mCurDebugger = std::move( cur );
			mCurConfiguration = "";
		}
	} );

	mUIDebuggerConfList->on( Event::OnValueChange, [this]( const Event* event ) {
		mCurConfiguration = event->getNode()->asType<UIDropDownList>()->getText().toUtf8();
	} );

	mTabContents->bind( "panel_debugger_buttons", mPanelBoxButtons.box );
	mTabContents->bind( "panel_debugger_continue", mPanelBoxButtons.resume );
	mTabContents->bind( "panel_debugger_pause", mPanelBoxButtons.pause );
	mTabContents->bind( "panel_debugger_step_over", mPanelBoxButtons.stepOver );
	mTabContents->bind( "panel_debugger_step_into", mPanelBoxButtons.stepInto );
	mTabContents->bind( "panel_debugger_step_out", mPanelBoxButtons.stepOut );

	const auto addKb = [this]( UIPushButton* but, const std::string& cmd ) {
		auto ctx = mManager->getPluginContext();
		auto kb = ctx->getKeybind( cmd );
		if ( !kb.empty() ) {
			auto tt( but->getTooltipText() );
			but->setTooltipText( tt.empty() ? String{ kb } : String{ tt + " ( " + kb + " )" } );
		}
	};
	addKb( mPanelBoxButtons.resume, "debugger-continue-interrupt" );
	addKb( mPanelBoxButtons.pause, "debugger-continue-interrupt" );
	addKb( mPanelBoxButtons.stepOver, "debugger-step-over" );
	addKb( mPanelBoxButtons.stepInto, "debugger-step-into" );
	addKb( mPanelBoxButtons.stepOut, "debugger-step-out" );

	mPanelBoxButtons.resume->onClick(
		[this]( auto ) { getPluginContext()->runCommand( "debugger-continue-interrupt" ); } );

	mPanelBoxButtons.pause->onClick(
		[this]( auto ) { getPluginContext()->runCommand( "debugger-continue-interrupt" ); } );

	mPanelBoxButtons.stepOver->onClick(
		[this]( auto ) { getPluginContext()->runCommand( "debugger-step-over" ); } );

	mPanelBoxButtons.stepInto->onClick(
		[this]( auto ) { getPluginContext()->runCommand( "debugger-step-into" ); } );

	mPanelBoxButtons.stepOut->onClick(
		[this]( auto ) { getPluginContext()->runCommand( "debugger-step-out" ); } );

	mTabContents->bind( "debugger_run_button", mRunButton );

	mTabContents->bind( "debugger_build_and_run_button", mBuildAndRunButton );

	mRunButton->setTooltipText( getPluginContext()->getKeybind( "debugger-start-stop" ) );

	mBuildAndRunButton->setTooltipText(
		getPluginContext()->getKeybind( "debugger-continue-interrupt" ) );

	mRunButton->onClick(
		[this]( auto ) { getPluginContext()->runCommand( "debugger-start-stop" ); } );

	mBuildAndRunButton->onClick(
		[this]( auto ) { getPluginContext()->runCommand( "debugger-continue-interrupt" ); } );

	setUIDebuggingState( StatusDebuggerController::State::NotStarted );

	updateSidePanelTab();

	updateSelectedDebugConfig();

	if ( mProjectPath.empty() )
		hideSidePanel();
}

void DebuggerPlugin::updateSelectedDebugConfig() {
	if ( mUIDebuggerList && mUIDebuggerConfList && !mCurDebugger.empty() ) {
		mUIDebuggerList->runOnMainThread( [this] {
			auto curConfig = mCurConfiguration;
			mUIDebuggerList->getListBox()->setSelected( mCurDebugger );
			mUIDebuggerConfList->getListBox()->setSelected( curConfig );
		} );
	}
}

void DebuggerPlugin::removeExpression( const std::string& name ) {
	auto expIt = std::find( mExpressions.begin(), mExpressions.end(), name );
	if ( expIt == mExpressions.end() )
		return;

	mExpressions.erase( expIt );
	resetExpressions();
}

void DebuggerPlugin::openExpressionMenu( ModelIndex idx ) {
	UIPopUpMenu* menu = UIPopUpMenu::New();
	auto context = getPluginContext();

	if ( idx.isValid() ) {
		menu->add( context->i18n( "debugger_remove_expression", "Remove Expression" ),
				   context->findIcon( "remove" ) )
			->setId( "debugger_remove_expression" );
	}

	menu->add( context->i18n( "debugger_add_expression", "Add Expression..." ),
			   context->findIcon( "add" ) )
		->setId( "debugger_add_expression" );

	menu->on( Event::OnItemClicked, [this, context, idx]( const Event* event ) {
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		std::string id( item->getId() );
		if ( id == "debugger_remove_expression" ) {
			ModelVariableNode* node = static_cast<ModelVariableNode*>( idx.internalData() );
			if ( mExpressionsHolder && node->getParent() == nullptr )
				removeExpression( node->var.name );
		} else if ( id == "debugger_add_expression" ) {
			auto msgBox =
				UIMessageBox::New( UIMessageBox::INPUT, context->i18n( "debugger_add_expression",
																	   "Add Expression..." ) );
			msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
			msgBox->showWhenReady();
			msgBox->on( Event::OnConfirm, [this, msgBox]( const Event* ) {
				std::string expression( msgBox->getTextInput()->getText().toUtf8() );
				if ( std::find( mExpressions.begin(), mExpressions.end(), expression ) ==
					 mExpressions.end() ) {
					mExpressions.push_back( expression );
					mExpressionsHolder->addChild(
						std::make_shared<ModelVariableNode>( expression, 0 ) );
					msgBox->closeWindow();

					if ( mDebuggingState == StatusDebuggerController::State::Paused && mListener ) {
						mListener->evaluateExpression( expression );
					}
				}
			} );
		}
	} );

	Vector2f pos( context->getWindow()->getInput()->getMousePos().asFloat() );
	menu->nodeToWorldTranslation( pos );
	UIMenu::findBestMenuPos( pos, menu );
	menu->setPixelsPosition( pos );
	menu->show();
}
void DebuggerPlugin::buildStatusBar() {
	if ( mProjectPath.empty() ) {
		hideStatusBarElement();
		return;
	}
	if ( getPluginContext()->getStatusBar() ) {
		auto but = getPluginContext()->getStatusBar()->find( "status_app_debugger" );
		if ( but ) {
			but->setVisible( true );
			return;
		}
	}

	auto context = getPluginContext();
	UIStatusBar* statusBar = context->getStatusBar();

	auto debuggerStatusElem = std::make_shared<StatusDebuggerController>(
		context->getMainSplitter(), getUISceneNode(), context );

	statusBar->insertStatusBarElement( "status_app_debugger", i18n( "debugger", "Debugger" ),
									   "icon(debug, 11dp)", debuggerStatusElem );

	debuggerStatusElem->onWidgetCreated = [this]( StatusDebuggerController* sdc, UIWidget* ) {
		UITreeView* uiExpressions = sdc->getUIExpressions();
		uiExpressions->setModel( mExpressionsHolder->getModel() );
		uiExpressions->removeEventsOfType( Event::OnModelEvent );
		uiExpressions->onModelEvent( [this]( const ModelEvent* modelEvent ) {
			if ( modelEvent->getModelEventType() == Abstract::ModelEventType::OpenMenu ) {
				openExpressionMenu( modelEvent->getModelIndex() );
			} else if ( mDebugger && mListener &&
						modelEvent->getModelEventType() == Abstract::ModelEventType::OpenTree ) {
				ModelVariableNode* node =
					static_cast<ModelVariableNode*>( modelEvent->getModelIndex().internalData() );
				mDebugger->variables(
					node->var.variablesReference, Variable::Type::Both,
					[this]( const int variablesReference, std::vector<Variable>&& vars ) {
						mExpressionsHolder->addVariables( variablesReference, std::move( vars ) );
					} );
				mExpressionsHolder->saveExpandedState( modelEvent->getModelIndex(), true );
			} else if ( mDebugger && mListener &&
						modelEvent->getModelEventType() == Abstract::ModelEventType::CloseTree ) {
				mExpressionsHolder->removeExpandedState( modelEvent->getModelIndex(), true );
			}
		} );
		uiExpressions->removeEventsOfType( Event::MouseClick );
		uiExpressions->onClick( [this]( const MouseEvent* ) { openExpressionMenu( {} ); },
								MouseButton::EE_BUTTON_RIGHT );

		auto uiBreakpoints = sdc->getUIBreakpoints();
		if ( nullptr == uiBreakpoints->onBreakpointEnabledChange ) {
			uiBreakpoints->onBreakpointEnabledChange = [this]( const std::string& filePath,
															   int line, bool enabled ) {
				breakpointSetEnabled( filePath, line, enabled );
			};
		}

		if ( nullptr == uiBreakpoints->onBreakpointRemove ) {
			uiBreakpoints->onBreakpointRemove = [this]( const std::string& filePath, int line ) {
				setBreakpoint( filePath, line );
			};
		}

		uiBreakpoints->setModel( mBreakpointsModel );

		uiBreakpoints->onModelEvent( [this]( const ModelEvent* modelEvent ) {
			if ( modelEvent->getModelEventType() == Abstract::ModelEventType::OpenMenu ) {
				// Implement
			} else if ( modelEvent->getModelEventType() == Abstract::ModelEventType::Open ) {
				auto idx( modelEvent->getModelIndex() );
				auto srcIdx( modelEvent->getModel()->index(
					idx.row(), BreakpointsModel::Columns::SourcePath, idx.parent() ) );
				auto lineIdx( modelEvent->getModel()->index(
					idx.row(), BreakpointsModel::Columns::Line, idx.parent() ) );
				Variant sourcePathVar( modelEvent->getModel()->data( srcIdx, ModelRole::Data ) );
				Variant lineVar( modelEvent->getModel()->data( lineIdx, ModelRole::Data ) );
				TextPosition pos( lineVar.asInt() - 1, 0 );
				getPluginContext()->focusOrLoadFile( sourcePathVar.toString(), { pos, pos } );
			}
		} );
	};

	if ( !mExpressionsHolder ) {
		mExpressionsHolder = std::make_shared<VariablesHolder>( getUISceneNode() );
		resetExpressions();
	}

	if ( !mHoverExpressionsHolder ) {
		mHoverExpressionsHolder = std::make_shared<VariablesHolder>( getUISceneNode() );
	}
}

void DebuggerPlugin::updateSidePanelTab() {
	mUIDebuggerList->getListBox()->clear();

	std::vector<String> debuggerNames;
	debuggerNames.reserve( mDaps.size() );
	for ( const auto& dap : mDaps )
		debuggerNames.emplace_back( dap.name );
	std::sort( debuggerNames.begin(), debuggerNames.end() );
	mUIDebuggerList->getListBox()->addListBoxItems( debuggerNames );
	bool empty = mUIDebuggerList->getListBox()->isEmpty();
	mUIDebuggerList->setEnabled( !empty );

	if ( !mUIDebuggerList->hasEventsOfType( Event::OnItemSelected ) ) {
		mUIDebuggerList->on( Event::OnItemSelected,
							 [this]( const Event* ) { updateDebuggerConfigurationList(); } );
	}

	if ( !empty ) {
		if ( !mCurDebugger.empty() ) {
			auto curConfig = mCurConfiguration;
			mUIDebuggerList->getListBox()->setSelected( mCurDebugger );
			mCurConfiguration = std::move( curConfig );
		} else
			mUIDebuggerList->getListBox()->setSelected( 0L );
	}

	updateDebuggerConfigurationList();
}

void DebuggerPlugin::runCurrentConfig() {
	runConfig( mCurDebugger, mCurConfiguration );
}

void DebuggerPlugin::sendPendingBreakpoints() {
	for ( const auto& pbp : mPendingBreakpoints )
		sendFileBreakpoints( pbp );
	mPendingBreakpoints.clear();
	resume( mListener->getCurrentThreadId() );
}

void DebuggerPlugin::sendFileBreakpoints( const std::string& filePath ) {
	if ( !mDebugger || !mListener || !mDebugger->isServerConnected() )
		return;

	if ( !mListener->isStopped() ) {
		mPendingBreakpoints.insert( filePath );
		mListener->setPausedToRefreshBreakpoints();
		mDebugger->pause( 1 );
		return;
	}

	{
		Lock l( mBreakpointsMutex );
		auto fileBps = mBreakpoints.find( filePath );
		if ( fileBps == mBreakpoints.end() )
			return;
	}
	mListener->sendBreakpoints();
}

void DebuggerPlugin::updateDebuggerConfigurationList() {
	if ( nullptr == mUIDebuggerConfList )
		return;

	std::string curConfig( mCurConfiguration );
	mUIDebuggerConfList->getListBox()->clear();
	mCurConfiguration = std::move( curConfig );

	std::string debuggerSelected = mUIDebuggerList->getListBox()->getItemSelectedText().toUtf8();

	auto debuggerIt =
		std::find_if( mDaps.begin(), mDaps.end(), [&debuggerSelected]( const DapTool& dap ) {
			return dap.name == debuggerSelected;
		} );

	if ( debuggerIt == mDaps.end() ) {
		mUIDebuggerConfList->setEnabled( false );
		return;
	}

	std::vector<String> confNames;
	confNames.reserve( mDaps.size() );
	for ( const auto& conf : debuggerIt->configurations )
		confNames.emplace_back( conf.name );

	{
		Lock l( mDapsMutex );
		for ( const auto& conf : mDapConfigs ) {
			auto type( conf.args.value( "type", "" ) );

			if ( std::find( debuggerIt->type.begin(), debuggerIt->type.end(), type ) ==
				 debuggerIt->type.end() )
				continue;

			confNames.emplace_back( conf.name );
		}
	}

	std::sort( confNames.begin(), confNames.end() );
	mUIDebuggerConfList->getListBox()->addListBoxItems( confNames );
	bool empty = mUIDebuggerConfList->getListBox()->isEmpty();
	mUIDebuggerConfList->setEnabled( !empty );
	if ( !empty ) {
		if ( !mCurConfiguration.empty() )
			mUIDebuggerConfList->getListBox()->setSelected( mCurConfiguration );
		else
			mUIDebuggerConfList->getListBox()->setSelected( 0L );
	}
}

bool DebuggerPlugin::replaceInVal( std::string& val,
								   const std::optional<ProjectBuildStep>& runConfig,
								   ProjectBuild* buildConfig, int randomPort ) {

	if ( runConfig ) {
		String::replaceAll( val, KEY_FILE, runConfig->cmd );
		String::replaceAll( val, KEY_CWD, runConfig->workingDir );
		String::replaceAll( val, KEY_FILEDIRNAME, runConfig->workingDir );
		std::string fileRelativePath( runConfig->cmd );
		FileSystem::filePathRemoveBasePath( mProjectPath, fileRelativePath );
		String::replaceAll( val, KEY_RELATIVE_FILE, fileRelativePath );
		std::string fileDirName( FileSystem::fileRemoveFileName( fileRelativePath ) );
		FileSystem::dirRemoveSlashAtEnd( fileDirName );
		String::replaceAll( val, KEY_RELATIVE_FILE_DIRNAME, fileDirName );
		String::replaceAll( val, KEY_FILE_BASENAME,
							FileSystem::fileNameFromPath( fileRelativePath ) );
		String::replaceAll(
			val, KEY_FILE_BASENAME_NOEXTENSION,
			FileSystem::fileRemoveExtension( FileSystem::fileNameFromPath( fileRelativePath ) ) );
		String::replaceAll( val, KEY_FILE_EXTNAME, FileSystem::fileExtension( fileRelativePath ) );
		String::replaceAll( val, KEY_FILE_DIRNAME_BASENAME,
							FileSystem::fileNameFromPath( fileDirName ) );
	}

	if ( buildConfig ) {
		String::replaceAll( val, KEY_DEFAULT_BUILD_TASK, buildConfig->getName() );
	}

	String::replaceAll( val, KEY_WORKSPACEFOLDER, mProjectPath );
	String::replaceAll( val, KEY_WORKSPACEROOT, mProjectPath );
	String::replaceAll( val, KEY_USER_HOME, Sys::getUserDirectory() );
	String::replaceAll( val, KEY_WORKSPACEFOLDER_BASENAME,
						FileSystem::fileNameFromPath( mProjectPath ) );
	String::replaceAll( val, KEY_FILE_WORKSPACEFOLDER, mProjectPath );
	String::replaceAll( val, KEY_EXEC_PATH, Sys::getProcessFilePath() );
	String::replaceAll( val, KEY_PATH_SEPARATOR, FileSystem::getOSSlash() );
	String::replaceAll( val, KEY_PATH_SEPARATOR_ABBR, FileSystem::getOSSlash() );
	String::replaceAll( val, KEY_UUID, UUID().toString() );
	String::replaceAll( val, KEY_TIMESTAMP,
						std::to_string( std::chrono::system_clock::to_time_t(
							std::chrono::system_clock::now() ) ) );

	auto* editor = getPluginContext()->getSplitter()->getCurEditor();
	if ( getPluginContext()->getSplitter()->getCurEditor() ) {
		String::replaceAll(
			val, KEY_LINE_NUMBER,
			String::toString( editor->getDocument().getSelection().start().line() ) );
		String::replaceAll( val, KEY_SELECTED_TEXT,
							editor->getDocument().getSelectedText().toUtf8() );
	}

	if ( String::contains( val, KEY_RANDPORT ) )
		String::replaceAll( val, KEY_RANDPORT, String::toString( randomPort ) );

	return replaceEnvVars( val, this->getPluginContext() );
}

std::pair<bool, std::vector<std::string>> DebuggerPlugin::replaceKeyInString(
	std::string val, int randomPort,
	const std::unordered_map<std::string, std::string>& solvedInputs ) {
	auto pbm = getPluginContext()->getProjectBuildManager();
	auto runConfig = pbm ? pbm->getCurrentRunConfig() : std::optional<ProjectBuildStep>{};
	auto buildConfig = pbm ? pbm->getCurrentBuild() : nullptr;

	const auto replaceVal = [this, &runConfig, &buildConfig, &solvedInputs,
							 randomPort]( std::string& val ) -> bool {
		if ( !replaceInVal( val, runConfig, buildConfig, randomPort ) )
			return false;

		LuaPattern::Range matches[2];
		if ( inputPtrn.matches( val, matches ) ) {
			std::string id( val.substr( matches[1].start, matches[1].end - matches[1].start ) );
			auto solvedIdIt = solvedInputs.find( id );
			if ( solvedIdIt != solvedInputs.end() )
				String::replaceAll( val, String::format( "${input:%s}", id ), solvedIdIt->second );
		}

		LuaPattern::Range matches2[2];
		if ( commandPtrn.matches( val, matches2 ) ) {
			std::string id( val.substr( matches2[1].start, matches2[1].end - matches2[1].start ) );
			String::toLowerInPlace( id );
			auto solvedIdIt = solvedInputs.find( id );
			if ( solvedIdIt != solvedInputs.end() )
				val = solvedIdIt->second;
		}

		return true;
	};

	if ( runConfig && val == KEY_ARGS ) {
		std::vector<std::string> argsArr;
		auto args = Process::parseArgs( runConfig->args );
		for ( auto arg : args ) {
			replaceVal( arg );
			argsArr.push_back( arg );
		}
		return { true, argsArr };
	}

	if ( !replaceVal( val ) )
		return { false, {} };

	return { true, { val } };
}

void DebuggerPlugin::replaceKeysInJson(
	nlohmann::json& json, int randomPort,
	const std::unordered_map<std::string, std::string>& solvedInputs ) {
	auto pbm = getPluginContext()->getProjectBuildManager();
	auto runConfig = pbm ? pbm->getCurrentRunConfig() : std::optional<ProjectBuildStep>{};
	auto buildConfig = pbm ? pbm->getCurrentBuild() : nullptr;

	const auto replaceVal = [this, &solvedInputs, &runConfig, &buildConfig,
							 randomPort]( nlohmann::json& j, std::string& val ) -> bool {
		replaceInVal( val, runConfig, buildConfig, randomPort );

		LuaPattern::Range matches[2];
		if ( inputPtrn.matches( val, matches ) ) {
			std::string id( val.substr( matches[1].start, matches[1].end - matches[1].start ) );
			auto solvedIdIt = solvedInputs.find( id );
			if ( solvedIdIt != solvedInputs.end() )
				String::replaceAll( val, String::format( "${input:%s}", id ), solvedIdIt->second );
		}

		LuaPattern::Range matches2[2];
		if ( commandPtrn.matches( val, matches2 ) ) {
			std::string name(
				val.substr( matches2[0].start, matches2[0].end - matches2[0].start ) );
			std::string id( val.substr( matches2[1].start, matches2[1].end - matches2[1].start ) );
			String::toLowerInPlace( id );
			auto solvedIdIt = solvedInputs.find( id );
			if ( solvedIdIt != solvedInputs.end() ) {
				if ( id == "pickprocess" ) {
					int pid = 0;
					if ( String::fromString( pid, solvedIdIt->second ) ) {
						j = pid;
						return false;
					}
				} else {
					String::replaceAll( val, name, solvedIdIt->second );
				}
			}
		}

		return true;
	};

	std::vector<std::string> keysToRemove;

	for ( auto& j : json ) {
		if ( j.is_object() ) {
			replaceKeysInJson( j, randomPort, solvedInputs );
		} else if ( j.is_array() ) {
			for ( auto& e : j ) {
				if ( e.is_string() ) {
					std::string val( e.get<std::string>() );
					replaceVal( e, val );
					e = std::move( val );
				}
			}
		} else if ( j.is_string() ) {
			std::string val( j.get<std::string>() );

			if ( runConfig && val == KEY_ARGS ) {
				auto argsArr = nlohmann::json::array();
				auto args = Process::parseArgs( runConfig->args );
				for ( auto arg : args ) {
					replaceVal( j, arg );
					argsArr.push_back( arg );
				}
				j = std::move( argsArr );
				continue;
			} else if ( runConfig && val == KEY_ENV && buildConfig ) {
				if ( !buildConfig->envs().empty() ) {
					j = nlohmann::json::object();
					for ( const auto& env : buildConfig->envs() )
						j[env.first] = env.second;
				} else {
					keysToRemove.push_back( "env" );
				}
			} else if ( val == KEY_STOPONENTRY ) {
				j = false;
			} else {
				bool containsPid = false;
				if ( ( val == KEY_PID || ( containsPid = String::icontains( val, KEY_PID ) ) ) &&
					 json.contains( "program" ) && json["program"].is_string() ) {
					auto programName(
						FileSystem::fileNameFromPath( json["program"].get<std::string>() ) );
					auto pids = Sys::pidof( programName );
					if ( !pids.empty() ) {
						if ( containsPid ) {
							String::replaceAll( val, KEY_PID, String::toString( pids[0] ) );
						} else {
							j = pids[0];
							continue;
						}
					}
				}
				if ( replaceVal( j, val ) )
					j = std::move( val );
			}
		}
	}

	for ( const auto& key : keysToRemove ) {
		json.erase( key );
	}
}

std::unordered_map<std::string, DapConfigurationInput>
DebuggerPlugin::needsToResolveInputs( nlohmann::json& json,
									  const std::vector<std::string>& cmdArgs ) {
	std::unordered_map<std::string, DapConfigurationInput> inputs;
	thread_local int recursionDepth = 0;

	const auto matchString = [this, &inputs]( const std::string& val ) {
		static LuaPattern ptrn( INPUT_PATTERN );
		PatternMatcher::Range matches[4];
		if ( ptrn.matches( val, matches ) ) {
			std::string id( val.substr( matches[1].start, matches[1].end - matches[1].start ) );
			auto it = mDapInputs.find( id );
			if ( it != mDapInputs.end() )
				inputs[it->first] = it->second;
		} else if ( String::icontains( val, CMD_PICK_PROCESS ) ) {
			DapConfigurationInput dci{
				"pickprocess", i18n( "process_id", "Process ID" ), "pickprocess", "", {} };
			inputs[std::string{ CMD_PICK_PROCESS }] = dci;
		} else if ( String::icontains( val, CMD_PROMPT_STRING ) ) {
			DapConfigurationInput dci{
				"promptstring", i18n( "name", "Name" ), "promptstring", "", {} };
			inputs[std::string{ CMD_PROMPT_STRING }] = dci;
		} else if ( String::icontains( val, CMD_PICK_FILE ) ) {
			DapConfigurationInput dci{
				"pickfile", i18n( "file_path", "File Path" ), "pickfile", "", {} };
			inputs[std::string{ CMD_PICK_FILE }] = dci;
		}
	};

	recursionDepth++;
	for ( auto& j : json ) {
		if ( j.is_object() ) {
			auto rinputs = needsToResolveInputs( j, cmdArgs );
			for ( auto& i : rinputs )
				inputs[std::move( i.first )] = std::move( i.second );
		} else if ( j.is_array() ) {
			for ( auto& e : j )
				if ( e.is_string() )
					matchString( e );
		} else if ( j.is_string() ) {
			matchString( j );
		}
	}
	recursionDepth--;

	if ( recursionDepth == 0 ) {
		for ( const auto& arg : cmdArgs )
			matchString( arg );
	}

	return inputs;
}

template <typename TCommandRegister, typename Cmd, typename CmdCb>
void DebuggerPlugin::registerCommand( TCommandRegister* doc, Cmd cmd, CmdCb cb ) {
	doc->setCommand( cmd, cb );
	mRegisteredCommands.push_back( cmd );
}

template <typename TCommandRegister>
void DebuggerPlugin::registerCommands( TCommandRegister* executer ) {
	registerCommand( executer, "debugger-continue-interrupt", [this] {
		if ( mDebugger && mListener ) {
			if ( mListener->isStopped() ) {
				resume( mListener->getCurrentThreadId() );
			} else {
				mDebugger->pause( 1 );
			}
		} else {
			auto pbm = getPluginContext()->getProjectBuildManager();
			if ( !pbm )
				return;
			if ( !pbm->hasBuildConfigWithBuildSteps() ) {
				runCurrentConfig();
				return;
			}
			pbm->buildCurrentConfig(
				getPluginContext()->getStatusBuildOutputController(), [this]( int exitCode ) {
					if ( exitCode == 0 ) {
						runCurrentConfig();
					} else {
						getPluginContext()->getUISceneNode()->runOnMainThread( [this] {
							auto msgBox = UIMessageBox::New(
								UIMessageBox::YES_NO,
								i18n( "build_failed_debug_anyways",
									  "Building the project failed, do you want to "
									  "debug the binary anyways?" ) );
							msgBox->setTitle( i18n( "build_failed", "Build Failed" ) );
							msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
							msgBox->on( Event::OnConfirm, [this]( auto ) { runCurrentConfig(); } );
							msgBox->showWhenReady();
						} );
					}
				} );
		}
	} );

	registerCommand( executer, "debugger-start", [this] {
		if ( mDebugger )
			exitDebugger( true );
		runCurrentConfig();
	} );

	registerCommand( executer, "debugger-stop", [this] { exitDebugger( true ); } );

	registerCommand( executer, "debugger-start-stop", [this] {
		if ( mDebugger && mDebugger->started() ) {
			exitDebugger( true );
		} else {
			runCurrentConfig();
		}
	} );

	registerCommand( executer, "debugger-step-over", [this] {
		if ( mDebugger && mListener && mListener->isStopped() )
			mDebugger->stepOver( mListener->getCurrentThreadId() );
	} );

	registerCommand( executer, "debugger-step-into", [this] {
		if ( mDebugger && mListener && mListener->isStopped() )
			mDebugger->stepInto( mListener->getCurrentThreadId() );
	} );

	registerCommand( executer, "debugger-step-out", [this] {
		if ( mDebugger && mListener && mListener->isStopped() )
			mDebugger->stepOut( mListener->getCurrentThreadId() );
	} );

	registerCommand( executer, "toggle-status-app-debugger", [this] {
		if ( getStatusDebuggerController() )
			getStatusDebuggerController()->toggle();
	} );

	registerCommand( executer, "show-debugger-tab", [this] {
		if ( mTab )
			mTab->setTabSelected();
	} );
}

void DebuggerPlugin::onRegisterDocument( TextDocument* doc ) {
	registerCommands( doc );

	doc->setCommand( "debugger-breakpoint-toggle", [doc, this] {
		if ( setBreakpoint( doc, doc->getSelection().start().line() + 1 ) )
			getUISceneNode()->getRoot()->invalidateDraw();
	} );

	doc->setCommand( "debugger-breakpoint-enable-toggle", [this, doc] {
		if ( breakpointToggleEnabled( doc, doc->getSelection().start().line() + 1 ) )
			getUISceneNode()->getRoot()->invalidateDraw();
	} );

	Lock l( mClientsMutex );
	mClients[doc] = std::make_unique<DebuggerPluginClient>( this, doc );
	doc->registerClient( mClients[doc].get() );
}

void DebuggerPlugin::onUnregisterDocument( TextDocument* doc ) {
	Lock l( mClientsMutex );
	doc->unregisterClient( mClients[doc].get() );
	mClients.erase( doc );
}

void DebuggerPlugin::onRegisterEditor( UICodeEditor* editor ) {
	editor->registerGutterSpace( this, PixelDensity::dpToPx( 8 ), 0 );

	editor->addUnlockedCommands( DebuggerCommandList );

	PluginBase::onRegisterEditor( editor );
}

void DebuggerPlugin::onUnregisterEditor( UICodeEditor* editor ) {
	editor->removeUnlockedCommands( DebuggerCommandList );

	editor->unregisterGutterSpace( this );

	editor->removeActionsByTag( getMouseMoveHash( editor ) );
}

void DebuggerPlugin::drawLineNumbersBefore( UICodeEditor* editor,
											const DocumentLineRange& lineRange,
											const Vector2f& startScroll,
											const Vector2f& screenStart, const Float& lineHeight,
											const Float&, const int&, const Float& ) {
	Primitives p;
	Float gutterSpace = editor->getGutterSpace( this );
	Float radius = lineHeight * 0.5f;
	Float lineOffset = editor->getLineOffset();
	Float offset = editor->getGutterLocalStartOffset( this );

	p.setColor( Color( editor->getLineNumberBackgroundColor() ).blendAlpha( editor->getAlpha() ) );
	p.drawRectangle( Rectf( { screenStart.x - editor->getPluginsGutterSpace() + offset,
							  screenStart.y + editor->getPluginsTopSpace() },
							Sizef( gutterSpace, editor->getPixelsSize().getHeight() -
													editor->getPluginsTopSpace() ) ) );

	if ( editor->getDocument().hasFilepath() ) {
		auto docIt = mBreakpoints.find( editor->getDocument().getFilePath() );
		if ( docIt != mBreakpoints.end() && !docIt->second.empty() ) {
			const auto& breakpoints = docIt->second;

			p.setColor( Color( editor->getColorScheme().getEditorColor( SyntaxStyleTypes::Error ) )
							.blendAlpha( editor->getAlpha() ) );

			for ( const SourceBreakpointStateful& breakpoint : breakpoints ) {
				int line = breakpoint.line - 1; // Breakpoints start at 1
				if ( line >= 0 && line >= lineRange.first && line <= lineRange.second ) {
					if ( !editor->getDocumentView().isLineVisible( line ) )
						continue;

					Vector2f lnPos(
						screenStart.x - editor->getPluginsGutterSpace() + offset,
						startScroll.y +
							editor->getDocumentView().getLineYOffset( line, lineHeight ) +
							lineOffset );

					Color color( Color( editor->getColorScheme().getEditorColor(
											breakpoint.enabled ? SyntaxStyleTypes::Error
															   : SyntaxStyleTypes::LineNumber2 ) )
									 .blendAlpha( editor->getAlpha() ) );

					static UIIcon* circleFilled = getUISceneNode()->findIcon( "circle-perfect" );

					if ( circleFilled ) {
						Float finalHeight = eefloor( radius * 1.75f );
						Drawable* drawable = circleFilled->getSize( finalHeight );
						if ( drawable ) {
							Color oldColor = drawable->getColor();
							drawable->setColor( color );
							drawable->draw(
								Sizef{ lnPos.x, lnPos.y + ( lineHeight - finalHeight ) * 0.5f }
									.floor() );
							drawable->setColor( oldColor );
						}
					} else {
						p.setColor( color );

						p.drawCircle( Sizef{ lnPos.x + radius + ( gutterSpace - radius ) * 0.5f,
											 lnPos.y + lineHeight * 0.5f }
										  .floor(),
									  radius );
					}
				}
			}
		}
	}

	if ( mDebugger && mListener && mListener->isStopped() &&
		 mListener->isCurrentScopePos( editor->getDocument().getFilePath() ) ) {
		int line = mListener->getCurrentScopePosLine() - 1;
		if ( line >= 0 && line >= lineRange.first && line <= lineRange.second &&
			 editor->getDocumentView().isLineVisible( line ) ) {

			Color color(
				Color( editor->getColorScheme().getEditorColor( SyntaxStyleTypes::Warning ) )
					.blendAlpha( editor->getAlpha() ) );

			Vector2f lnPos( screenStart.x - editor->getPluginsGutterSpace() + offset,
							startScroll.y +
								editor->getDocumentView().getLineYOffset( line, lineHeight ) +
								lineOffset );

			Float dim = radius * 2;
			Float gutterSpace = editor->getGutterSpace( this );

			static UIIcon* sfIcon = getUISceneNode()->findIcon( "debug-stackframe" );
			if ( sfIcon ) {
				Drawable* drawable = sfIcon->getSize( lineHeight );
				if ( drawable ) {
					drawable->setColor( color );
					drawable->draw( lnPos.floor() );
					return;
				}
			}

			lnPos.x += editor->getLineNumberPaddingLeft() + ( gutterSpace - dim ) * 0.5f;
			lnPos.y += ( lineHeight - dim ) * 0.5f;
			p.setColor( color );

			Triangle2f tri;
			tri.V[0] = lnPos + Vector2f{ 0, 0 };
			tri.V[1] = lnPos + Vector2f{ 0, dim };
			tri.V[2] = lnPos + Vector2f{ dim, dim * 0.5f };
			p.drawTriangle( tri );
		}
	}
}

void DebuggerPlugin::drawBeforeLineText( UICodeEditor* editor, const Int64& index,
										 Vector2f position, const Float& /*fontSize*/,
										 const Float& lineHeight ) {
	if ( !mDebugger || !mListener || !mListener->isStopped() ||
		 !mListener->isCurrentScopePos( editor->getDocument().getFilePath(), index + 1 ) ||
		 !editor->getDocumentView().isLineVisible( index ) )
		return;

	Primitives p;
	Color color( editor->getColorScheme().getEditorSyntaxStyle( "warning"_sst ).color );
	Color blendedColor( Color( color, 20 ).blendAlpha( editor->getAlpha() ) );
	p.setColor( blendedColor );
	p.drawRectangle(
		Rectf( position, Sizef( editor->getViewportWidth( false, true ), lineHeight ) ) );
}

bool DebuggerPlugin::setBreakpoint( const std::string& doc, Uint32 lineNumber ) {
	Lock l( mBreakpointsMutex );

	initStatusDebuggerController();

	if ( !mBreakpointsModel )
		mBreakpointsModel = std::make_shared<BreakpointsModel>( mBreakpoints, getUISceneNode() );

	auto sdc = getStatusDebuggerController();
	if ( sdc && sdc->getUIBreakpoints()->getModel() == nullptr )
		sdc->getUIBreakpoints()->setModel( mBreakpointsModel );

	auto& breakpoints = mBreakpoints[doc];
	auto breakpointIt = breakpoints.find( SourceBreakpointStateful( lineNumber ) );
	if ( breakpointIt != breakpoints.end() ) {
		breakpoints.erase( breakpointIt );
		mBreakpointsModel->erase( doc, lineNumber );
	} else {
		breakpoints.insert( SourceBreakpointStateful( lineNumber ) );
		mBreakpointsModel->insert( doc, lineNumber );
	}

	mThreadPool->run( [this, doc] { sendFileBreakpoints( doc ); } );

	getUISceneNode()->getRoot()->invalidateDraw();

	return true;
}

bool DebuggerPlugin::setBreakpoint( TextDocument* doc, Uint32 lineNumber ) {
	if ( !doc->hasFilepath() )
		return false;
	if ( !isSupportedByAnyDebugger( doc->getSyntaxDefinition().getLSPName() ) )
		return false;
	return setBreakpoint( doc->getFilePath(), lineNumber );
}

bool DebuggerPlugin::setBreakpoint( UICodeEditor* editor, Uint32 lineNumber ) {
	if ( setBreakpoint( &editor->getDocument(), lineNumber ) )
		editor->invalidateDraw();
	return true;
}

bool DebuggerPlugin::breakpointSetEnabled( const std::string& doc, Uint32 lineNumber,
										   bool enabled ) {
	if ( mChangingBreakpoint )
		return false;
	BoolScopedOp changing( mChangingBreakpoint, true );
	Lock l( mBreakpointsMutex );
	auto& breakpoints = mBreakpoints[doc];
	SourceBreakpointStateful sb( lineNumber );
	auto breakpointIt = breakpoints.find( sb );
	if ( breakpointIt != breakpoints.end() ) {
		if ( enabled != breakpointIt->enabled ) {
			breakpointIt->enabled = enabled;
			mBreakpointsModel->enable( doc, lineNumber, enabled );
			mThreadPool->run( [this, doc] { sendFileBreakpoints( doc ); } );
			getUISceneNode()->getRoot()->invalidateDraw();
		}
		return true;
	}
	return false;
}

bool DebuggerPlugin::breakpointToggleEnabled( const std::string& doc, Uint32 lineNumber ) {
	Lock l( mBreakpointsMutex );
	auto& breakpoints = mBreakpoints[doc];
	auto breakpointIt = breakpoints.find( SourceBreakpointStateful( lineNumber ) );
	if ( breakpointIt != breakpoints.end() )
		return breakpointSetEnabled( doc, lineNumber, !breakpointIt->enabled );
	return false;
}

bool DebuggerPlugin::breakpointToggleEnabled( TextDocument* doc, Uint32 lineNumber ) {
	if ( !doc->hasFilepath() )
		return false;
	if ( !isSupportedByAnyDebugger( doc->getSyntaxDefinition().getLSPName() ) )
		return false;
	return breakpointToggleEnabled( doc->getFilePath(), lineNumber );
}

bool DebuggerPlugin::hasBreakpoint( const std::string& doc, Uint32 lineNumber ) {
	Lock l( mBreakpointsMutex );
	auto& breakpoints = mBreakpoints[doc];
	auto breakpointIt = breakpoints.find( SourceBreakpointStateful( lineNumber ) );
	return breakpointIt != breakpoints.end();
}

bool DebuggerPlugin::onMouseDown( UICodeEditor* editor, const Vector2i& position,
								  const Uint32& flags ) {
	if ( !( flags & ( EE_BUTTON_LMASK | EE_BUTTON_RMASK ) ) )
		return false;
	Float offset = editor->getGutterLocalStartOffset( this );
	Vector2f localPos( editor->convertToNodeSpace( position.asFloat() ) );
	if ( localPos.x >= editor->getPixelsPadding().Left + offset &&
		 localPos.x < editor->getPixelsPadding().Left + offset + editor->getGutterSpace( this ) +
						  editor->getLineNumberPaddingLeft() &&
		 localPos.y > editor->getPluginsTopSpace() ) {
		if ( editor->getUISceneNode()->getEventDispatcher()->isFirstPress() ) {
			auto cursorPos( editor->resolveScreenPosition( position.asFloat() ) );
			if ( ( flags & EE_BUTTON_RMASK ) &&
				 hasBreakpoint( editor->getDocument().getFilePath(), cursorPos.line() + 1 ) ) {
				breakpointToggleEnabled( &editor->getDocument(), cursorPos.line() + 1 );
			} else {
				setBreakpoint( editor, cursorPos.line() + 1 );
			}
		}
		return true;
	}
	return false;
}

bool DebuggerPlugin::isSupportedByAnyDebugger( const std::string& language ) {
	for ( const auto& dap : mDaps ) {
		if ( std::any_of( dap.languagesSupported.begin(), dap.languagesSupported.end(),
						  [&language]( const auto& l ) { return l == language; } ) )
			return true;
	}
	return false;
}

static std::string findCommand( const std::string& findBinary, const std::string& cmd ) {
	std::string findCmd = findBinary;
	String::replaceAll( findCmd, "${command}", cmd );
	Process p;
	if ( p.create( findCmd ) ) {
		std::string path;
		p.readAllStdOut( path, Seconds( 5 ) );
		int retCode = -1;
		p.join( &retCode );
		if ( retCode == 0 && !path.empty() ) {
			String::trimInPlace( path, '\n' );
			return path;
		}
	}
	return "";
}

void DebuggerPlugin::runConfig( const std::string& debugger, const std::string& configuration ) {
	auto debuggerIt = std::find_if( mDaps.begin(), mDaps.end(), [&debugger]( const DapTool& dap ) {
		return dap.name == debugger;
	} );

	if ( debuggerIt == mDaps.end() ) {
		return;
	}

	auto configIt = std::find_if(
		debuggerIt->configurations.begin(), debuggerIt->configurations.end(),
		[&configuration]( const DapConfig& conf ) { return conf.name == configuration; } );

	bool usingExternalConfig = false;
	if ( configIt == debuggerIt->configurations.end() ) {
		configIt = std::find_if(
			mDapConfigs.begin(), mDapConfigs.end(),
			[&configuration]( const DapConfig& conf ) { return conf.name == configuration; } );

		if ( configIt == mDapConfigs.end() )
			return;

		usingExternalConfig = true;
	}

	auto runConfig = debuggerIt->run;

	if ( !usingExternalConfig && configIt->request == REQUEST_TYPE_LAUNCH &&
		 getPluginContext()->getProjectBuildManager() &&
		 !getPluginContext()->getProjectBuildManager()->getCurrentRunConfig() ) {
		auto msg =
			i18n( "no_run_config",
				  "You must first have a \"Run Target\" configured and selected. Go to \"Build "
				  "Settings\" and create a new build and run setting (build step is optional)." );
		mManager->getPluginContext()->getNotificationCenter()->addNotification( msg, Seconds( 5 ) );
		return;
	}

	auto inputs = needsToResolveInputs( configIt->args, configIt->cmdArgs );
	if ( !inputs.empty() ) {
		resolveInputsBeforeRun( inputs, *debuggerIt, *configIt );
		return;
	}

	prepareAndRun( *debuggerIt, *configIt, {} );
}

void DebuggerPlugin::resolveInputsBeforeRun(
	std::unordered_map<std::string, DapConfigurationInput> inputs, DapTool debugger,
	DapConfig config, std::unordered_map<std::string, std::string> solvedInputs ) {
	if ( inputs.empty() ) {
		prepareAndRun( debugger, config, solvedInputs );
		return;
	}

	auto input = inputs.begin()->second;
	if ( input.type == "pickprocess"sv ) {
		UIWindow* win = processPicker();
		win->setTitle( i18n( "pick_process", "Pick Process" ) );
		win->center();
		win->showWhenReady();
		win->on( Event::OnConfirm, [inputs, win, debugger, config, solvedInputs,
									this]( const Event* ) mutable {
			UITableView* uiTableView = win->find( "processes_list" )->asType<UITableView>();
			auto model = static_cast<ProcessesModel*>( uiTableView->getModel() );
			std::string inputData(
				model->data( uiTableView->getSelection().first(), ModelRole::Display ).toString() );
			std::string id( inputs.begin()->second.id );
			solvedInputs[id] = inputData;
			inputs.erase( inputs.begin() );
			resolveInputsBeforeRun( inputs, debugger, config, solvedInputs );
			win->closeWindow();
		} );
	} else if ( input.type == "pickfile" ) {
		UIFileDialog* dialog = UIFileDialog::New(
			UIFileDialog::DefaultFlags | ( getPluginContext()->getConfig().ui.nativeFileDialogs
											   ? UIFileDialog::UseNativeFileDialog
											   : 0 ),
			"*", getPluginContext()->getDefaultFileDialogFolder() );
		dialog->setWindowFlags( UI_WIN_DEFAULT_FLAGS | UI_WIN_MAXIMIZE_BUTTON | UI_WIN_MODAL );
		dialog->setTitle( i18n( "open_file", "Open File" ) );
		dialog->setCloseShortcut( KEY_ESCAPE );
		dialog->setSingleClickNavigation(
			getPluginContext()->getConfig().editor.singleClickNavigation );
		dialog->setAllowsMultiFileSelect( false );
		dialog->on( Event::OpenFile, [this, dialog, inputs, solvedInputs, debugger,
									  config]( const Event* event ) mutable {
			auto file = event->getNode()->asType<UIFileDialog>()->getFullPath();
			solvedInputs[inputs.begin()->second.id] = file;
			inputs.erase( inputs.begin() );
			resolveInputsBeforeRun( inputs, debugger, config, solvedInputs );
			dialog->closeWindow();
		} );
		dialog->on( Event::OnWindowClose, [this]( const Event* ) {
			if ( getPluginContext()->getSplitter() &&
				 getPluginContext()->getSplitter()->getCurWidget() &&
				 !SceneManager::instance()->isShuttingDown() )
				getPluginContext()->getSplitter()->getCurWidget()->setFocus();
		} );
		dialog->center();
		dialog->show();
	} else {
		bool isPick = input.type == "pickstring";
		UIMessageBox* msgBox = UIMessageBox::New(
			isPick ? UIMessageBox::DROPDOWNLIST : UIMessageBox::INPUT, input.description );
		msgBox->setTitle( input.id );
		msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
		msgBox->center();
		if ( isPick ) {
			auto listBox = msgBox->getDropDownList()->getListBox();
			std::vector<String> ioptions;
			ioptions.reserve( input.options.size() );
			for ( const auto& option : input.options )
				ioptions.push_back( option );
			listBox->addListBoxItems( ioptions );
		}
		msgBox->showWhenReady();
		msgBox->on( Event::OnConfirm, [inputs, msgBox, isPick, debugger, config, solvedInputs,
									   this]( const Event* ) mutable {
			std::string inputData( isPick ? msgBox->getDropDownList()->getText().toUtf8()
										  : msgBox->getTextInput()->getText().toUtf8() );
			solvedInputs[inputs.begin()->second.id] = inputData;
			inputs.erase( inputs.begin() );
			resolveInputsBeforeRun( inputs, debugger, config, solvedInputs );
			msgBox->closeWindow();
		} );
	}
}

std::optional<Connection> getDebugServer( nlohmann::json& json ) {
	auto debugServerInt = json_get_if<int>( json, KEY_DEBUG_SERVER );

	if ( debugServerInt )
		return Connection{ *debugServerInt, "localhost" };

	auto debugServerStr = json_get_if<std::string>( json, KEY_DEBUG_SERVER );
	if ( debugServerStr ) {
		int port;
		if ( String::isNumber( *debugServerStr ) && String::fromString( port, *debugServerStr ) )
			return Connection( port, "localhost" );

		auto split = String::split( *debugServerStr, ':' );
		if ( split.size() == 2 && String::fromString( port, split[1] ) )
			return Connection( port, split[1] );
	}

	return {};
}

void DebuggerPlugin::prepareAndRun( DapTool debugger, DapConfig config,
									std::unordered_map<std::string, std::string> solvedInputs ) {
	int randomPort = Math::randi( 44000, 45000 );
	ProtocolSettings protocolSettings;
	protocolSettings.launchRequestType = config.request;
	protocolSettings.runTarget = config.runTarget;
	auto args = config.args;
	replaceKeysInJson( args, randomPort, solvedInputs );
	protocolSettings.launchArgs = std::move( args );
	protocolSettings.redirectStdout = debugger.redirectStdout;
	protocolSettings.redirectStderr = debugger.redirectStderr;
	protocolSettings.supportsSourceRequest = debugger.supportsSourceRequest;
	bool forceUseProgram = false;
	bool usesPorts = false;

	for ( auto& arg : debugger.run.args ) {
		auto ret( replaceKeyInString( arg, randomPort, solvedInputs ) );

		// No success? Abort
		if ( !ret.first )
			return;

		auto res( ret.second );
		if ( res.size() == 1 && res[0] != arg ) {
			if ( String::contains( arg, KEY_RANDPORT ) )
				usesPorts = true;
			arg = res[0];
		}
	}

	for ( const std::string& cmdArg : config.cmdArgs ) {
		if ( cmdArg == KEY_FILE || cmdArg == KEY_ARGS || cmdArg == CMD_PICK_PROCESS ||
			 cmdArg == CMD_PROMPT_STRING )
			forceUseProgram = true;
		auto ret = replaceKeyInString( cmdArg, randomPort, solvedInputs );

		// No success? Abort
		if ( !ret.first )
			return;

		auto args = ret.second;
		for ( const auto& arg : args )
			debugger.run.args.emplace_back( arg );
	}

	if ( getDebugServer( protocolSettings.launchArgs ) )
		usesPorts = true;

	mThreadPool->run(
		[this, protocolSettings = std::move( protocolSettings ), randomPort,
		 debugger = std::move( debugger ), forceUseProgram, usesPorts]() mutable {
			run( debugger.name, std::move( protocolSettings ), std::move( debugger.run ),
				 randomPort, forceUseProgram, usesPorts, debugger.unstableFrameId );
		},
		[this]( const Uint64& ) {
			if ( !mDebugger || !mDebugger->started() ) {
				exitDebugger();

				getManager()->getPluginContext()->getNotificationCenter()->addNotification(
					i18n( "debugger_init_failed", "Failed to initialize debugger." ) );
			}
		} );
}

UIWindow* DebuggerPlugin::processPicker() {
	static constexpr auto PROCESS_PICKER_LAYOUT = R"html(
<window id="process_picker" lw="450dp" lh="450dp" padding="4dp" window-title="@string(list_of_processes, List of Processes)">
	<vbox lw="mp" lh="mp">
		<hbox lw="mp" lh="wc" margin-bottom="4dp">
			<TextView text="@string(filter_colon, Filter:)" lh="mp" margin-right="8dp" />
			<TextInput id="processes_filter" lw="0dp" lw8="1" />
		</hbox>
		<TableView id="processes_list" lw="mp" lh="0dp" lw8="1" />
		<hbox class="buttons" lw="mp" lh="wc" margin-top="4dp">
			<PushButton id="pick_process" text="@string(pick_process, Pick Process)" enabled="false" />
			<PushButton id="update_process_list" text="@string(update_list, Update List)" margin-left="4dp" />
			<Widget lw="0dp" lw8="1" />
			<PushButton id="cancel_pick" text="@string(cancel, Cancel)" />
		</hbox>
	</vbox>
</window>
	)html";
	UIWindow* win =
		getUISceneNode()->loadLayoutFromString( PROCESS_PICKER_LAYOUT )->asType<UIWindow>();
	UITextInput* uiFilter = win->find( "processes_filter" )->asType<UITextInput>();
	UITableView* uiTableView = win->find( "processes_list" )->asType<UITableView>();
	UIPushButton* uiPickBut = win->find( "pick_process" )->asType<UIPushButton>();
	UIPushButton* uiUpdateList = win->find( "update_process_list" )->asType<UIPushButton>();
	UIPushButton* uiCancel = win->find( "cancel_pick" )->asType<UIPushButton>();
	auto model = std::make_shared<ProcessesModel>( std::vector<std::pair<Uint64, std::string>>{},
												   getUISceneNode() );
	uiTableView->setAutoColumnsWidth( true );
	uiTableView->setFitAllColumnsToWidget( true );
	uiTableView->setMainColumn( 1 );
	uiTableView->setModel( model );
	uiTableView->onModelEvent( [win]( const ModelEvent* event ) {
		if ( event->getModelEventType() == ModelEventType::Open )
			win->sendCommonEvent( Event::OnConfirm );
	} );
	uiTableView->setOnSelectionChange( [uiTableView, uiPickBut]() {
		uiPickBut->setEnabled( !uiTableView->getSelection().isEmpty() );
	} );
	uiFilter->on( Event::OnValueChange, [uiFilter, uiTableView, model]( auto ) {
		model->setFilter( uiFilter->getText() );
		uiTableView->setSelection( model->index( 0 ) );
	} );
	uiFilter->on( Event::OnPressEnter, [uiTableView, win]( auto ) {
		if ( !uiTableView->getSelection().isEmpty() )
			win->sendCommonEvent( Event::OnConfirm );
	} );
	uiUpdateList->onClick( [this, model]( auto ) {
		mThreadPool->run( [model] { model->setProcesses( Sys::listProcesses() ); } );
	} );
	mThreadPool->run( [model, uiTableView] {
		model->setProcesses( Sys::listProcesses() );
		uiTableView->scrollToBottom();
	} );
	uiCancel->onClick( [win]( auto ) { win->closeWindow(); } );
	uiPickBut->onClick( [win]( auto ) { win->sendCommonEvent( Event::OnConfirm ); } );
	win->on( Event::OnWindowReady, [uiFilter]( auto ) { uiFilter->setFocus(); } );
	win->setKeyBindingCommand( "close-window", [win]() { win->closeWindow(); } );
	win->addKeyBinding( { KEY_ESCAPE }, "close-window" );
	return win;
}

bool DebuggerPlugin::resume( int threadId, bool singleThread ) {
	mHoverExpressionsHolder->clear( true );

	if ( mHoverTooltip && mHoverTooltip->isVisible() )
		mHoverTooltip->hide();

	if ( !mDebugger )
		return false;

	return mDebugger->resume( threadId, singleThread );
}

std::optional<Command>
DebuggerPlugin::debuggerBinaryExists( const std::string& debugger,
									  std::optional<DapRunConfig> optRunConfig ) {
	auto debuggerIt = std::find_if( mDaps.begin(), mDaps.end(), [&debugger]( const DapTool& dap ) {
		return dap.name == debugger;
	} );

	if ( debuggerIt == mDaps.end() )
		return {};

	DapRunConfig runConfig = optRunConfig ? *optRunConfig : debuggerIt->run;
	Command cmd;
	cmd.command = std::move( runConfig.command );
	cmd.arguments = std::move( runConfig.args );

	std::string findBinary;
	auto findBinaryIt = debuggerIt->findBinary.find( String::toLower( Sys::getPlatform() ) );
	if ( findBinaryIt != debuggerIt->findBinary.end() )
		findBinary = findBinaryIt->second;

	std::string fallbackCommand = debuggerIt->fallbackCommand;

	if ( !findBinary.empty() && Sys::which( cmd.command ).empty() ) {
		auto foundCmd = findCommand( findBinary, cmd.command );
		if ( !foundCmd.empty() ) {
			cmd.command = std::move( foundCmd );
		} else if ( !fallbackCommand.empty() ) {
			foundCmd = findCommand( findBinary, fallbackCommand );
			if ( !foundCmd.empty() )
				cmd.command = std::move( foundCmd );
		}
	}

	if ( !cmd.command.empty() && !FileSystem::fileExists( cmd.command ) &&
		 Sys::which( cmd.command ).empty() ) {
		auto args = Process::parseArgs( cmd.command );
		if ( args.empty() ||
			 ( !FileSystem::fileExists( args[0] ) && Sys::which( args[0] ).empty() ) ) {
			if ( fallbackCommand.empty() || ( !FileSystem::fileExists( fallbackCommand ) &&
											  Sys::which( fallbackCommand ).empty() ) ) {
				return {};
			} else {
				cmd.command = std::move( fallbackCommand );
			}
		}
	}

	return cmd;
}

void DebuggerPlugin::initStatusDebuggerController() {
	auto sdc = getStatusDebuggerController();
	if ( sdc && sdc->getWidget() == nullptr ) {
		sdc->show();
		sdc->hide();
	}
}

void DebuggerPlugin::run( const std::string& debugger, ProtocolSettings&& protocolSettings,
						  DapRunConfig&& runConfig, int randPort, bool forceUseProgram,
						  bool usesPorts, bool unstableFrameId ) {
	std::optional<Command> cmdOpt = debuggerBinaryExists( debugger, runConfig );

	if ( !cmdOpt && ( protocolSettings.launchRequestType == REQUEST_TYPE_LAUNCH ||
					  ( protocolSettings.launchRequestType == REQUEST_TYPE_ATTACH &&
						protocolSettings.launchArgs.value( "mode", "" ) == "local" &&
						protocolSettings.launchArgs.contains( "program" ) ) ) ) {
		auto msg =
			String::format( i18n( "debugger_binary_not_found",
								  "Debugger binary not found. Binary \"%s\" must be installed." )
								.toUtf8(),
							runConfig.command );

		mManager->getPluginContext()->getNotificationCenter()->addNotification( msg );
		return;
	}

	Command cmd = std::move( *cmdOpt );
	bool isRemote = false;
	bool runsDapServer = false;

	for ( auto& arg : cmd.arguments )
		replaceExecutableArgs( arg );

	Connection con;
	con.host = protocolSettings.launchArgs.value( "host", "localhost" );
	con.port = randPort;

	auto debugServer = getDebugServer( protocolSettings.launchArgs );
	if ( debugServer ) {
		runsDapServer = true;
		con.host = debugServer->host;
		con.port = debugServer->port;
	} else if ( protocolSettings.launchArgs.contains( "port" ) &&
				protocolSettings.launchArgs["port"].is_number_integer() ) {
		con.port = protocolSettings.launchArgs.value( "port", randPort );
	}

	std::string localRoot = mProjectPath;
	std::string remoteRoot;

	if ( protocolSettings.launchRequestType == REQUEST_TYPE_LAUNCH ) {
		if ( usesPorts ) {
			auto bus = std::make_unique<BusSocketProcess>( cmd, con );
			mDebugger = std::make_unique<DebuggerClientDap>( protocolSettings, std::move( bus ) );
		} else {
			auto bus = std::make_unique<BusProcess>( cmd );
			mDebugger = std::make_unique<DebuggerClientDap>( protocolSettings, std::move( bus ) );
		}
	} else if ( protocolSettings.launchRequestType == REQUEST_TYPE_ATTACH ) {
		auto mode = protocolSettings.launchArgs.value( "mode", "" );
		if ( mode.empty() )
			mode = "local";

		bool useSocket = !con.host.empty() && con.port != 0;
		if ( ( protocolSettings.launchArgs.contains( "host" ) ||
			   protocolSettings.launchArgs.contains( "port" ) ) &&
			 !useSocket ) {
			getManager()->getPluginContext()->getNotificationCenter()->addNotification(
				i18n( "host_port_required", "No host or port has been specified." ) );
			return;
		}

		if ( !useSocket && forceUseProgram && usesPorts ) {
			useSocket = true;
			con.port = randPort;
		}

		bool useProcessId = protocolSettings.launchArgs.contains( "pid" ) &&
							protocolSettings.launchArgs["pid"].is_number() &&
							protocolSettings.launchArgs.value( "pid", 0 ) != 0;

		bool useProgram =
			forceUseProgram || ( protocolSettings.launchArgs.contains( "program" ) &&
								 protocolSettings.launchArgs["program"].is_string() &&
								 !protocolSettings.launchArgs.value( "program", "" ).empty() );

		if ( mode == "local" ) {
			if ( runsDapServer ) {
				if ( protocolSettings.launchArgs.contains( "remoteRoot" ) &&
					 protocolSettings.launchArgs["remoteRoot"].is_string() ) {
					isRemote = true;
					remoteRoot = protocolSettings.launchArgs["remoteRoot"].get<std::string>();
				}
				if ( protocolSettings.launchArgs.contains( "localRoot" ) &&
					 protocolSettings.launchArgs["localRoot"].is_string() ) {
					localRoot = protocolSettings.launchArgs["localRoot"].get<std::string>();
				}
				auto bus = std::make_unique<BusSocketProcess>( cmd, con );
				mDebugger =
					std::make_unique<DebuggerClientDap>( protocolSettings, std::move( bus ) );
			} else if ( useSocket ) {
				auto bus = std::make_unique<BusSocketProcess>( cmd, con );
				mDebugger =
					std::make_unique<DebuggerClientDap>( protocolSettings, std::move( bus ) );
			} else if ( useProcessId || useProgram ) {
				auto bus = std::make_unique<BusProcess>( cmd );
				mDebugger =
					std::make_unique<DebuggerClientDap>( protocolSettings, std::move( bus ) );
			} else {
				// Unsupported
			}
		} else if ( mode == "remote" ) {
			auto bus = std::make_unique<BusSocket>( con );
			mDebugger = std::make_unique<DebuggerClientDap>( protocolSettings, std::move( bus ) );
			isRemote = true;
		}
	} else {
		getManager()->getPluginContext()->getNotificationCenter()->addNotification( String::format(
			i18n( "unknown_request_type", "Unknown request type: %s" ).toUtf8().c_str(),
			cmd.command ) );
		return;
	}

	if ( !mDebugger ) {
		getManager()->getPluginContext()->getNotificationCenter()->addNotification(
			i18n( "debugger_configuration_not_supported",
				  "Debugger configuration currently not supported." ) );
		return;
	}

	mListener = std::make_unique<DebuggerClientListener>( mDebugger.get(), this );
	mListener->setIsRemote( isRemote );
	mListener->setUnstableFrameId( unstableFrameId );

	if ( isRemote ) {
		// mListener->setLocalRoot( localRoot );
		// mListener->setRemoteRoot( remoteRoot );
	}

	mDebugger->addListener( mListener.get() );
	mDebugger->setSilent( mSilence );

	DebuggerClientDap* dap = static_cast<DebuggerClientDap*>( mDebugger.get() );
	dap->runInTerminalCb = [this]( bool isIntegrated, std::string cmd,
								   const std::vector<std::string>& args, const std::string& cwd,
								   const std::unordered_map<std::string, std::string>& env,
								   std::function<void( int )> doneFn ) {
		if ( !FileSystem::fileExists( cmd ) )
			cmd = FileSystem::fileNameFromPath( cmd );
		getUISceneNode()->runOnMainThread( [this, isIntegrated, cmd = std::move( cmd ), cwd, args,
											doneFn = std::move( doneFn ), env = std::move( env )] {
			if ( isIntegrated || !env.empty() ) {
				UITerminal* term =
					getPluginContext()->getTerminalManager()->createTerminalInSplitter(
						cwd, cmd, args, env, false, false );

				doneFn( term && term->getTerm() && term->getTerm()->getTerminal() &&
								term->getTerm()->getTerminal()->getProcess()
							? term->getTerm()->getTerminal()->getProcess()->pid()
							: 0 );
			} else {
				std::string fcmd = cmd + ( !args.empty() ? " " : "" ) + String::join( args, ' ' );
				doneFn(
					getPluginContext()->getTerminalManager()->openInExternalTerminal( fcmd, cwd ) );
			}
		} );
	};

	dap->runTargetCb = [this] {
		getUISceneNode()->runOnMainThread(
			[this] { getPluginContext()->runCommand( "project-run-executable" ); } );
	};

	mDebugger->start();
}

void DebuggerPlugin::exitDebugger( bool requestDisconnect ) {
	if ( mDebugger && mListener )
		mDebugger->removeListener( mListener.get() );

	if ( requestDisconnect && mDebugger )
		mDebugger->disconnect( true, false );

	if ( mDebugger || mListener ) {
		mThreadPool->run( [this] {
			mDebugger.reset();
			mListener.reset();
		} );
	}
	setUIDebuggingState( StatusDebuggerController::State::NotStarted );
}

void DebuggerPlugin::hideSidePanel() {
	if ( mSidePanel && mTab ) {
		mSidePanel->removeTab( mTab, false );
		mTab = nullptr;
	}
}

void DebuggerPlugin::hideStatusBarElement() {
	if ( getPluginContext()->getStatusBar() ) {
		auto but = getPluginContext()->getStatusBar()->find( "status_app_debugger" );
		if ( but )
			but->setVisible( false );
	}
}

StatusDebuggerController* DebuggerPlugin::getStatusDebuggerController() const {
	auto debuggerElement =
		getPluginContext()->getStatusBar()->getStatusBarElement( "status_app_debugger" );
	return static_cast<StatusDebuggerController*>( debuggerElement.get() );
}

void DebuggerPlugin::updatePanelUIState( StatusDebuggerController::State state ) {
	auto isDebugging = state != StatusDebuggerController::State::NotStarted;

	mPanelBoxButtons.box->setVisible( isDebugging );
	mPanelBoxButtons.resume->setVisible( isDebugging )
		->setEnabled( state == StatusDebuggerController::State::Paused );
	mPanelBoxButtons.pause->setVisible( isDebugging )
		->setEnabled( state == StatusDebuggerController::State::Running );
	mPanelBoxButtons.stepOver->setVisible( isDebugging )
		->setEnabled( state == StatusDebuggerController::State::Paused );
	mPanelBoxButtons.stepInto->setVisible( isDebugging )
		->setEnabled( state == StatusDebuggerController::State::Paused );
	mPanelBoxButtons.stepOut->setVisible( isDebugging )
		->setEnabled( state == StatusDebuggerController::State::Paused );

	mRunButton->setText( isDebugging ? i18n( "Stop Debugger", "Stop Debugger" )
									 : i18n( "debug", "Debug" ) );

	mBuildAndRunButton->setEnabled( !isDebugging );
}

void DebuggerPlugin::setUIDebuggingState( StatusDebuggerController::State state ) {
	if ( isShuttingDown() )
		return;
	mDebuggingState = state;

	auto ctrl = getStatusDebuggerController();
	if ( ctrl ) {
		ctrl->setDebuggingState( state );
	}

	if ( mPanelBoxButtons.box ) {
		if ( Engine::isMainThread() )
			updatePanelUIState( state );
		else
			mPanelBoxButtons.box->runOnMainThread( [this, state] { updatePanelUIState( state ); } );
	}

	if ( state == StatusDebuggerController::State::NotStarted ||
		 state == StatusDebuggerController::State::Running ) {
		resetExpressions();
	}
}

void DebuggerPlugin::displayTooltip( UICodeEditor* editor, const std::string& expression,
									 const EvaluateInfo& info, const Vector2f& position ) {
	if ( mHoverTooltip == nullptr ) {
		UIWindow* win = UIWindow::New();
		win->setId( "debugger_hover_window" );
		win->setMinWindowSize( 400, 250 );
		win->setKeyBindingCommand( "closeWindow", [this, win, editor] {
			win->closeWindow();
			if ( getPluginContext()->getSplitter() &&
				 getPluginContext()->getSplitter()->editorExists( editor ) )
				editor->setFocus();
		} );
		win->getKeyBindings().addKeybind( { KEY_ESCAPE }, "closeWindow" );
		win->setWindowFlags( UI_WIN_CLOSE_BUTTON | UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS |
							 UI_WIN_SHADOW | UI_WIN_EPHEMERAL | UI_WIN_RESIZEABLE |
							 UI_WIN_DRAGGABLE_CONTAINER | UI_WIN_SHARE_ALPHA_WITH_CHILDREN );
		win->center();
		win->on( Event::OnWindowClose, [this]( auto ) { mHoverTooltip = nullptr; } );
		win->on( Event::OnWindowReady, [win]( auto ) { win->setFocus(); } );

		UILinearLayout* vbox = UILinearLayout::NewVertical();
		vbox->setParent( win->getContainer() );
		vbox->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::MatchParent );

		UITreeView* tv = UITreeView::New();
		tv->setId( "debugger_hover_treeview" );
		tv->setHeadersVisible( false );
		tv->setAutoColumnsWidth( true );
		tv->setFitAllColumnsToWidget( true );
		tv->setLayoutSizePolicy( SizePolicy::MatchParent, SizePolicy::Fixed );
		tv->setLayoutWeight( 1 );
		tv->setParent( vbox );
		tv->setModel( mHoverExpressionsHolder->getModel() );
		tv->setFocusOnSelection( false );

		tv->on( Event::OnModelEvent, [this]( const Event* event ) {
			if ( !mDebugger || !mListener )
				return;
			const ModelEvent* modelEvent = static_cast<const ModelEvent*>( event );
			auto idx( modelEvent->getModelIndex() );
			if ( modelEvent->getModelEventType() == Abstract::ModelEventType::OpenTree ) {
				ModelVariableNode* node =
					static_cast<ModelVariableNode*>( modelEvent->getModelIndex().internalData() );
				mDebugger->variables(
					node->var.variablesReference, Variable::Type::Both,
					[this]( const int variablesReference, std::vector<Variable>&& vars ) {
						mHoverExpressionsHolder->addVariables( variablesReference,
															   std::move( vars ) );
					} );
			} else if ( modelEvent->getModelEventType() == Abstract::ModelEventType::OpenMenu &&
						idx.isValid() ) {
				mListener->createAndShowVariableMenu( idx );
			}
		} );

		mHoverTooltip = win;
	}

	if ( editor->getTooltip() )
		editor->getTooltip()->hide();

	mHoverTooltip->find( "debugger_hover_treeview" )->asType<UITreeView>()->clearViewMetadata();
	mHoverExpressionsHolder->clear( true );

	Variable var;
	var.evaluateName = expression;
	var.name = std::move( expression );
	var.value = info.result;
	var.type = info.type;
	var.variablesReference = info.variablesReference;
	var.indexedVariables = info.indexedVariables;
	var.namedVariables = info.namedVariables;
	var.memoryReference = info.memoryReference;
	mHoverExpressionsHolder->upsertRootChild( std::move( var ) );

	mHoverTooltip->setData( String::hash( "debugger" ) );
	mHoverTooltip->setPixelsPosition( UITooltip::getTooltipPosition( mHoverTooltip, position ) );

	if ( editor->hasFocus() && !mHoverTooltip->isVisible() )
		mHoverTooltip->showWhenReady();
}

bool DebuggerPlugin::onMouseMove( UICodeEditor* editor, const Vector2i& position, const Uint32& ) {
	if ( !mDebugger || !mListener || !mDebugger->isServerConnected() ||
		 mDebuggingState != StatusDebuggerController::State::Paused ) {
		return false;
	}

	auto localPos( editor->convertToNodeSpace( position.asFloat() ) );
	if ( localPos.x <= editor->getGutterWidth() )
		return false;

	if ( editor->getTooltip() )
		editor->getTooltip()->hide();

	editor->debounce(
		[this, editor, position]() {
			if ( !mManager->getSplitter()->editorExists( editor ) )
				return;
			auto docPos = editor->resolveScreenPosition( position.asFloat() );
			auto range = editor->getDocument().getWordRangeInPosition( docPos, true );
			auto expression = editor->getDocument().getWordInPosition( docPos, true ).toUtf8();

			if ( !range.isValid() || expression.empty() ||
				 LuaPattern::hasMatches( expression, "^[%p%s]+$" ) )
				return;

			if ( !mDebugger )
				return;

			mCurrentHover = range;

			mDebugger->evaluate(
				expression, "hover", mListener->getCurrentFrameId(),
				[this, editor, expression]( const std::string&,
											const std::optional<EvaluateInfo>& info ) {
					if ( info && mManager->getSplitter()->editorExists( editor ) &&
						 !info->result.empty() ) {
						editor->runOnMainThread(
							[this, editor, info = std::move( *info ), expression]() {
								auto mousePos = editor->getInput()->getMousePos();
								if ( !editor->getScreenRect().contains( mousePos.asFloat() ) )
									return;

								auto docPos = editor->resolveScreenPosition( mousePos.asFloat() );
								auto range =
									editor->getDocument().getWordRangeInPosition( docPos, true );

								if ( mCurrentHover.contains( range ) ) {
									mCurrentHover = range;
									displayTooltip( editor, expression, info, mousePos.asFloat() );
								}
							} );
					}
				} );
		},
		mHoverDelay, getMouseMoveHash( editor ) );
	editor->updateMouseCursor( position.asFloat() );
	return false;
}

void DebuggerPlugin::onDocumentLineMove( TextDocument* doc, const Int64& fromLine,
										 const Int64& toLine, const Int64& numLines ) {
	Lock l( mBreakpointsMutex );

	auto& breakpoints = mBreakpoints[doc->getFilePath()];

	if ( breakpoints.empty() )
		return;

	std::vector<SourceBreakpointStateful> bpToModify;

	for ( auto& bp : breakpoints ) {
		// bp line numbers start at 1
		if ( bp.line - 1 >= fromLine ) {
			bpToModify.push_back( bp );
			break;
		}
	}

	for ( const auto& bp : bpToModify )
		breakpoints.erase( bp );

	for ( auto&& bp : bpToModify ) {
		bp.line += numLines;
		breakpoints.insert( std::move( bp ) );
	}

	if ( mBreakpointsModel )
		mBreakpointsModel->move( doc->getFilePath(), fromLine, toLine, numLines );
}

} // namespace ecode
