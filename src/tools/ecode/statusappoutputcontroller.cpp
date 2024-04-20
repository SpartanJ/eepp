#include "statusappoutputcontroller.hpp"
#include "ecode.hpp"
#include "widgetcommandexecuter.hpp"

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
	mScrollLocked = true;

	UIPushButton* runButton = getRunButton( mApp );

	if ( runButton )
		runButton->setText( mApp->i18n( "cancel_run", "Cancel Run" ) );

	const auto updateRunButton = [this]() {
		UIPushButton* buildButton = getRunButton( mApp );
		buildButton->runOnMainThread(
			[this, buildButton] { buildButton->setText( mApp->i18n( "run", "Run" ) ); } );
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
<rellayce id="app_output" lw="mp" lh="mp" visible="false" lw="mp" lh="mp">
	<CodeEditor id="app_output_output" lw="mp" lh="mp" />
</rellayce>
	)xml";

	if ( mMainSplitter->getLastWidget() != nullptr ) {
		mMainSplitter->getLastWidget()->setVisible( false );
		mMainSplitter->getLastWidget()->setParent( mUISceneNode );
	}

	mContainer = mApp->getUISceneNode()
					 ->loadLayoutFromString( XML, mMainSplitter )
					 ->asType<UIRelativeLayoutCommandExecuter>();
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
}

} // namespace ecode
