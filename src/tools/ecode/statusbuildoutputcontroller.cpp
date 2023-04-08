#include "statusbuildoutputcontroller.hpp"
#include "ecode.hpp"

namespace ecode {

StatusBuildOutputController::StatusBuildOutputController( UISplitter* mainSplitter,
														  UISceneNode* uiSceneNode, App* app ) :
	mMainSplitter( mainSplitter ),
	mUISceneNode( uiSceneNode ),
	mApp( app ),
	mSplitter( mApp->getSplitter() ) {}

void StatusBuildOutputController::toggle() {
	if ( nullptr == mContainer ) {
		show();
		return;
	}

	if ( mMainSplitter->getLastWidget() != nullptr ) {
		if ( mMainSplitter->getLastWidget() == mContainer ) {
			hide();
		} else {
			show();
		}
	} else {
		show();
	}
}

void StatusBuildOutputController::hide() {
	if ( mContainer && mContainer->isVisible() ) {
		mContainer->setParent( mUISceneNode );
		mContainer->setVisible( false );
		mApp->getStatusBar()->updateState();
		if ( mSplitter->getCurWidget() )
			mSplitter->getCurWidget()->setFocus();
	}
}

void StatusBuildOutputController::show() {
	if ( nullptr == mContainer ) {
		mMainSplitter->updateLayout();
		mContainer = createContainer();
		mContainer->setId( "build_output" );
		mContainer->setVisible( false );
	}

	if ( !mContainer->isVisible() ) {
		mApp->hideLocateBar();
		mApp->hideSearchBar();
		mApp->hideGlobalSearchBar();
		if ( mMainSplitter->getLastWidget() != nullptr ) {
			mMainSplitter->getLastWidget()->setVisible( false );
			mMainSplitter->getLastWidget()->setParent( mUISceneNode );
		}
		mContainer->setParent( mMainSplitter );
		mContainer->setVisible( true );
		mContainer->setFocus();
		mApp->getStatusBar()->updateState();
	}
}

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

void StatusBuildOutputController::run( const std::string& buildName, const std::string& buildType,
									   const ProjectBuildOutputParser& outputParser ) {
	if ( !mApp->getProjectBuildManager() )
		return;

	auto pbm = mApp->getProjectBuildManager();

	show();

	mContainer->getDocument().reset();
	mContainer->setScrollY( mContainer->getMaxScroll().y );

	std::vector<SyntaxPattern> patterns;

	for ( const auto& parser : outputParser.getConfig() ) {
		SyntaxPattern ptn( { parser.pattern }, getProjectOutputParserTypeToString( parser.type ) );
		patterns.emplace_back( std::move( ptn ) );
	}

	patterns.emplace_back(
		SyntaxPattern( { "%d%d%d%d%-%d%d%-%d%d%s%d%d%:%d%d%:%d%d%:.*error.*[^\n]+" }, "error" ) );
	patterns.emplace_back( SyntaxPattern(
		{ "%d%d%d%d%-%d%d%-%d%d%s%d%d%:%d%d%:%d%d%:.*warning.*[^\n]+" }, "warning" ) );
	patterns.emplace_back(
		SyntaxPattern( { "%d%d%d%d%-%d%d%-%d%d%s%d%d%:%d%d%:%d%d%:[^\n]+" }, "notice" ) );

	SyntaxDefinition synDef( "custom_build", {}, patterns );

	mContainer->getDocument().setSyntaxDefinition( synDef );
	mContainer->getVScrollBar()->setValue( 1.f );

	auto res = pbm->run(
		buildName, [this]( const auto& key, const auto& def ) { return mApp->i18n( key, def ); },
		buildType,
		[this]( auto, auto buffer ) {
			mContainer->runOnMainThread( [this, buffer]() {
				bool scrollToBottom = mContainer->getVScrollBar()->getValue() == 1.f;
				mContainer->getDocument().textInput( buffer );
				if ( scrollToBottom )
					mContainer->setScrollY( mContainer->getMaxScroll().y );
			} );
		},
		[this]( auto exitCode ) {
			String buffer;

			if ( EXIT_SUCCESS == exitCode ) {
				buffer = Sys::getDateTimeStr() + ": " +
						 mApp->i18n( "build_successful", "Build run successfully\n" );
			} else {
				buffer = Sys::getDateTimeStr() + ": " +
						 mApp->i18n( "build_failed", "Build run with errors\n" );
			}

			mContainer->runOnMainThread( [this, buffer]() {
				bool scrollToBottom = mContainer->getVScrollBar()->getValue() == 1.f;
				mContainer->getDocument().textInput( buffer );
				if ( scrollToBottom )
					mContainer->setScrollY( mContainer->getMaxScroll().y );
			} );
		} );

	if ( !res.isValid() ) {
		mApp->getNotificationCenter()->addNotification( res.errorMsg );
	}
}

UICodeEditor* StatusBuildOutputController::createContainer() {
	UICodeEditor* editor = UICodeEditor::NewOpt( true, true );
	editor->setLocked( true );
	editor->setLineBreakingColumn( 0 );
	editor->setShowLineNumber( false );
	return editor;
}

} // namespace ecode
