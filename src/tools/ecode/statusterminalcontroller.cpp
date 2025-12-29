#include "statusterminalcontroller.hpp"
#include "ecode.hpp"

namespace ecode {

StatusTerminalController::StatusTerminalController( UISplitter* mainSplitter,
													UISceneNode* uiSceneNode, App* app ) :
	StatusBarElement( mainSplitter, uiSceneNode, app ), mApp( app ) {}

UIWidget* StatusTerminalController::getWidget() {
	return mContainer;
}

UIWidget* StatusTerminalController::createWidget() {
	if ( mContainer == nullptr )
		mContainer = createContainer();
	return getWidget();
}

bool StatusTerminalController::tryTabClose( UITab* tab ) {
	if ( !tab->getOwnedWidget()->isWidget() || tab->getOwnedWidget() == nullptr )
		return true;

	UIWidget* widget = tab->getOwnedWidget()->asType<UIWidget>();
	if ( widget == nullptr || widget->getData() == 0 )
		return true;

	if ( mContext->getConfig().term.warnBeforeClosingTab && widget->isType( UI_TYPE_TERMINAL ) ) {
		UITerminal* term = widget->asType<UITerminal>();
		ProcessID pid = term->getTerm()->getTerminal()->getProcess()->pid();
		if ( Sys::processHasChildren( pid ) ) {
			UIMessageBox* msgBox =
				UIMessageBox::New( UIMessageBox::OK_CANCEL,
								   mContext->i18n( "terminal_close_warn",
												   "Are you sure you want to close this "
												   "terminal?\nIt's still running a process." ) );
			msgBox->on( Event::OnConfirm, [widget]( auto ) {
				reinterpret_cast<UITab*>( widget->getData() )->removeTab();
			} );
			msgBox->on( Event::OnClose, [this]( const Event* ) { mContainer->setFocus(); } );
			msgBox->setTitle( "ecode" );
			msgBox->center();
			msgBox->showWhenReady();
			return false;
		}
	}
	return true;
}

UIHLinearLayoutCommandExecuter* StatusTerminalController::createContainer() {
	if ( mContainer )
		return mContainer;
	static const auto XML = R"xml(
		<hboxce id="terminal_panel" class="tab_widget_cont" lw="mp" lh="mp">
			<TabWidget id="terminal_panel_tab_widget" lw="0dp" lh="mp" lw8="1"
					tabbar-hide-on-single-tab="true"
					tabbar-allow-rearrange="true"
					tabbar-allow-drag-and-drop-tabs="true"
					tabbar-allow-switch-tabs-in-empty-spaces="true"
					tab-close-button-visible="true"
					tab-closable="true"></TabWidget>
			<vbox lw="16dp" lh="mp" class="vertical_bar">
				<PushButton class="expand_status_bar_panel" lw="mp" tooltip="@string(expand_panel, Expand Panel)" />
				<PushButton id="terminal_panel_add" lw="mp" icon="icon(add, 12dp)" tooltip="@string(add_terminal, Add Terminal)" />
				<Widget lw="mp" lh="0" lw8="1" />
				<PushButton class="status_bar_panel_hide" lw="mp" tooltip="@string(hide_panel, Hide Panel)" />
			</vbox>
		</hboxce>
	)xml";

	if ( mMainSplitter->getLastWidget() != nullptr ) {
		mMainSplitter->getLastWidget()->setVisible( false );
		mMainSplitter->getLastWidget()->setParent( mUISceneNode );
	}

	mContainer = mUISceneNode->loadLayoutFromString( XML, mMainSplitter )
					 ->asType<UIHLinearLayoutCommandExecuter>();
	mContainer->bind( "terminal_panel_tab_widget", mTabWidget );
	mContainer->bind( "terminal_panel_add", mAddBtn );
	mContainer->on( Event::OnFocus, [this]( auto ) {
		if ( mTabWidget->getTabSelected() && mTabWidget->getTabSelected()->getOwnedWidget() )
			mTabWidget->getTabSelected()->getOwnedWidget()->setFocus();
	} );

	mTabWidget->setAcceptsDropOfWidgetFn( []( const UIWidget* widget ) {
		return widget->isType( UI_TYPE_TAB ) &&
			   widget->asConstType<UITab>()->getOwnedWidget()->isType( UI_TYPE_TERMINAL );
	} );

	mTabWidget->setTabTryCloseCallback(
		[this]( UITab* tab, UITabWidget::FocusTabBehavior ) -> bool {
			return tryTabClose( tab );
		} );

	createTerminal();

	mAddBtn->onClick( [this]( auto ) { createTerminal(); } );

	auto kb = mContext->getKeybind( "create-new-terminal" );
	if ( !kb.empty() ) {
		mAddBtn->setTooltipText(
			String::format( "%s (%s)", mAddBtn->getTooltipText().toUtf8(), kb ) );
	}

	const auto onTabCountChange = [this]( auto ) {
		if ( SceneManager::instance()->isShuttingDown() )
			return;
		auto tabCount = mTabWidget->getTabCount();
		if ( tabCount == 0 )
			createTerminal();
	};

	mTabWidget->on( Event::OnTabAdded, onTabCountChange );
	mTabWidget->on( Event::OnTabClosed, onTabCountChange );

	mContext->getStatusBar()->registerStatusBarPanel( mContainer, mContainer );

	return mContainer;
}

UITabWidget* StatusTerminalController::getTabWidget() {
	return mTabWidget;
}

UITerminal* StatusTerminalController::getUITerminal() {
	return mTabWidget->getTabCount() > 0 &&
				   mTabWidget->getTab( 0 )->getOwnedWidget()->isType( UI_TYPE_TERMINAL )
			   ? mTabWidget->getTab( 0 )->getOwnedWidget()->asType<UITerminal>()
			   : nullptr;
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

	mApp->registerUnlockedCommands( *term );

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
	term->setCommand( UITerminal::getExclusiveModeToggleCommandName(),
					  [term] { term->setExclusiveMode( !term->getExclusiveMode() ); } );
	term->setCommand( "close-tab", [this] {
		if ( tryTabClose( mTabWidget->getTabSelected() ) )
			mTabWidget->removeTab( mTabWidget->getTabSelected() );
	} );
	term->setCommand( "create-new", [this] { createTerminal(); } );
	term->setCommand( "create-new-terminal", [this] { createTerminal(); } );
	term->setCommand( "next-tab", [this] { mTabWidget->focusNextTab(); } );
	term->setCommand( "previous-tab", [this] { mTabWidget->focusPreviousTab(); } );
	for ( int i = 1; i <= 10; i++ ) {
		term->setCommand( String::format( "switch-to-tab-%d", i ), [this, i] {
			mTabWidget->setTabSelected( eeclamp<Uint32>( i, 0, mTabWidget->getTabCount() - 1 ) );
		} );
	}
	term->setCommand( "switch-to-first-tab", [this] {
		if ( mTabWidget->getTabCount() )
			mTabWidget->setTabSelected( (Uint32)0 );
	} );
	term->setCommand( "switch-to-last-tab", [this] {
		if ( mTabWidget->getTabCount() )
			mTabWidget->setTabSelected( (Uint32)( mTabWidget->getTabCount() - 1 ) );
	} );
	term->setCommand( "terminal-rename", [this, term] {
		UIMessageBox* msgBox = UIMessageBox::New(
			UIMessageBox::INPUT, mApp->i18n( "new_terminal_name", "New terminal name:" ) );
		msgBox->setTitle( mApp->getWindowTitle() );
		msgBox->getTextInput()->setHint( mApp->i18n( "any_name_ellipsis", "Any name..." ) );
		msgBox->setCloseShortcut( { KEY_ESCAPE, KEYMOD_NONE } );
		msgBox->showWhenReady();
		msgBox->on( Event::OnConfirm, [msgBox, term]( const Event* ) {
			std::string title( msgBox->getTextInput()->getText().toUtf8() );
			term->setTitle( title );
			msgBox->close();
			term->setFocus();
		} );
	} );
	term->getTerm()->pushEventCallback( [this, term]( const TerminalDisplay::Event& event ) {
		if ( event.type == TerminalDisplay::EventType::PROCESS_EXIT &&
			 mApp->getConfig().term.closeTerminalTabOnExit && term->getData() != 0 ) {
			UITab* tab = (UITab*)term->getData();
			auto* tabWidget = mApp->getSplitter()->tabWidgetFromWidget( term );
			if ( tabWidget )
				tabWidget->removeTab( tab );
		}
	} );

	term->setCommand( "statusbar-panel-expand-contract-toggle",
					  [this] { mContext->getStatusBar()->togglePanelExpansion(); } );
	term->getKeyBindings().addKeybindsStringUnordered( mContext->getStatusBarKeybindings() );

	term->setFocus();
	term->setParent( mTabWidget );

	UIIcon* icon = mUISceneNode->findIcon( "terminal" );
	auto tab = mTabWidget->add(
		program, term, icon != nullptr ? icon->getSize( PixelDensity::dpToPxI( 12 ) ) : nullptr );

	term->setData( (UintPtr)tab );
	term->on( Event::OnTitleChange, [tab, term]( auto ) { tab->setText( term->getTitle() ); } );

	term->on( Event::OnCreateContextMenu, [this]( const Event* event ) {
		auto cevent = static_cast<const ContextMenuEvent*>( event );
		cevent->getMenu()->addSeparator();
		cevent->getMenu()
			->add( mContext->getStatusBar()->isPanelExpanded()
					   ? mContext->i18n( "contract_panel", "Contract Panel" )
					   : mContext->i18n( "expand_panel", "Expand Panel" ),
				   mContext->findIcon( "fullscreen" ) )
			->setId( "statusbar-panel-expand-contract-toggle" );
	} );

	mTabWidget->setTabSelected( tab );
	return term;
}

} // namespace ecode
