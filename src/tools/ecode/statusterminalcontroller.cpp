#include "statusterminalcontroller.hpp"
#include "ecode.hpp"

namespace ecode {

StatusTerminalController::StatusTerminalController( UISplitter* mainSplitter,
													UISceneNode* uiSceneNode, App* app ) :
	StatusBarElement( mainSplitter, uiSceneNode, app ) {}

UIWidget* StatusTerminalController::getWidget() {
	return mUITerminal;
}

UIWidget* StatusTerminalController::createWidget() {
	if ( mUITerminal == nullptr )
		mUITerminal = createTerminal();
	return getWidget();
}

UITerminal* StatusTerminalController::getUITerminal() {
	return mUITerminal;
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
	mApp->getTerminalManager()->setKeybindings( term );
	term->setCommand( "switch-to-previous-colorscheme", [this] {
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
	term->setCommand( "switch-to-next-colorscheme", [this] {
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
	term->setId( "terminal" );
	return term;
}

} // namespace ecode
