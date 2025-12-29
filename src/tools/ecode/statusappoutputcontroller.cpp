#include "statusappoutputcontroller.hpp"
#include "notificationcenter.hpp"
#include "plugins/plugincontextprovider.hpp"
#include "widgetcommandexecuter.hpp"
#include <eepp/ui/tools/uidocfindreplace.hpp>
#include <eepp/ui/uiscrollbar.hpp>

namespace ecode {

static std::string getProjectOutputParserTypeToString( const ProjectOutputParserTypes& type ) {
	switch ( type ) {
		case ProjectOutputParserTypes::Error:
			return "error";
		case ProjectOutputParserTypes::Warning:
			return "warning";
		case ProjectOutputParserTypes::Notice:
			return "notice";
	}
	return "notice";
}

StatusAppOutputController::StatusAppOutputController( UISplitter* mainSplitter,
													  UISceneNode* uiSceneNode,
													  PluginContextProvider* pluginContext ) :
	StatusBarElement( mainSplitter, uiSceneNode, pluginContext ) {}

UIPushButton* StatusAppOutputController::getRunButton() {
	if ( mContext->getSidePanel() ) {
		UIWidget* tab = mContext->getSidePanel()->find<UIWidget>( "build_tab_view" );
		if ( tab )
			return tab->find<UIPushButton>( "run_button" );
	}
	return nullptr;
}

UIPushButton* StatusAppOutputController::getBuildAndRunButton() {
	if ( mContext->getSidePanel() ) {
		UIWidget* tab = mContext->getSidePanel()->find<UIWidget>( "build_tab_view" );
		if ( tab )
			return tab->find<UIPushButton>( "build_and_run_button" );
	}
	return nullptr;
}

void StatusAppOutputController::initNewOutput( const ProjectBuildOutputParser& outputParser,
											   bool fromBuildPanel ) {
	show();
	mAppOutput->getDocument().reset();
	mAppOutput->invalidateLongestLineWidth();
	mAppOutput->setScrollY( mAppOutput->getMaxScroll().y );

	std::vector<SyntaxPattern> patterns;

	auto configs = { outputParser.getPresetConfig(), outputParser.getConfig() };
	for ( const auto& config : configs ) {
		for ( const auto& parser : config ) {
			mPatternHolder.push_back( { LuaPatternStorage( parser.pattern ), parser } );

			SyntaxPattern ptn( { parser.pattern },
							   getProjectOutputParserTypeToString( parser.type ) );

			patterns.emplace_back( std::move( ptn ) );
		}
	}

	patterns.emplace_back(
		SyntaxPattern( { "%d%d%d%d%-%d%d%-%d%d%s%d%d%:%d%d%:%d%d%:.*error.*[^\n]+" }, "error" ) );
	patterns.emplace_back( SyntaxPattern(
		{ "%d%d%d%d%-%d%d%-%d%d%s%d%d%:%d%d%:%d%d%:.*warning.*[^\n]+" }, "warning" ) );
	patterns.emplace_back(
		SyntaxPattern( { "%d%d%d%d%-%d%d%-%d%d%s%d%d%:%d%d%:%d%d%:[^\n]+" }, "notice" ) );

	SyntaxDefinition synDef( "custom_build", {}, std::move( patterns ) );

	mAppOutput->getDocument().setSyntaxDefinition( synDef );
	mAppOutput->getVScrollBar()->setValue( 1.f );
	mAppOutput->getDocument().getHighlighter()->setMaxTokenizationLength( 2048 );
	mAppOutput->setLineWrapMode( LineWrapMode::Word );
	mScrollLocked = true;

	if ( fromBuildPanel ) {
		UIPushButton* runButton = getRunButton();

		if ( runButton )
			runButton->setText( mContext->i18n( "cancel_run", "Cancel Run" ) );

		UIPushButton* buildAndRunButton = getBuildAndRunButton();

		if ( buildAndRunButton )
			buildAndRunButton->setEnabled( false );
	}

	mRunButton->setEnabled( false );
	mStopButton->setEnabled( true );
}

void StatusAppOutputController::insertBuffer( const std::string& buffer ) {
	if ( mAppOutput == nullptr )
		return;
	mAppOutput->runOnMainThread( [this, buffer]() {
		mAppOutput->getDocument().insert( 0, mAppOutput->getDocument().endOfDoc(), buffer );
		if ( mScrollLocked )
			mAppOutput->setScrollY( mAppOutput->getMaxScroll().y );
	} );
}

void StatusAppOutputController::run( const ProjectBuildCommand& runData,
									 const ProjectBuildOutputParser& outputParser ) {
	if ( nullptr == mContext->getProjectBuildManager() )
		return;

	initNewOutput( outputParser );

	auto res = mContext->getProjectBuildManager()->run(
		runData, [this]( const auto& key, const auto& def ) { return mContext->i18n( key, def ); },
		[this]( auto, std::string buffer, const ProjectBuildCommand* ) { insertBuffer( buffer ); },
		[this]( auto exitCode, const ProjectBuildCommand* ) {
			String buffer;

			if ( EXIT_SUCCESS == exitCode ) {
				buffer = Sys::getDateTimeStr() + ": " +
						 mContext->i18n( "run_successful", "Run successfully\n" );
			} else {
				buffer = Sys::getDateTimeStr() + ": " +
						 mContext->i18n( "run_failed", "Run with errors\n" );
			}

			insertBuffer( buffer );

			updateRunButton();
		} );

	if ( !res.isValid() ) {
		mContext->getNotificationCenter()->addNotification( res.errorMsg );

		updateRunButton();
	}
}

UIWidget* StatusAppOutputController::getWidget() {
	return mContainer;
}

UIWidget* StatusAppOutputController::createWidget() {
	if ( nullptr == mContainer )
		createContainer();
	return mContainer;
}

UICodeEditor* StatusAppOutputController::getContainer() {
	return mAppOutput;
}

void StatusAppOutputController::createContainer() {
	if ( mContainer )
		return;
	const auto XML = R"xml(
	<hboxce id="app_output" class="vertical_bar" lw="mp" lh="mp" visible="false">
		<rellayce id="app_command_executer" lw="0" lw8="1" lh="mp">
			<CodeEditor id="app_output_output" lw="mp" lh="mp" />
		</rellayce>
		<vbox lw="16dp" lh="mp">
			<PushButton class="expand_status_bar_panel" lw="mp" tooltip="@string(expand_panel, Expand Panel)" />
			<PushButton id="app_output_clear" lw="mp" icon="icon(eraser, 12dp)" tooltip="@string(clear, Clear)" />
			<PushButton id="app_output_run" lw="mp" icon="icon(play, 12dp)" tooltip="@string(run, Run)" />
			<PushButton id="app_output_stop" lw="mp" icon="icon(stop, 12dp)" enabled="false" />
			<PushButton id="app_output_find" lw="mp" icon="icon(search, 12dp)" tooltip="@string(find, Find)" />
			<PushButton id="app_output_configure" lw="mp" icon="icon(settings, 12dp)" tooltip="@string(configure_ellipsis, Configure...)" />
			<Widget lw="mp" lh="0" lw8="1" />
			<PushButton class="status_bar_panel_hide" lw="mp" tooltip="@string(hide_panel, Hide Panel)" />
		</vbox>
	</hboxce>
	)xml";

	if ( mMainSplitter->getLastWidget() != nullptr ) {
		mMainSplitter->getLastWidget()->setVisible( false );
		mMainSplitter->getLastWidget()->setParent( mUISceneNode );
	}

	mContainer = mContext->getUISceneNode()
					 ->loadLayoutFromString( XML, mMainSplitter )
					 ->asType<UIHLinearLayoutCommandExecuter>();

	mContext->getStatusBar()->registerStatusBarPanel( mContainer, mContainer );

	auto editor = mContainer->find<UICodeEditor>( "app_output_output" );
	editor->getKeyBindings().addKeybindsStringUnordered( mContext->getStatusBarKeybindings() );

	editor->setLocked( true );
	editor->setLineBreakingColumn( 0 );
	editor->setShowLineNumber( false );
	editor->getDocument().reset();
	editor->setScrollY( editor->getMaxScroll().y );
	editor->setColorScheme( mContext->getSplitter()->getCurrentColorScheme() );
	mAppOutput = editor;
	mAppOutput->on( Event::OnScrollChange, [this]( auto ) {
		mScrollLocked = mAppOutput->getMaxScroll().y == mAppOutput->getScroll().y;
	} );
	mContainer->setVisible( false );

	mContainer->bind( "app_output_clear", mClearButton );
	mContainer->bind( "app_output_run", mRunButton );
	mContainer->bind( "app_output_stop", mStopButton );
	mContainer->bind( "app_output_find", mFindButton );
	mContainer->bind( "app_output_configure", mConfigureButton );

	mClearButton->onClick( [this]( auto ) {
		mAppOutput->getDocument().reset();
		mAppOutput->invalidateLongestLineWidth();
		mAppOutput->setScrollY( mAppOutput->getMaxScroll().y );
	} );

	mRunButton->onClick( [this]( auto ) {
		auto pbm = mContext->getProjectBuildManager();
		if ( nullptr == pbm )
			return;
		if ( !pbm->hasRunConfig() ) {
			UIMessageBox::New(
				UIMessageBox::OK,
				mContext->i18n( "must_configure_build_and_run_config",
								"You must first add a build and run configuration" ) )
				->setCloseShortcut( { KEY_ESCAPE } )
				->setTitle( mContext->getWindowTitle() )
				->showWhenReady()
				->on( Event::OnConfirm, [this]( auto ) {
					auto pbm = mContext->getProjectBuildManager();
					if ( nullptr != pbm )
						pbm->selectTab();
				} );
			return;
		}
		pbm->runCurrentConfig( this, false );
	} );

	mStopButton->onClick( [this]( auto ) {
		auto pbm = mContext->getProjectBuildManager();
		if ( nullptr == pbm )
			return;
		pbm->cancelRun();
	} );

	mFindButton->onClick( [this]( auto ) {
		if ( mAppOutput->getFindReplace() == nullptr || !mAppOutput->getFindReplace()->isVisible() )
			mAppOutput->showFindReplace();
		else if ( mAppOutput->getFindReplace() ) {
			mAppOutput->getFindReplace()->hide();
			mAppOutput->setFocus();
		}
	} );

	mConfigureButton->onClick( [this]( auto ) {
		auto pbm = mContext->getProjectBuildManager();
		if ( nullptr == pbm )
			return;
		pbm->editCurrentBuild();
		hide();
	} );
}

void StatusAppOutputController::updateRunButton() {
	UIPushButton* runButton = getRunButton();
	if ( runButton ) {
		runButton->runOnMainThread(
			[this, runButton] { runButton->setText( mContext->i18n( "run", "Run" ) ); } );
	}

	UIPushButton* buildAndRunButton = getBuildAndRunButton();

	if ( buildAndRunButton )
		buildAndRunButton->setEnabled( true );

	mRunButton->setEnabled( true );
	mStopButton->setEnabled( false );
}

} // namespace ecode
