#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uiicon.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/window/clipboard.hpp>
#include <eterm/ui/uiterminal.hpp>

using namespace EE::Scene;

namespace eterm { namespace UI {

UITerminal* UITerminal::New( Font* font, const Float& fontSize, const Sizef& pixelsSize,
							 std::string program, const std::vector<std::string>& args,
							 const std::string& workingDir, const size_t& historySize,
							 IProcessFactory* processFactory, const bool& useFrameBuffer ) {
	auto win = SceneManager::instance()->getUISceneNode()->getWindow();
	auto terminal =
		TerminalDisplay::create( win, font, fontSize, pixelsSize, program, args, workingDir,
								 historySize, processFactory, useFrameBuffer );
	return UITerminal::New( terminal );
}

UITerminal* UITerminal::New( const std::shared_ptr<TerminalDisplay>& terminalDisplay ) {
	return eeNew( UITerminal, ( terminalDisplay ) );
}

UITerminal::~UITerminal() {}

Uint32 UITerminal::getType() const {
	return UI_TYPE_TERMINAL;
}

bool UITerminal::isType( const Uint32& type ) const {
	return getType() == type || UIWidget::isType( type );
}

void UITerminal::draw() {
	mTerm->setPosition( mScreenPosi.asFloat() );
	mTerm->draw();
}

UITerminal::UITerminal( const std::shared_ptr<TerminalDisplay>& terminalDisplay ) :
	UIWidget( "terminal" ),
	mKeyBindings( getUISceneNode()->getWindow()->getInput() ),
	mTerm( terminalDisplay ) {
	mFlags |= UI_TAB_STOP;
	mTerm->pushEventCallback( [&]( const TerminalDisplay::Event& event ) {
		if ( event.type == TerminalDisplay::EventType::TITLE && getParent() ) {
			if ( !mIsCustomTitle && mTitle != event.eventData ) {
				mTitle = event.eventData;
				sendTextEvent( Event::OnTitleChange, mTitle );
			}
		}
	} );
	setCommand( "terminal-scroll-up-screen",
				[&] { mTerm->action( TerminalShortcutAction::SCROLLUP_SCREEN ); } );
	setCommand( "terminal-scroll-down-screen",
				[&] { mTerm->action( TerminalShortcutAction::SCROLLDOWN_SCREEN ); } );
	setCommand( "terminal-scroll-up-row",
				[&] { mTerm->action( TerminalShortcutAction::SCROLLUP_ROW ); } );
	setCommand( "terminal-scroll-down-row",
				[&] { mTerm->action( TerminalShortcutAction::SCROLLDOWN_ROW ); } );
	setCommand( "terminal-scroll-up-history",
				[&] { mTerm->action( TerminalShortcutAction::SCROLLUP_HISTORY ); } );
	setCommand( "terminal-scroll-down-history",
				[&] { mTerm->action( TerminalShortcutAction::SCROLLDOWN_HISTORY ); } );
	setCommand( "terminal-font-size-grow",
				[&] { mTerm->action( TerminalShortcutAction::FONTSIZE_GROW ); } );
	setCommand( "terminal-font-size-shrink",
				[&] { mTerm->action( TerminalShortcutAction::FONTSIZE_SHRINK ); } );
	setCommand( "terminal-paste", [&] { mTerm->action( TerminalShortcutAction::PASTE ); } );
	setCommand( "terminal-copy", [&] { mTerm->action( TerminalShortcutAction::COPY ); } );
	setCommand( "terminal-paste", [&] { mTerm->action( TerminalShortcutAction::PASTE ); } );
	subscribeScheduledUpdate();
}

const std::shared_ptr<TerminalDisplay>& UITerminal::getTerm() const {
	return mTerm;
}

void UITerminal::scheduledUpdate( const Time& ) {
	mTerm->update();
	if ( mTerm->isDirty() )
		invalidateDraw();
}

const std::string& UITerminal::getTitle() const {
	return mTitle;
}

void UITerminal::setTitle( const std::string& title ) {
	if ( title != mTitle ) {
		mTitle = title;
		mIsCustomTitle = true;
		sendTextEvent( Event::OnTitleChange, title );
	}
}

KeyBindings& UITerminal::getKeyBindings() {
	return mKeyBindings;
}

void UITerminal::setKeyBindings( const KeyBindings& keyBindings ) {
	mKeyBindings = keyBindings;
}

void UITerminal::addKeyBindingString( const std::string& shortcut, const std::string& command ) {
	mKeyBindings.addKeybindString( shortcut, command );
}

void UITerminal::addKeyBinding( const KeyBindings::Shortcut& shortcut,
								const std::string& command ) {
	mKeyBindings.addKeybind( shortcut, command );
}

void UITerminal::replaceKeyBindingString( const std::string& shortcut,
										  const std::string& command ) {
	mKeyBindings.replaceKeybindString( shortcut, command );
}

void UITerminal::replaceKeyBinding( const KeyBindings::Shortcut& shortcut,
									const std::string& command ) {
	mKeyBindings.replaceKeybind( shortcut, command );
}

void UITerminal::addKeyBindsString( const std::map<std::string, std::string>& binds ) {
	mKeyBindings.addKeybindsString( binds );
}

void UITerminal::addKeyBinds( const std::map<KeyBindings::Shortcut, std::string>& binds ) {
	mKeyBindings.addKeybinds( binds );
}

void UITerminal::execute( const std::string& command ) {
	auto cmdIt = mCommands.find( command );
	if ( cmdIt != mCommands.end() ) {
		cmdIt->second();
	}
}

void UITerminal::setCommands( const std::map<std::string, TerminalCommand>& cmds ) {
	mCommands.insert( cmds.begin(), cmds.end() );
}

void UITerminal::setCommand( const std::string& command, const UITerminal::TerminalCommand& func ) {
	mCommands[command] = func;
}

bool UITerminal::hasCommand( const std::string& command ) {
	return mCommands.find( command ) != mCommands.end();
}

bool UITerminal::getExclusiveMode() const {
	return mExclusiveMode;
}

void UITerminal::setExclusiveMode( bool exclusiveMode ) {
	mExclusiveMode = exclusiveMode;
}

Uint32 UITerminal::onTextInput( const TextInputEvent& event ) {
	mTerm->onTextInput( event.getChar() );
	return 1;
}

Uint32 UITerminal::onKeyDown( const KeyEvent& event ) {
	if ( mUISceneNode->getUIEventDispatcher()->justGainedFocus() )
		return 0;

	std::string cmd = mKeyBindings.getCommandFromKeyBind( { event.getKeyCode(), event.getMod() } );
	if ( !cmd.empty() ) {
		if ( !mExclusiveMode || cmd == getExclusiveModeToggleCommandName() )
			execute( cmd );
		return 1;
	}

	mTerm->onKeyDown( event.getKeyCode(), event.getChar(), event.getMod(), event.getScancode() );
	return 1;
}

Uint32 UITerminal::onKeyUp( const KeyEvent& ) {
	return 1;
}

Uint32 UITerminal::onMouseMove( const Vector2i& position, const Uint32& flags ) {
	mTerm->onMouseMove( position, flags );
	return 1;
}

Uint32 UITerminal::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	if ( ( flags & EE_BUTTON_LMASK ) &&
		 mTerm->getTerminal()->getSelectionMode() == TerminalSelectionMode::SEL_IDLE ) {
		mDraggingSel = true;
	} else if ( ( flags & EE_BUTTON_LMASK ) && mDraggingSel ) {
		return 1;
	}
	mTerm->onMouseDown( position, flags );
	return 1;
}

Uint32 UITerminal::onMouseDoubleClick( const Vector2i& position, const Uint32& flags ) {
	mTerm->onMouseDoubleClick( position, flags );
	return 1;
}

Uint32 UITerminal::onMouseUp( const Vector2i& position, const Uint32& flags ) {
	if ( ( flags & EE_BUTTON_LMASK ) && mDraggingSel ) {
		mDraggingSel = false;
	} else if ( flags & EE_BUTTON_RMASK ) {
		onCreateContextMenu( position, flags );
		return 1;
	}
	mTerm->onMouseUp( position, flags );
	return 1;
}

void UITerminal::onPositionChange() {
	mTerm->setPosition( mScreenPosi.asFloat() );
	UIWidget::onPositionChange();
}

void UITerminal::onSizeChange() {
	mTerm->setSize( getPixelsSize() );
	UIWidget::onSizeChange();
}

Uint32 UITerminal::onFocus() {
	mTerm->setFocus( true );
	invalidateDraw();
	return UIWidget::onFocus();
}

Uint32 UITerminal::onFocusLoss() {
	mTerm->setFocus( false );
	invalidateDraw();
	return UIWidget::onFocusLoss();
}

void UITerminal::createDefaultContextMenuOptions( UIPopUpMenu* menu ) {
	if ( !mCreateDefaultContextMenuOptions )
		return;

	menuAdd( menu, "copy", "Copy", "copy", "terminal-copy" )
		->setEnabled( mTerm->getTerminal() && mTerm->getTerminal()->hasSelection() );
	menuAdd( menu, "paste", "Paste", "paste", "terminal-paste" )
		->setEnabled( !getUISceneNode()->getWindow()->getClipboard()->getText().empty() );
}

Drawable* UITerminal::findIcon( const std::string& name ) {
	UIIcon* icon = getUISceneNode()->findIcon( name );
	if ( icon )
		return icon->getSize( mMenuIconSize );
	return nullptr;
}

UIMenuItem* UITerminal::menuAdd( UIPopUpMenu* menu, const std::string& translateKey,
								 const String& translateString, const std::string& icon,
								 const std::string& cmd ) {
	UIMenuItem* menuItem =
		menu->add( getTranslatorString( "@string/uiterminal_" + translateKey, translateString ),
				   findIcon( icon ), mKeyBindings.getCommandKeybindString( cmd ) );
	menuItem->setId( cmd );
	return menuItem;
}

bool UITerminal::onCreateContextMenu( const Vector2i& position, const Uint32& flags ) {
	if ( mCurrentMenu )
		return false;

	UIPopUpMenu* menu = UIPopUpMenu::New();

	ContextMenuEvent event( this, menu, Event::OnCreateContextMenu, position, flags );
	sendEvent( &event );

	createDefaultContextMenuOptions( menu );

	if ( menu->getCount() == 0 ) {
		menu->close();
		return false;
	}

	menu->setCloseOnHide( true );
	menu->addEventListener( Event::OnItemClicked, [&]( const Event* event ) {
		if ( !event->getNode()->isType( UI_TYPE_MENUITEM ) )
			return;
		UIMenuItem* item = event->getNode()->asType<UIMenuItem>();
		std::string txt( item->getId() );
		execute( txt );
	} );

	Vector2f pos( position.asFloat() );
	menu->nodeToWorldTranslation( pos );
	UIMenu::findBestMenuPos( pos, menu );
	menu->setPixelsPosition( pos );
	menu->show();
	menu->addEventListener( Event::OnClose, [&]( const Event* ) { mCurrentMenu = nullptr; } );
	mCurrentMenu = menu;
	return true;
}

}} // namespace eterm::UI
