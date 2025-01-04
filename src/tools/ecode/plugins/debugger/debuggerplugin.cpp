#include "debuggerplugin.hpp"
#include "../../projectbuild.hpp"
#include "../../uistatusbar.hpp"
#include "busprocess.hpp"
#include "dap/debuggerclientdap.hpp"
#include "statusdebuggercontroller.hpp"
#include <eepp/graphics/primitives.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/ui/uidropdownlist.hpp>
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

	if ( getManager()->getPluginContext()->getStatusBar() ) {
		getManager()->getPluginContext()->getStatusBar()->removeStatusBarElement(
			"status_app_debugger" );
	}

	mDebugger.reset();
	mListener.reset();
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
			dapTool.url = dap.value( "url", "" );
			if ( dap.contains( "run" ) ) {
				auto& run = dap["run"];
				dapTool.run.command = run.value( "command", "" );
				if ( run.contains( "command_arguments" ) && run["command_arguments"].is_array() ) {
					auto& args = run["command_arguments"];
					dapTool.run.args.reserve( args.size() );
					for ( auto& arg : args ) {
						if ( args.is_string() )
							dapTool.run.args.emplace_back( arg.get<std::string>() );
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

			mDaps.emplace_back( std::move( dapTool ) );
		}
	}

	if ( j.contains( "config" ) ) {
	}

	if ( mKeyBindings.empty() ) {
		mKeyBindings["debugger-continue-interrupt"] = "f5";
	}

	if ( j.contains( "keybindings" ) ) {
		auto& kb = j["keybindings"];
		std::initializer_list<std::string> list = { "debugger-continue-interrupt" };
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
}

PluginRequestHandle DebuggerPlugin::processMessage( const PluginMessage& msg ) {
	switch ( msg.type ) {
		case PluginMessageType::WorkspaceFolderChanged: {
			mProjectPath = msg.asJSON()["folder"];

			if ( getUISceneNode() && mSidePanel )
				getUISceneNode()->runOnMainThread( [this] {
					if ( mProjectPath.empty() )
						hideSidePanel();
				} );

			updateUI();
			mInitialized = true;
			break;
		}
		case ecode::PluginMessageType::UIReady: {
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
	<vbox id="debugger_panel" lw="mp" lh="wc" padding="4dp">
		<vbox id="debugger_config_view" lw="mp" lh="wc">
			<TextView text="@string(debugger, Debugger)" font-size="15dp" focusable="false" />
			<DropDownList id="debugger_list" layout_width="mp" layout_height="wrap_content" margin-top="2dp" />
			<TextView text="@string(debugger_configuration, Debugger Configuration)" focusable="false" margin-top="8dp" />
			<DropDownList id="debugger_conf_list" layout_width="mp" layout_height="wrap_content" margin-top="2dp" />
			<PushButton id="debugger_run_button" lw="mp" lh="wc" text="@string(run, Run)" margin-top="8dp" icon="icon(play, 12dp)" />
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

	updateSidePanelTab();
}

void DebuggerPlugin::buildStatusBar() {
	if ( mProjectPath.empty() ) {
		hideStatusBarElement();
		return;
	}
	if ( getManager()->getPluginContext()->getStatusBar() ) {
		auto but = getManager()->getPluginContext()->getStatusBar()->find( "status_app_debugger" );
		if ( but ) {
			but->setVisible( true );
			return;
		}
	}

	auto context = getManager()->getPluginContext();
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
	for ( const auto& fileBps : mBreakpoints )
		mDebugger->setBreakpoints( fileBps.first,
								   DebuggerClientListener::fromSet( fileBps.second ) );
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
	auto runConfig =
		getManager()->getPluginContext()->getProjectBuildManager()->getCurrentRunConfig();

	for ( auto& j : json ) {
		if ( j.is_object() ) {
			replaceKeysInJson( j );
		} else if ( j.is_string() ) {
			std::string val( j.get<std::string>() );
			if ( runConfig && val == KEY_FILE ) {
				j = runConfig->cmd;
			} else if ( runConfig && val == KEY_ARGS ) {
				auto argsArr = nlohmann::json::array();
				auto args = Process::parseArgs( runConfig->args );
				for ( const auto& arg : args )
					argsArr.push_back( arg );
				j = argsArr;
			} else if ( runConfig && val == KEY_CWD ) {
				j = runConfig->workingDir;
			} else if ( runConfig && val == KEY_ENV ) {
				j = nlohmann::json{};
			} else if ( val == KEY_STOPONENTRY ) {
				j = false;
			}
		}
	}
}

void DebuggerPlugin::onRegisterDocument( TextDocument* doc ) {
	doc->setCommand( "debugger-continue-interrupt", [this]() {
		if ( mDebugger && mListener ) {
			if ( mListener->isStopped() ) {
				mDebugger->resume( mListener->getStoppedData()->threadId
									   ? *mListener->getStoppedData()->threadId
									   : 1 );
			} else {
				mDebugger->pause( 1 );
			}
		} else {
			runCurrentConfig();
		}
	} );
}

void DebuggerPlugin::onRegisterEditor( UICodeEditor* editor ) {
	editor->registerGutterSpace( this, PixelDensity::dpToPx( 8 ), 0 );

	PluginBase::onRegisterEditor( editor );
}

void DebuggerPlugin::onUnregisterEditor( UICodeEditor* editor ) {
	editor->unregisterGutterSpace( this );
}

void DebuggerPlugin::drawLineNumbersBefore( UICodeEditor* editor,
											const DocumentLineRange& lineRange,
											const Vector2f& startScroll,
											const Vector2f& screenStart, const Float& lineHeight,
											const Float&, const int&, const Float& ) {
	if ( !editor->getDocument().hasFilepath() )
		return;
	auto docIt = mBreakpoints.find( editor->getDocument().getFilePath() );
	if ( docIt == mBreakpoints.end() || docIt->second.empty() )
		return;
	const auto& breakpoints = docIt->second;
	Primitives p;
	Float lineOffset = editor->getLineOffset();
	p.setColor( Color( editor->getColorScheme().getEditorColor( SyntaxStyleTypes::Error ) )
					.blendAlpha( editor->getAlpha() ) );
	Float gutterSpace = editor->getGutterSpace( this );
	Float radius = PixelDensity::dpToPx( 3 );
	Float offset = editor->getGutterLocalStartOffset( this );

	for ( const SourceBreakpoint& breakpoint : breakpoints ) {
		if ( breakpoint.line >= lineRange.first && breakpoint.line <= lineRange.second ) {
			if ( !editor->getDocumentView().isLineVisible( breakpoint.line ) )
				continue;

			auto lnPos( Vector2f(
				screenStart.x - editor->getPluginsGutterSpace() + offset,
				startScroll.y +
					editor->getDocumentView().getLineYOffset( breakpoint.line, lineHeight ) +
					lineOffset ) );

			// p.setColor( Color::Gray );
			// p.drawRectangle( { lnPos, Sizef{ gutterSpace, lineHeight } } );

			p.setColor( Color( editor->getColorScheme().getEditorColor( SyntaxStyleTypes::Error ) )
							.blendAlpha( editor->getAlpha() ) );

			p.drawCircle( { lnPos.x + radius + eefloor( ( gutterSpace - radius ) * 0.5f ),
							lnPos.y + lineHeight * 0.5f },
						  radius );
		}
	}


}

bool DebuggerPlugin::setBreakpoint( UICodeEditor* editor, Uint32 lineNumber ) {
	if ( !editor->getDocument().hasFilepath() )
		return false;
	if ( !isSupportedByAnyDebugger( editor->getDocument().getSyntaxDefinition().getLSPName() ) )
		return false;
	Lock l( mBreakpointsMutex );
	auto& breakpoints = mBreakpoints[editor->getDocument().getFilePath()];
	auto breakpointIt = breakpoints.find( SourceBreakpointStateful( lineNumber ) );
	if ( breakpointIt != breakpoints.end() ) {
		breakpoints.erase( breakpointIt );
	} else {
		breakpoints.insert( SourceBreakpointStateful( lineNumber ) );
	}
	mThreadPool->run(
		[this, editor] { sendFileBreakpoints( editor->getDocument().getFilePath() ); } );
	editor->invalidateDraw();
	return true;
}

bool DebuggerPlugin::onMouseDown( UICodeEditor* editor, const Vector2i& position,
								  const Uint32& flags ) {
	if ( !( flags & EE_BUTTON_LMASK ) )
		return false;
	Float offset = editor->getGutterLocalStartOffset( this );
	Vector2f localPos( editor->convertToNodeSpace( position.asFloat() ) );
	if ( localPos.x >= editor->getPixelsPadding().Left + offset &&
		 localPos.x < editor->getPixelsPadding().Left + offset + editor->getGutterSpace( this ) &&
		 localPos.y > editor->getPluginsTopSpace() ) {
		if ( editor->getUISceneNode()->getEventDispatcher()->isFirstPress() ) {
			auto cursorPos( editor->resolveScreenPosition( position.asFloat() ) );
			setBreakpoint( editor, cursorPos.line() );
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

	if ( configIt == debuggerIt->configurations.end() ) {
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

	auto bus = std::make_unique<BusProcess>( cmd );

	mDebugger = std::make_unique<DebuggerClientDap>( protocolSettings, std::move( bus ) );
	mListener = std::make_unique<DebuggerClientListener>( mDebugger.get(), this );
	mDebugger->addListener( mListener.get() );

	mRunButton->setEnabled( false );

	mThreadPool->run( [this] { mDebugger->start(); },
					  [this]( const Uint64& ) {
						  if ( !mDebugger || !mDebugger->started() ) {
							  exitDebugger();
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
}

void DebuggerPlugin::hideSidePanel() {
	if ( mSidePanel && mTab ) {
		mSidePanel->removeTab( mTab, false );
		mTab = nullptr;
	}
}

void DebuggerPlugin::hideStatusBarElement() {
	if ( getManager()->getPluginContext()->getStatusBar() ) {
		auto but = getManager()->getPluginContext()->getStatusBar()->find( "status_app_debugger" );
		if ( but )
			but->setVisible( false );
	}
}

} // namespace ecode
