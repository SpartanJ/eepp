#include "statusappoutputcontroller.hpp"
#include "ecode.hpp"

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
													  UISceneNode* uiSceneNode, App* app ) :
	StatusBarElement( mainSplitter, uiSceneNode, app ) {}

static void safeInsertBuffer( TextDocument& doc, const std::string& buffer ) {
	doc.insert( 0, doc.endOfDoc(), buffer );
}

UIPushButton* StatusAppOutputController::getRunButton( App* app ) {
	if ( app->getSidePanel() ) {
		UIWidget* tab = app->getSidePanel()->find<UIWidget>( "build_tab_view" );
		if ( tab )
			return tab->find<UIPushButton>( "run_button" );
	}
	return nullptr;
}

void StatusAppOutputController::run( const ProjectBuildCommand& runData,
									 const ProjectBuildOutputParser& outputParser ) {
	if ( nullptr == mApp->getProjectBuildManager() )
		return;

	auto pbm = mApp->getProjectBuildManager();

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

	UIPushButton* runButton = getRunButton( mApp );

	if ( runButton )
		runButton->setText( mApp->i18n( "cancel_run", "Cancel Run" ) );

	mRunButton->setEnabled( false );
	mStopButton->setEnabled( true );

	const auto updateRunButton = [this]() {
		UIPushButton* runButton = getRunButton( mApp );
		if ( runButton ) {
			runButton->runOnMainThread(
				[this, runButton] { runButton->setText( mApp->i18n( "run", "Run" ) ); } );
		}
		mRunButton->setEnabled( true );
		mStopButton->setEnabled( false );
	};

	auto res = pbm->run(
		runData, [this]( const auto& key, const auto& def ) { return mApp->i18n( key, def ); },
		[this]( auto, std::string buffer, const ProjectBuildCommand* ) {
			mAppOutput->runOnMainThread( [this, buffer]() {
				safeInsertBuffer( mAppOutput->getDocument(), buffer );
				if ( mScrollLocked )
					mAppOutput->setScrollY( mAppOutput->getMaxScroll().y );
			} );
		},
		[this, updateRunButton]( auto exitCode, const ProjectBuildCommand* ) {
			String buffer;

			if ( EXIT_SUCCESS == exitCode ) {
				buffer = Sys::getDateTimeStr() + ": " +
						 mApp->i18n( "run_successful", "Run successfully\n" );
			} else {
				buffer =
					Sys::getDateTimeStr() + ": " + mApp->i18n( "run_failed", "Run with errors\n" );
			}

			mAppOutput->runOnMainThread( [this, buffer]() {
				safeInsertBuffer( mAppOutput->getDocument(), buffer );
				if ( mScrollLocked )
					mAppOutput->setScrollY( mAppOutput->getMaxScroll().y );
			} );

			updateRunButton();
		} );

	if ( !res.isValid() ) {
		mApp->getNotificationCenter()->addNotification( res.errorMsg );

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
	<hbox id="app_output" class="vertical_bar" lw="mp" lh="mp" visible="false">
		<rellayce id="app_command_executer" lw="0" lw8="1" lh="mp">
			<CodeEditor id="app_output_output" lw="mp" lh="mp" />
		</rellayce>
		<vbox lw="16dp" lh="mp">
			<PushButton id="app_output_clear" lw="mp" icon="icon(eraser, 12dp)" tooltip="@string(clear, Clear)" />
			<PushButton id="app_output_run" lw="mp" icon="icon(play, 12dp)" tooltip="@string(run, Run)" />
			<PushButton id="app_output_stop" lw="mp" icon="icon(stop, 12dp)" enabled="false" />
			<PushButton id="app_output_find" lw="mp" icon="icon(search, 12dp)" tooltip="@string(find, Find)" />
			<PushButton id="app_output_configure" lw="mp" icon="icon(settings, 12dp)" tooltip="@string(configure_ellipsis, Configure...)" />
		</vbox>
	</hbox>
	)xml";

	if ( mMainSplitter->getLastWidget() != nullptr ) {
		mMainSplitter->getLastWidget()->setVisible( false );
		mMainSplitter->getLastWidget()->setParent( mUISceneNode );
	}

	mContainer = mApp->getUISceneNode()
					 ->loadLayoutFromString( XML, mMainSplitter )
					 ->asType<UILinearLayout>();
	auto editor = mContainer->find<UICodeEditor>( "app_output_output" );
	editor->setLocked( true );
	editor->setLineBreakingColumn( 0 );
	editor->setShowLineNumber( false );
	editor->getDocument().reset();
	editor->setScrollY( editor->getMaxScroll().y );
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
		auto pbm = mApp->getProjectBuildManager();
		if ( nullptr == pbm )
			return;
		if ( !pbm->hasRunConfig() ) {
			UIMessageBox::New( UIMessageBox::OK,
							   mApp->i18n( "must_configure_build_and_run_config",
										   "You must first add a build and run configuration" ) )
				->setCloseShortcut( { KEY_ESCAPE } )
				->setTitle( mApp->getWindowTitle() )
				->showWhenReady()
				->on( Event::OnConfirm, [this]( auto ) {
					auto pbm = mApp->getProjectBuildManager();
					if ( nullptr != pbm )
						pbm->selectTab();
				} );
			return;
		}
		pbm->runCurrentConfig( this, false );
	} );

	mStopButton->onClick( [this]( auto ) {
		auto pbm = mApp->getProjectBuildManager();
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
		auto pbm = mApp->getProjectBuildManager();
		if ( nullptr == pbm )
			return;
		pbm->editCurrentBuild();
		hide();
	} );
}

} // namespace ecode
