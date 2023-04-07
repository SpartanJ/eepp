#include "statusterminalcontroller.hpp"
#include "ecode.hpp"

namespace ecode {

StatusTerminalController::StatusTerminalController( UISplitter* mainSplitter,
													UISceneNode* uiSceneNode, App* app ) :
	mMainSplitter( mainSplitter ),
	mUISceneNode( uiSceneNode ),
	mApp( app ),
	mSplitter( mApp->getSplitter() ) {}

void StatusTerminalController::toggle() {
	if ( nullptr == mUITerminal ) {
		show();
		return;
	}

	if ( mMainSplitter->getLastWidget() != nullptr ) {
		if ( mMainSplitter->getLastWidget() == mUITerminal ) {
			hide();
		} else {
			show();
		}
	} else {
		show();
	}
}

void StatusTerminalController::hide() {
	if ( mUITerminal && mUITerminal->isVisible() ) {
		mUITerminal->setParent( mUISceneNode );
		mUITerminal->setVisible( false );
		mApp->getStatusBar()->updateState();
		if ( mSplitter->getCurWidget() )
			mSplitter->getCurWidget()->setFocus();
	}
}

void StatusTerminalController::show() {
	if ( nullptr == mUITerminal ) {
		mMainSplitter->updateLayout();
		mUITerminal = createTerminal();
		mUITerminal->setId( "terminal" );
		mUITerminal->setVisible( false );
	}

	if ( !mUITerminal->isVisible() ) {
		mApp->hideLocateBar();
		mApp->hideSearchBar();
		mApp->hideGlobalSearchBar();
		if ( mMainSplitter->getLastWidget() != nullptr ) {
			mMainSplitter->getLastWidget()->setVisible( false );
			mMainSplitter->getLastWidget()->setParent( mUISceneNode );
		}
		mUITerminal->setParent( mMainSplitter );
		mUITerminal->setVisible( true );
		mUITerminal->setFocus();
		mApp->getStatusBar()->updateState();
	}
}

UITerminal* StatusTerminalController::createTerminal( const std::string& workingDir,
													  std::string program,
													  const std::vector<std::string>& args ) {
	Sizef initialSize( 16, 16 );
	if ( program.empty() && !mApp->termConfig().shell.empty() )
		program = mApp->termConfig().shell;

	UITerminal* term = UITerminal::New(
		mApp->getTerminalFont() ? mApp->getTerminalFont() : mApp->getFontMono(),
		mApp->termConfig().fontSize.asPixels( 0, Sizef(), mApp->getDisplayDPI() ), initialSize,
		program, args, !workingDir.empty() ? workingDir : mApp->getCurrentWorkingDir(), 10000,
		nullptr, false );
	if ( term->getTerm() == nullptr ) {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::OK,
			mApp->i18n( "feature_not_supported_in_os",
						"This feature is not supported in this Operating System" ) );
		msgBox->showWhenReady();
		return nullptr;
	}

	const auto& terminalColorSchemes = mApp->getTerminalManager()->getTerminalColorSchemes();
	const auto& currentTerminalColorScheme =
		mApp->getTerminalManager()->getTerminalCurrentColorScheme();
	auto csIt = terminalColorSchemes.find( currentTerminalColorScheme );
	term->getTerm()->getTerminal()->setAllowMemoryTrimnming( true );
	term->setColorScheme( csIt != terminalColorSchemes.end()
							  ? terminalColorSchemes.at( currentTerminalColorScheme )
							  : TerminalColorScheme::getDefault() );
	term->addKeyBinds( mApp->getLocalKeybindings() );
	term->addKeyBinds( UICodeEditorSplitter::getLocalDefaultKeybindings() );
	term->addKeyBinds( { { { KEY_E, KEYMOD_CTRL | KEYMOD_LALT | KEYMOD_SHIFT },
						   UITerminal::getExclusiveModeToggleCommandName() } } );
	// Remove the keybinds that are problematic for a terminal
	term->getKeyBindings().removeCommandsKeybind(
		{ "open-file", "download-file-web", "open-folder", "debug-draw-highlight-toggle",
		  "debug-draw-boxes-toggle", "debug-draw-debug-data", "debug-widget-tree-view",
		  "open-locatebar", "open-command-palette", "open-global-search", "menu-toggle",
		  "console-toggle", "go-to-line" } );
	term->setCommand( "switch-to-previous-colorscheme", [&] {
		auto it = mApp->getTerminalManager()->getTerminalColorSchemes().find(
			mApp->getTerminalManager()->getTerminalCurrentColorScheme() );
		auto prev = std::prev( it, 1 );
		if ( prev != mApp->getTerminalManager()->getTerminalColorSchemes().end() ) {
			mApp->getTerminalManager()->setTerminalColorScheme( prev->first );
		} else {
			mApp->getTerminalManager()->setTerminalColorScheme(
				mApp->getTerminalManager()->getTerminalColorSchemes().rbegin()->first );
		}
	} );
	term->setCommand( "switch-to-next-colorscheme", [&] {
		auto it = mApp->getTerminalManager()->getTerminalColorSchemes().find(
			mApp->getTerminalManager()->getTerminalCurrentColorScheme() );
		mApp->getTerminalManager()->setTerminalColorScheme(
			++it != mApp->getTerminalManager()->getTerminalColorSchemes().end()
				? it->first
				: mApp->getTerminalManager()->getTerminalColorSchemes().begin()->first );
	} );
	term->setCommand( UITerminal::getExclusiveModeToggleCommandName(), [term, this] {
		term->setExclusiveMode( !term->getExclusiveMode() );
		mApp->updateTerminalMenu();
	} );
	mApp->registerUnlockedCommands( *term );
	mApp->getSplitter()->registerSplitterCommands( *term );
	term->setFocus();
	return term;
}

} // namespace ecode
