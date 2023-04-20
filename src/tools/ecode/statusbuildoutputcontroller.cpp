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

UIPushButton* StatusBuildOutputController::getBuildButton( App* app ) {
	if ( app->getSidePanel() ) {
		UIWidget* tab = app->getSidePanel()->find<UIWidget>( "build_tab" );
		if ( tab )
			return tab->find<UIPushButton>( "build_button" );
	}
	return nullptr;
}

UIPushButton* StatusBuildOutputController::getCleanButton( App* app ) {
	if ( app->getSidePanel() ) {
		UIWidget* tab = app->getSidePanel()->find<UIWidget>( "build_tab" );
		if ( tab )
			return tab->find<UIPushButton>( "clean_button" );
	}
	return nullptr;
}

void StatusBuildOutputController::runBuild( const std::string& buildName,
											const std::string& buildType,
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

	UIPushButton* buildButton = getBuildButton( mApp );
	if ( buildButton )
		buildButton->setText( mApp->i18n( "cancel_build", "Cancel Build" ) );
	UIPushButton* cleanButton = getCleanButton( mApp );
	bool enableCleanButton = false;
	if ( cleanButton && cleanButton->isEnabled() ) {
		cleanButton->setEnabled( false );
		enableCleanButton = true;
	}

	auto res = pbm->build(
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
		[this, enableCleanButton]( auto exitCode ) {
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

			UIPushButton* buildButton = getBuildButton( mApp );
			if ( buildButton )
				buildButton->setText( mApp->i18n( "build", "Build" ) );

			if ( enableCleanButton ) {
				UIPushButton* cleanButton = getCleanButton( mApp );
				if ( cleanButton )
					cleanButton->setEnabled( true );
			}
		} );

	if ( !res.isValid() ) {
		mApp->getNotificationCenter()->addNotification( res.errorMsg );
	}
}

void StatusBuildOutputController::runClean( const std::string& buildName,
											const std::string& buildType,
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

	UIPushButton* buildButton = getBuildButton( mApp );
	bool enableBuildButton = false;
	if ( buildButton && buildButton->isEnabled() ) {
		buildButton->setEnabled( false );
		enableBuildButton = true;
	}
	UIPushButton* cleanButton = getCleanButton( mApp );
	if ( cleanButton )
		cleanButton->setText( mApp->i18n( "cancel_clean", "Cancel Clean" ) );

	auto res = pbm->clean(
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
		[this, enableBuildButton]( auto exitCode ) {
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

			UIPushButton* cleanButton = getCleanButton( mApp );
			if ( cleanButton )
				cleanButton->setText( mApp->i18n( "clean", "Clean" ) );

			if ( enableBuildButton ) {
				UIPushButton* buildButton = getBuildButton( mApp );
				if ( buildButton )
					buildButton->setEnabled( true );
			}
		} );

	if ( !res.isValid() ) {
		mApp->getNotificationCenter()->addNotification( res.errorMsg );
	}
}

UICodeEditor* StatusBuildOutputController::getContainer() {
	return mContainer;
}

UICodeEditor* StatusBuildOutputController::createContainer() {
	UICodeEditor* editor = UICodeEditor::NewOpt( true, true );
	editor->setLocked( true );
	editor->setLineBreakingColumn( 0 );
	editor->setShowLineNumber( false );
	return editor;
}

} // namespace ecode
