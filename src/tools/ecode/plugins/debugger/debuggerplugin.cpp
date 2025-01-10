#include "../../notificationcenter.hpp"
#include "../../projectbuild.hpp"
#include "../../uistatusbar.hpp"
#include "../../widgetcommandexecuter.hpp"
#include "busprocess.hpp"
#include "dap/debuggerclientdap.hpp"
#include "debuggerplugin.hpp"
#include "models/breakpointsmodel.hpp"
#include "statusdebuggercontroller.hpp"
#include <eepp/graphics/primitives.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/uidropdownlist.hpp>
#include <eepp/ui/uitooltip.hpp>
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

	if ( mSidePanel && mTab )
		mSidePanel->removeTab( mTab );

	if ( getPluginContext()->getStatusBar() )
		getPluginContext()->getStatusBar()->removeStatusBarElement( "status_app_debugger" );

	mManager->unsubscribeMessages( this );

	for ( auto editor : mEditors ) {
		onBeforeUnregister( editor.first );
		onUnregisterEditor( editor.first );
	}

	mDebugger.reset();
	mListener.reset();

	if ( !isShuttingDown() && getPluginContext()->getMainLayout() ) {
		for ( const auto& kb : mKeyBindings )
			getPluginContext()->getMainLayout()->getKeyBindings().removeCommandKeybind( kb.first );
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
	"debugger-stop",
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

	if ( j.contains( "dap" ) ) {
		auto& dapArr = j["dap"];
		mDaps.reserve( dapArr.size() );
		for ( const auto& dap : dapArr ) {
			DapTool dapTool;
			dapTool.name = dap.value( "name", "" );
			dapTool.type = dap.value( "type", "" );
			dapTool.url = dap.value( "url", "" );

			if ( dap.contains( "run" ) ) {
				auto& run = dap["run"];
				dapTool.run.command = run.value( "command", "" );
				dapTool.fallbackCommand = run.value( "command_fallback", "" );
				if ( run.contains( "command_arguments" ) ) {
					if ( run["command_arguments"].is_array() ) {
						auto& args = run["command_arguments"];
						dapTool.run.args.reserve( args.size() );
						for ( auto& arg : args ) {
							if ( args.is_string() )
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
						dapConfig.command = config.value( "command", "launch" );
						dapConfig.args = config["arguments"];
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

	if ( j.contains( "config" ) ) {
	}

	if ( mKeyBindings.empty() ) {
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
		if ( type != "cppdbg" )
			continue;
		dapConfig.name = conf.value( "name", "" );
		if ( dapConfig.name.empty() )
			continue;
		dapConfig.command = conf.value( "request", "" );
		if ( dapConfig.command != "launch" && dapConfig.command != "attach" )
			continue;
		dapConfig.args = std::move( conf );

		{
			Lock l( mDapsMutex );
			mDapConfigs.emplace_back( std::move( dapConfig ) );
		}
	}
}

void DebuggerPlugin::loadProjectConfigurations() {
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

		getUISceneNode()->runOnMainThread( [this] { updateDebuggerConfigurationList(); } );
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

			mBreakpointsModel.reset();
			updateUI();
			mInitialized = true;
			break;
		}
		case ecode::PluginMessageType::UIReady: {
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
	</style>
	<vbox id="debugger_panel" lw="mp" lh="wc" padding="4dp">
		<vbox id="debugger_config_view" lw="mp" lh="wc">
			<TextView text="@string(debugger, Debugger)" font-size="15dp" focusable="false" />
			<DropDownList id="debugger_list" layout_width="mp" layout_height="wrap_content" margin-top="2dp" />
			<TextView text="@string(debugger_configuration, Debugger Configuration)" focusable="false" margin-top="8dp" />
			<DropDownList id="debugger_conf_list" layout_width="mp" layout_height="wrap_content" margin-top="2dp" />
			<PushButton id="debugger_run_button" lw="mp" lh="wc" text="@string(run, Run)" margin-top="8dp" icon="icon(play, 12dp)" />
			<hbox id="panel_debugger_buttons" lw="wc" lh="wc" layout_gravity="center_horizontal" visible="false" clip="none" margin-top="8dp">
				<PushButton id="panel_debugger_continue" class="debugger_continue" lw="24dp" lh="24dp" icon="icon(debug-continue, 12dp)" tooltip="@string(continue, Continue)" />
				<PushButton id="panel_debugger_pause" class="debugger_pause" lw="24dp" lh="24dp" icon="icon(debug-pause, 12dp)" tooltip="@string(pause, Pause)" />
				<PushButton id="panel_debugger_step_over" class="debugger_step_over" lw="24dp" lh="24dp" icon="icon(debug-step-over, 12dp)" tooltip="@string(step_over, Step Over)" />
				<PushButton id="panel_debugger_step_into" class="debugger_step_into" lw="24dp" lh="24dp" icon="icon(debug-step-into, 12dp)" tooltip="@string(step_into, Step Into)" />
				<PushButton id="panel_debugger_step_out" class="debugger_step_out" lw="24dp" lh="24dp" icon="icon(debug-step-out, 12dp)" tooltip="@string(step_out, Step Out)" />
			</hbox>
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

	mTabContents->bind( "panel_debugger_buttons", mPanelBoxButtons.box );
	mTabContents->bind( "panel_debugger_continue", mPanelBoxButtons.resume );
	mTabContents->bind( "panel_debugger_pause", mPanelBoxButtons.pause );
	mTabContents->bind( "panel_debugger_step_over", mPanelBoxButtons.stepOver );
	mTabContents->bind( "panel_debugger_step_into", mPanelBoxButtons.stepInto );
	mTabContents->bind( "panel_debugger_step_out", mPanelBoxButtons.stepOut );

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

	setUIDebuggingState( StatusDebuggerController::State::NotStarted );

	updateSidePanelTab();
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

	if ( !empty )
		mUIDebuggerList->getListBox()->setSelected( 0L );

	updateDebuggerConfigurationList();

	mRunButton = mTabContents->find<UIPushButton>( "debugger_run_button" );

	if ( !mRunButton->hasEventsOfType( Event::MouseClick ) ) {
		mRunButton->onClick( [this]( auto ) {
			if ( mDebugger && mDebugger->started() ) {
				exitDebugger();
			} else {
				runCurrentConfig();
			}
		} );
	}
}

void DebuggerPlugin::runCurrentConfig() {
	runConfig( mUIDebuggerList->getListBox()->getItemSelectedText().toUtf8(),
			   mUIDebuggerConfList->getListBox()->getItemSelectedText().toUtf8() );
}

void DebuggerPlugin::sendPendingBreakpoints() {
	for ( const auto& pbp : mPendingBreakpoints )
		sendFileBreakpoints( pbp );
	mPendingBreakpoints.clear();
	if ( mDebugger )
		mDebugger->resume( 1 );
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

	Lock l( mBreakpointsMutex );
	auto fileBps = mBreakpoints.find( filePath );
	if ( fileBps == mBreakpoints.end() )
		return;
	for ( const auto& fileBps : mBreakpoints ) {
		mDebugger->setBreakpoints( fileBps.first,
								   DebuggerClientListener::fromSet( fileBps.second ) );
	}
}

void DebuggerPlugin::updateDebuggerConfigurationList() {
	mUIDebuggerConfList->getListBox()->clear();

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
			if ( conf.args.value( "type", "" ) != debuggerIt->type )
				continue;

			confNames.emplace_back( conf.name );
		}
	}

	std::sort( confNames.begin(), confNames.end() );
	mUIDebuggerConfList->getListBox()->addListBoxItems( confNames );
	bool empty = mUIDebuggerConfList->getListBox()->isEmpty();
	mUIDebuggerConfList->setEnabled( !empty );
	if ( !empty )
		mUIDebuggerConfList->getListBox()->setSelected( 0L );
}

void DebuggerPlugin::replaceKeysInJson( nlohmann::json& json ) {
	static constexpr auto KEY_FILE = "${file}";
	static constexpr auto KEY_ARGS = "${args}";
	static constexpr auto KEY_CWD = "${cwd}";
	static constexpr auto KEY_ENV = "${env}";
	static constexpr auto KEY_STOPONENTRY = "${stopOnEntry}";
	static constexpr auto KEY_WORKSPACEFOLDER = "${workspaceFolder}";
	static constexpr auto KEY_FILEDIRNAME = "${fileDirname}";
	auto runConfig = getPluginContext()->getProjectBuildManager()->getCurrentRunConfig();

	for ( auto& j : json ) {
		if ( j.is_object() ) {
			replaceKeysInJson( j );
		} else if ( j.is_string() ) {
			std::string val( j.get<std::string>() );

			if ( runConfig && val == KEY_ARGS ) {
				auto argsArr = nlohmann::json::array();
				auto args = Process::parseArgs( runConfig->args );
				for ( const auto& arg : args )
					argsArr.push_back( arg );
				j = std::move( argsArr );
				continue;
			} else if ( runConfig && val == KEY_ENV ) {
				j = nlohmann::json{};
			} else if ( val == KEY_STOPONENTRY ) {
				j = false;
			} else if ( runConfig ) {
				String::replaceAll( val, KEY_FILE, runConfig->cmd );
				String::replaceAll( val, KEY_CWD, runConfig->workingDir );
				String::replaceAll( val, KEY_FILEDIRNAME, runConfig->workingDir );
				String::replaceAll( val, KEY_WORKSPACEFOLDER, mProjectPath );
				j = std::move( val );
			}
		}
	}
}

void DebuggerPlugin::onRegisterDocument( TextDocument* doc ) {
	doc->setCommand( "debugger-continue-interrupt", [this] {
		if ( mDebugger && mListener ) {
			if ( mListener->isStopped() ) {
				mDebugger->resume( mListener->getCurrentThreadId() );
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
						auto msgBox =
							UIMessageBox::New( UIMessageBox::YES_NO,
											   i18n( "build_failed_debug_anyways",
													 "Building the project failed, do you want to "
													 "debug the binary anyways?" ) );
						msgBox->setTitle( i18n( "build_failed", "Build Failed" ) );
						msgBox->on( Event::OnConfirm, [this]( auto ) { runCurrentConfig(); } );
						msgBox->showWhenReady();
					}
				} );
		}
	} );

	doc->setCommand( "debugger-stop", [this] { exitDebugger(); } );

	doc->setCommand( "debugger-breakpoint-toggle", [doc, this] {
		if ( setBreakpoint( doc, doc->getSelection().start().line() + 1 ) )
			getUISceneNode()->getRoot()->invalidateDraw();
	} );

	doc->setCommand( "debugger-breakpoint-enable-toggle", [this, doc] {
		if ( breakpointToggleEnabled( doc, doc->getSelection().start().line() + 1 ) )
			getUISceneNode()->getRoot()->invalidateDraw();
	} );

	doc->setCommand( "debugger-step-over", [this] {
		if ( mDebugger && mListener && mListener->isStopped() )
			mDebugger->stepOver( mListener->getCurrentThreadId() );
	} );

	doc->setCommand( "debugger-step-into", [this] {
		if ( mDebugger && mListener && mListener->isStopped() )
			mDebugger->stepInto( mListener->getCurrentThreadId() );
	} );

	doc->setCommand( "debugger-step-out", [this] {
		if ( mDebugger && mListener && mListener->isStopped() )
			mDebugger->stepOut( mListener->getCurrentThreadId() );
	} );

	doc->setCommand( "toggle-status-app-debugger", [this] {
		if ( getStatusDebuggerController() )
			getStatusDebuggerController()->toggle();
	} );
}

void DebuggerPlugin::onRegisterEditor( UICodeEditor* editor ) {
	editor->registerGutterSpace( this, PixelDensity::dpToPx( 8 ), 0 );

	editor->addUnlockedCommands( DebuggerCommandList );

	PluginBase::onRegisterEditor( editor );
}

void DebuggerPlugin::onUnregisterEditor( UICodeEditor* editor ) {
	editor->removeUnlockedCommands( DebuggerCommandList );

	editor->unregisterGutterSpace( this );
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

	if ( mDebugger && mListener && mListener->isStopped() && mListener->getCurrentScopePos() &&
		 editor->getDocument().getFilePath() == mListener->getCurrentScopePos()->first ) {
		int line = mListener->getCurrentScopePos()->second - 1;
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

bool DebuggerPlugin::setBreakpoint( const std::string& doc, Uint32 lineNumber ) {
	Lock l( mBreakpointsMutex );

	auto sdc = getStatusDebuggerController();
	if ( sdc && sdc->getWidget() == nullptr ) {
		sdc->show();
		sdc->hide();
		sdc->getUIBreakpoints()->onBreakpointEnabledChange = [this]( const std::string& filePath,
																	 int line, bool enabled ) {
			breakpointSetEnabled( filePath, line, enabled );
		};
		sdc->getUIBreakpoints()->onBreakpointRemove =
			[this]( const std::string& filePath, int line ) { setBreakpoint( filePath, line ); };
	}

	if ( !mBreakpointsModel )
		mBreakpointsModel = std::make_shared<BreakpointsModel>( mBreakpoints, getUISceneNode() );

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
	Lock l( mBreakpointsMutex );
	auto& breakpoints = mBreakpoints[doc];
	auto breakpointIt = breakpoints.find( SourceBreakpointStateful( lineNumber ) );
	if ( breakpointIt != breakpoints.end() ) {
		breakpointIt->enabled = enabled;
		mBreakpointsModel->enable( doc, lineNumber, breakpointIt->enabled );
		getUISceneNode()->getRoot()->invalidateDraw();
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

bool DebuggerPlugin::onMouseDown( UICodeEditor* editor, const Vector2i& position,
								  const Uint32& flags ) {
	if ( !( flags & EE_BUTTON_LMASK ) )
		return false;
	Float offset = editor->getGutterLocalStartOffset( this );
	Vector2f localPos( editor->convertToNodeSpace( position.asFloat() ) );
	if ( localPos.x >= editor->getPixelsPadding().Left + offset &&
		 localPos.x < editor->getPixelsPadding().Left + offset + editor->getGutterSpace( this ) +
						  editor->getLineNumberPaddingLeft() &&
		 localPos.y > editor->getPluginsTopSpace() ) {
		if ( editor->getUISceneNode()->getEventDispatcher()->isFirstPress() ) {
			auto cursorPos( editor->resolveScreenPosition( position.asFloat() ) );
			setBreakpoint( editor, cursorPos.line() + 1 );
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
	p.create( findCmd );
	std::string path;
	p.readAllStdOut( path, Seconds( 5 ) );
	int retCode = -1;
	p.join( &retCode );
	if ( retCode == 0 && !path.empty() ) {
		String::trimInPlace( path, '\n' );
		return path;
	}
	return "";
}

void DebuggerPlugin::runConfig( const std::string& debugger, const std::string& configuration ) {
	auto runConfig = getPluginContext()->getProjectBuildManager()->getCurrentRunConfig();
	if ( !runConfig ) {
		auto msg =
			i18n( "no_run_config",
				  "You must first have a \"Run Target\" configured and selected. Go to \"Build "
				  "Settings\" and create a new build and run setting (build step is optional)." );
		mManager->getPluginContext()->getNotificationCenter()->addNotification( msg, Seconds( 5 ) );
		return;
	}

	auto debuggerIt = std::find_if( mDaps.begin(), mDaps.end(), [&debugger]( const DapTool& dap ) {
		return dap.name == debugger;
	} );

	if ( debuggerIt == mDaps.end() ) {
		return;
	}

	auto configIt = std::find_if(
		debuggerIt->configurations.begin(), debuggerIt->configurations.end(),
		[&configuration]( const DapConfig& conf ) { return conf.name == configuration; } );

	if ( configIt == debuggerIt->configurations.end() ) {
		configIt = std::find_if(
			mDapConfigs.begin(), mDapConfigs.end(),
			[&configuration]( const DapConfig& conf ) { return conf.name == configuration; } );

		if ( configIt == debuggerIt->configurations.end() )
			return;
	}

	ProtocolSettings protocolSettings;
	protocolSettings.launchCommand = configIt->command;
	auto args = configIt->args;
	replaceKeysInJson( args );
	protocolSettings.launchRequest = args;

	Command cmd;
	cmd.command = debuggerIt->run.command;
	cmd.arguments = debuggerIt->run.args;

	mRunButton->setEnabled( false );

	std::string findBinary;
	auto findBinaryIt = debuggerIt->findBinary.find( String::toLower( Sys::getPlatform() ) );
	if ( findBinaryIt != debuggerIt->findBinary.end() )
		findBinary = findBinaryIt->second;

	std::string fallbackCommand = debuggerIt->fallbackCommand;

	mThreadPool->run(
		[this, cmd = std::move( cmd ), protocolSettings = std::move( protocolSettings ),
		 findBinary = std::move( findBinary ),
		 fallbackCommand = std::move( fallbackCommand )]() mutable {
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

			if ( !FileSystem::fileExists( cmd.command ) && Sys::which( cmd.command ).empty() ) {
				auto args = Process::parseArgs( cmd.command );
				if ( args.size() <= 1 ||
					 ( !FileSystem::fileExists( args[0] ) && Sys::which( args[0] ).empty() ) ) {
					if ( fallbackCommand.empty() || ( !FileSystem::fileExists( fallbackCommand ) &&
													  Sys::which( fallbackCommand ).empty() ) ) {
						auto msg = String::format(
							i18n( "debugger_binary_not_found",
								  "Debugger binary not found. Binary \"%s\" must be installed." )
								.toUtf8(),
							cmd.command );

						mManager->getPluginContext()->getNotificationCenter()->addNotification(
							msg );
						return;
					} else {
						cmd.command = std::move( fallbackCommand );
					}
				}
			}

			auto bus = std::make_unique<BusProcess>( cmd );

			mDebugger = std::make_unique<DebuggerClientDap>( protocolSettings, std::move( bus ) );
			mListener = std::make_unique<DebuggerClientListener>( mDebugger.get(), this );
			mDebugger->addListener( mListener.get() );

			mDebugger->start();
		},
		[this]( const Uint64& ) {
			if ( !mDebugger || !mDebugger->started() ) {
				exitDebugger();

				getManager()->getPluginContext()->getNotificationCenter()->addNotification(
					i18n( "debugger_init_failed", "Failed to initialize debugger." ) );
			} else {
				mRunButton->runOnMainThread( [this] {
					mRunButton->setEnabled( true );
					mRunButton->setText( i18n( "cancel_run", "Cancel Run" ) );
				} );
			}
		} );
}

void DebuggerPlugin::exitDebugger() {
	if ( mDebugger && mListener )
		mDebugger->removeListener( mListener.get() );
	mThreadPool->run( [this] {
		mDebugger.reset();
		mListener.reset();
	} );
	if ( getUISceneNode() && mRunButton ) {
		mRunButton->runOnMainThread( [this] {
			mRunButton->setText( i18n( "run", "Run" ) );
			mRunButton->setEnabled( true );
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
	mPanelBoxButtons.box->setVisible( state != StatusDebuggerController::State::NotStarted );
	mPanelBoxButtons.resume->setVisible( state != StatusDebuggerController::State::NotStarted )
		->setEnabled( state == StatusDebuggerController::State::Paused );
	mPanelBoxButtons.pause->setVisible( state != StatusDebuggerController::State::NotStarted )
		->setEnabled( state == StatusDebuggerController::State::Running );
	mPanelBoxButtons.stepOver->setVisible( state != StatusDebuggerController::State::NotStarted )
		->setEnabled( state == StatusDebuggerController::State::Paused );
	mPanelBoxButtons.stepInto->setVisible( state != StatusDebuggerController::State::NotStarted )
		->setEnabled( state == StatusDebuggerController::State::Paused );
	mPanelBoxButtons.stepOut->setVisible( state != StatusDebuggerController::State::NotStarted )
		->setEnabled( state == StatusDebuggerController::State::Paused );
}

void DebuggerPlugin::setUIDebuggingState( StatusDebuggerController::State state ) {
	mDebuggingState = state;

	auto ctrl = getStatusDebuggerController();
	if ( ctrl ) {
		ctrl->setDebuggingState( state );
	}

	if ( mPanelBoxButtons.box ) {
		if ( Engine::isRunninMainThread() )
			updatePanelUIState( state );
		else
			mPanelBoxButtons.box->runOnMainThread( [this, state] { updatePanelUIState( state ); } );
	}
}

// Mouse Hover Tooltip
static Action::UniqueID getMouseMoveHash( UICodeEditor* editor ) {
	return hashCombine( String::hash( "DebuggerPlugin::onMouseMove-" ),
						reinterpret_cast<Action::UniqueID>( editor ) );
}

void DebuggerPlugin::hideTooltip( UICodeEditor* editor ) {
	UITooltip* tooltip = nullptr;
	if ( editor && ( tooltip = editor->getTooltip() ) && tooltip->isVisible() &&
		 tooltip->getData() == String::hash( "debugger" ) ) {
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

void DebuggerPlugin::displayTooltip( UICodeEditor* editor, const EvaluateInfo& resp,
									 const Vector2f& position ) {
	// HACK: Gets the old font style to restore it when the tooltip is hidden
	UITooltip* tooltip = editor->createTooltip();
	if ( tooltip == nullptr )
		return;
	mOldWordWrap = tooltip->isWordWrap();
	mOldMaxWidth = tooltip->getMaxWidthEq();
	tooltip->setWordWrap( true );
	tooltip->setMaxWidthEq( "50vw" );
	editor->setTooltipText( resp.result );
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
	tooltip->setData( String::hash( "debugger" ) );
	tooltip->setBackgroundColor( editor->getColorScheme().getEditorColor( "background"_sst ) );
	tooltip->getUIStyle()->setStyleSheetProperty( StyleSheetProperty(
		"background-color",
		editor->getColorScheme().getEditorColor( "background"_sst ).toHexString(), true,
		StyleSheetSelectorRule::SpecificityImportant ) );

	if ( tooltip->getText().empty() )
		return;

	tooltip->notifyTextChangedFromTextCache();

	if ( editor->hasFocus() && !tooltip->isVisible() &&
		 !tooltip->getTextCache()->getString().empty() )
		tooltip->show();
}

void DebuggerPlugin::tryHideTooltip( UICodeEditor* editor, const Vector2i& position ) {
	if ( !mCurrentHover.isValid() ||
		 ( mCurrentHover.isValid() &&
		   !mCurrentHover.contains( editor->resolveScreenPosition( position.asFloat() ) ) ) )
		hideTooltip( editor );
}

bool DebuggerPlugin::onMouseMove( UICodeEditor* editor, const Vector2i& position,
								  const Uint32& flags ) {

	if ( !mDebugger || !mListener || !mDebugger->isServerConnected() ||
		 mDebuggingState != StatusDebuggerController::State::Paused ) {
		tryHideTooltip( editor, position );
		return false;
	}

	if ( flags != 0 ) {
		tryHideTooltip( editor, position );
		return false;
	}

	editor->debounce(
		[this, editor, position]() {
			if ( !mManager->getSplitter()->editorExists( editor ) )
				return;
			auto docPos = editor->resolveScreenPosition( position.asFloat() );
			auto range = editor->getDocument().getWordRangeInPosition( docPos, true );
			auto expression = editor->getDocument().getWordInPosition( docPos, true ).toUtf8();

			if ( !range.isValid() || expression.empty() )
				return;

			mCurrentHover = range;

			mDebugger->evaluate(
				expression, "hover", mListener->getCurrentFrameId(),
				[this, editor]( const std::string&, const std::optional<EvaluateInfo>& info ) {
					if ( info && mManager->getSplitter()->editorExists( editor ) &&
						 !info->result.empty() ) {
						editor->runOnMainThread( [this, editor, info = std::move( *info )]() {
							auto mousePos =
								editor->getUISceneNode()->getWindow()->getInput()->getMousePos();
							if ( !editor->getScreenRect().contains( mousePos.asFloat() ) )
								return;

							auto docPos = editor->resolveScreenPosition( mousePos.asFloat() );
							auto range =
								editor->getDocument().getWordRangeInPosition( docPos, true );

							if ( mCurrentHover.contains( range ) ) {
								mCurrentHover = range;
								displayTooltip( editor, info, mousePos.asFloat() );
							}
						} );
					}
				} );
		},
		mHoverDelay, getMouseMoveHash( editor ) );
	tryHideTooltip( editor, position );
	return true;
}

} // namespace ecode
