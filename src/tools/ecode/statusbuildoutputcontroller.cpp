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

UICodeEditor* StatusBuildOutputController::createContainer() {
	UICodeEditor* editor = UICodeEditor::NewOpt( true, true );
	editor->setLocked( true );
	editor->setLineBreakingColumn( 0 );
	editor->setShowLineNumber( false );
	return editor;
}

} // namespace ecode
