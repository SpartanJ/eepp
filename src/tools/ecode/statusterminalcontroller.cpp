#include "statusterminalcontroller.hpp"
#include "ecode.hpp"

namespace ecode {

StatusTerminalController::StatusTerminalController( UISplitter* mainSplitter,
													UISceneNode* uiSceneNode, App* app ) :
	StatusBarElement( mainSplitter, uiSceneNode, app ), mApp( app ) {}

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

UITerminal* StatusTerminalController::createTerminal(
	const std::string& workingDir, std::string program, std::vector<std::string> args,
	const std::unordered_map<std::string, std::string>& env ) {
	Sizef initialSize( 16, 16 );
	if ( program.empty() && !mContext->termConfig().shell.empty() )
		program = mContext->termConfig().shell;

	if ( args.empty() && !mContext->termConfig().shellArgs.empty() )
		args = Process::parseArgs( mContext->termConfig().shellArgs );

	UITerminal* term = UITerminal::New(
		mContext->getTerminalFont() ? mContext->getTerminalFont() : mContext->getFontMono(),
		mContext->termConfig().fontSize.asPixels( 0, Sizef(), mContext->getDisplayDPI() ),
		initialSize, program, args, env,
		!workingDir.empty() ? workingDir : mContext->getCurrentWorkingDir(), 10000, nullptr,
		false );

	if ( term == nullptr || term->getTerm() == nullptr ) {
		mContext->getTerminalManager()->displayError( workingDir );
		return nullptr;
	}

	const auto& terminalColorSchemes = mContext->getTerminalManager()->getTerminalColorSchemes();
	const auto& currentTerminalColorScheme =
		mContext->getTerminalManager()->getTerminalCurrentColorScheme();
	auto csIt = terminalColorSchemes.find( currentTerminalColorScheme );
	term->getTerm()->getTerminal()->setAllowMemoryTrimnming( true );
	term->getTerm()->setCursorMode( mContext->termConfig().cursorStyle );
	term->setScrollViewType( mContext->termConfig().scrollBarType );
	term->setVerticalScrollMode( mContext->termConfig().scrollBarMode );

	term->setColorScheme( csIt != terminalColorSchemes.end()
							  ? terminalColorSchemes.at( currentTerminalColorScheme )
							  : TerminalColorScheme::getDefault() );
	mContext->getTerminalManager()->setKeybindings( term );
	term->setCommand( "switch-to-previous-colorscheme", [this] {
		auto it = mContext->getTerminalManager()->getTerminalColorSchemes().find(
			mContext->getTerminalManager()->getTerminalCurrentColorScheme() );
		auto prev = std::prev( it, 1 );
		if ( prev != mContext->getTerminalManager()->getTerminalColorSchemes().end() ) {
			mContext->getTerminalManager()->setTerminalColorScheme( prev->first );
		} else {
			mContext->getTerminalManager()->setTerminalColorScheme(
				mContext->getTerminalManager()->getTerminalColorSchemes().rbegin()->first );
		}
	} );
	term->setCommand( "switch-to-next-colorscheme", [this] {
		auto it = mContext->getTerminalManager()->getTerminalColorSchemes().find(
			mContext->getTerminalManager()->getTerminalCurrentColorScheme() );
		mContext->getTerminalManager()->setTerminalColorScheme(
			++it != mContext->getTerminalManager()->getTerminalColorSchemes().end()
				? it->first
				: mContext->getTerminalManager()->getTerminalColorSchemes().begin()->first );
	} );
	term->setCommand( UITerminal::getExclusiveModeToggleCommandName(), [term, this] {
		term->setExclusiveMode( !term->getExclusiveMode() );
		mApp->updateTerminalMenu();
	} );
	mContext->getSplitter()->registerSplitterCommands( *term );
	mApp->registerUnlockedCommands( *term );
	term->setFocus();
	term->setId( "terminal" );
	return term;
}

} // namespace ecode
