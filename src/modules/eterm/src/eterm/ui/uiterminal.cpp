#include <eepp/scene/scenemanager.hpp>
#include <eepp/system/luapattern.hpp>
#include <eepp/ui/uieventdispatcher.hpp>
#include <eepp/ui/uiicon.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uitabwidget.hpp>
#include <eepp/window/clipboard.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#include <eterm/ui/uiterminal.hpp>

using namespace EE::Scene;

namespace eterm { namespace UI {

UITerminal* UITerminal::New( Font* font, const Float& fontSize, const Sizef& pixelsSize,
							 const std::string& program, const std::vector<std::string>& args,
							 const std::unordered_map<std::string, std::string>& env,
							 const std::string& workingDir, const size_t& historySize,
							 IProcessFactory* processFactory, bool useFrameBuffer,
							 bool keepAlive ) {
	auto win = SceneManager::instance()->getUISceneNode()->getWindow();
	auto terminal =
		TerminalDisplay::create( win, font, fontSize, pixelsSize, program, args, workingDir,
								 historySize, processFactory, useFrameBuffer, keepAlive, env );
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
	if ( mTerm ) {
		mTerm->setPosition( mScreenPos.trunc() );
		mTerm->draw();
	}
}

UITerminal::UITerminal( const std::shared_ptr<TerminalDisplay>& terminalDisplay ) :
	UIWidget( "terminal" ),
	mKeyBindings( getInput() ),
	mVScroll( UIScrollBar::NewVertical() ),
	mTerm( terminalDisplay ) {
	mFlags |= UI_TAB_STOP | UI_SCROLLABLE;
	if ( !terminalDisplay )
		return;
	mTerm->pushEventCallback( [this]( const TerminalDisplay::Event& event ) {
		switch ( event.type ) {
			case TerminalDisplay::EventType::TITLE: {
				if ( !mIsCustomTitle && mTitle != event.eventData ) {
					mTitle = event.eventData;
					sendTextEvent( Event::OnTitleChange, mTitle );
				}
				break;
			}
			case TerminalDisplay::EventType::HISTORY_LENGTH_CHANGE: {
				if ( !mTerm->getTerminal()->tisaltscr() )
					onContentSizeChange();
				break;
			}
			case TerminalDisplay::EventType::SCROLL_HISTORY: {
				updateScrollPosition();
				break;
			}
			default: {
			}
		}
	} );

	mVScroll->setParent( this );
	mVScroll->on( Event::OnValueChange, [this]( const Event* ) { updateScroll(); } );

	setCommand( "terminal-scroll-up-screen",
				[this] { mTerm->action( TerminalShortcutAction::SCROLLUP_SCREEN ); } );
	setCommand( "terminal-scroll-down-screen",
				[this] { mTerm->action( TerminalShortcutAction::SCROLLDOWN_SCREEN ); } );
	setCommand( "terminal-scroll-up-row",
				[this] { mTerm->action( TerminalShortcutAction::SCROLLUP_ROW ); } );
	setCommand( "terminal-scroll-down-row",
				[this] { mTerm->action( TerminalShortcutAction::SCROLLDOWN_ROW ); } );
	setCommand( "terminal-scroll-up-history",
				[this] { mTerm->action( TerminalShortcutAction::SCROLLUP_HISTORY ); } );
	setCommand( "terminal-scroll-down-history",
				[this] { mTerm->action( TerminalShortcutAction::SCROLLDOWN_HISTORY ); } );
	setCommand( "terminal-font-size-grow",
				[this] { mTerm->action( TerminalShortcutAction::FONTSIZE_GROW ); } );
	setCommand( "terminal-font-size-shrink",
				[this] { mTerm->action( TerminalShortcutAction::FONTSIZE_SHRINK ); } );
	setCommand( "terminal-paste", [this] { mTerm->action( TerminalShortcutAction::PASTE ); } );
	setCommand( "terminal-copy", [this] { mTerm->action( TerminalShortcutAction::COPY ); } );
	setCommand( "terminal-open-link",
				[this] { Engine::instance()->openURI( mTerm->getTerminal()->getSelection() ); } );
	subscribeScheduledUpdate();
}

int UITerminal::getContentSize() const {
	if ( mTerm && mTerm->getTerminal() )
		return mTerm->getTerminal()->getHistorySize() + mTerm->getTerminal()->getNumRows();
	return 0;
}

void UITerminal::onContentSizeChange() {
	int contentSize( getContentSize() );
	int visibleArea( getVisibleArea() );

	if ( ScrollBarMode::AlwaysOn == mVScrollMode ) {
		mVScroll->setVisible( true )->setEnabled( true );
	} else if ( ScrollBarMode::AlwaysOff == mVScrollMode ) {
		mVScroll->setVisible( false )->setEnabled( false );
	} else {
		bool visible = !mTerm->isAltScr() && contentSize > visibleArea;
		mVScroll->setVisible( visible )->setEnabled( visible );
	}

	mVScroll->setPixelsPosition( getPixelsSize().getWidth() - mVScroll->getPixelsSize().getWidth() -
									 mPaddingPx.Right,
								 mPaddingPx.Top );

	mVScroll->setPixelsSize( mVScroll->getPixelsSize().getWidth(),
							 getPixelsSize().getHeight() - mPaddingPx.Top - mPaddingPx.Bottom );

	mVScroll->setPageStep( visibleArea / (Float)contentSize );

	updateScrollPosition();
	updateScroll();
}

const ScrollBarMode& UITerminal::getVerticalScrollMode() const {
	return mVScrollMode;
}

const UITerminal::ScrollViewType& UITerminal::getViewType() const {
	return mViewType;
}

void UITerminal::setViewType( const ScrollViewType& viewType ) {
	if ( viewType != mViewType ) {
		mViewType = viewType;
		onContentSizeChange();
	}
}

UIScrollBar* UITerminal::getVerticalScrollBar() const {
	return mVScroll;
}

void UITerminal::onAlphaChange() {
	UIWidget::onAlphaChange();
	mVScroll->setAlpha( mAlpha );
}

void UITerminal::onPaddingChange() {
	mTerm->setPadding(
		{ mPaddingPx.Left, mPaddingPx.Top,
		  mPaddingPx.Right +
			  ( mViewType == Exclusive ? mVScroll->getPixelsSize().getWidth() : 0.f ),
		  mPaddingPx.Bottom } );
	onContentSizeChange();
	UIWidget::onPaddingChange();
}

int UITerminal::getVisibleArea() const {
	return ( mTerm && mTerm->getTerminal() ) ? mTerm->getTerminal()->getNumRows() : 0;
}

void UITerminal::updateScrollPosition() {
	if ( mTerm && mTerm->getTerminal() )
		mVScroll->setValue( 1.f - mTerm->getTerminal()->scrollPos() /
									  (Float)mTerm->getTerminal()->getHistorySize(),
							false );
}

int UITerminal::getScrollableArea() const {
	int contentSize( getContentSize() );
	int size( getVisibleArea() );
	return contentSize - size;
}

void UITerminal::updateScroll() {
	int totalScroll = getScrollableArea();
	int initScroll( mScrollOffset );
	mScrollOffset = 0;

	if ( mVScroll->isVisible() && totalScroll > 0 )
		mScrollOffset = totalScroll * mVScroll->getValue();

	if ( initScroll != mScrollOffset )
		onScrollChange();
}

void UITerminal::onScrollChange() {
	if ( !mTerm || !mTerm->getTerminal() )
		return;
	int scrollTo = ( getScrollableArea() - mScrollOffset );
	TerminalArg arg( scrollTo );
	mTerm->getTerminal()->kscrollto( &arg );
}

void UITerminal::setVerticalScrollMode( const ScrollBarMode& Mode ) {
	if ( Mode != mVScrollMode ) {
		mVScrollMode = Mode;
		onContentSizeChange();
	}
}

std::string UITerminal::getPropertyString( const PropertyDefinition* propertyDef,
										   const Uint32& propertyIndex ) const {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::VScrollMode:
			return getVerticalScrollMode() == ScrollBarMode::Auto
					   ? "auto"
					   : ( getVerticalScrollMode() == ScrollBarMode::AlwaysOn ? "on" : "off" );
		case PropertyId::ScrollBarStyle:
			return mVScroll->getScrollBarType() == UIScrollBar::NoButtons ? "no-buttons"
																		  : "two-buttons";
		case PropertyId::ScrollBarMode:
			return getViewType() == Inclusive ? "inclusive" : "exclusive";
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

std::vector<PropertyId> UITerminal::getPropertiesImplemented() const {
	auto props = UIWidget::getPropertiesImplemented();
	auto local = { PropertyId::VScrollMode, PropertyId::ScrollBarStyle, PropertyId::ScrollBarMode };
	props.insert( props.end(), local.begin(), local.end() );
	return props;
}

void UITerminal::executeFile( const std::string& cmd ) {
	if ( mTerm )
		mTerm->executeFile( cmd );
}

void UITerminal::executeBinary( const std::string& binaryPath, const std::string& args ) {
	if ( mTerm )
		mTerm->executeBinary( binaryPath, args );
}

const TerminalColorScheme& UITerminal::getColorScheme() const {
	return mTerm->getColorScheme();
}

void UITerminal::setColorScheme( const TerminalColorScheme& colorScheme ) {
	mTerm->setColorScheme( colorScheme );
}

bool UITerminal::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::ScrollBarMode: {
			std::string val( attribute.asString() );
			String::toLowerInPlace( val );
			if ( "inclusive" == val || "inside" == val )
				setViewType( Inclusive );
			else if ( "exclusive" == val || "outside" == val )
				setViewType( Exclusive );
			break;
		}
		case PropertyId::VScrollMode: {
			std::string val( attribute.asString() );
			String::toLowerInPlace( val );

			if ( "on" == val )
				setVerticalScrollMode( ScrollBarMode::AlwaysOn );
			else if ( "off" == val )
				setVerticalScrollMode( ScrollBarMode::AlwaysOn );
			else if ( "auto" == val )
				setVerticalScrollMode( ScrollBarMode::Auto );
			break;
		}
		case PropertyId::ScrollBarStyle: {
			std::string val( attribute.asString() );
			String::toLowerInPlace( val );

			if ( "no-buttons" == val || "nobuttons" == val ) {
				mVScroll->setScrollBarStyle( UIScrollBar::NoButtons );
			} else if ( "two-buttons" == val || "twobuttons" == val ) {
				mVScroll->setScrollBarStyle( UIScrollBar::TwoButtons );
			}
			break;
		}
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

const std::shared_ptr<TerminalDisplay>& UITerminal::getTerm() const {
	return mTerm;
}

void UITerminal::scheduledUpdate( const Time& ) {
	if ( !mTerm )
		return;
	mTerm->update();

	if ( mTerm->isDirty() && isVisible() )
		invalidateDraw();

	if ( ScrollBarMode::AlwaysOn == mVScrollMode ) {
		mVScroll->setVisible( !mTerm->getTerminal()->tisaltscr() )
			->setEnabled( !mTerm->getTerminal()->tisaltscr() );
	} else if ( ScrollBarMode::Auto == mVScrollMode ) {
		if ( mViewType == Inclusive && mMouseClock.getElapsedTime() > Seconds( 1 ) &&
			 !mVScroll->isDragging() )
			mVScroll->setVisible( false )->setEnabled( false );
	}
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

Float UITerminal::getFontSize() const {
	return mTerm->getFontSize();
}

void UITerminal::setFontSize( Float fontSize ) {
	mTerm->setFontSize( fontSize );
}

Font* UITerminal::getFont() const {
	return mTerm->getFont();
}

void UITerminal::setFont( Font* font ) {
	mTerm->setFont( font );
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

bool UITerminal::isUsingCustomTitle() const {
	return mIsCustomTitle;
}

Uint32 UITerminal::onTextInput( const TextInputEvent& event ) {
	Input* input = getInput();

	if ( ( input->isLeftAltPressed() && !event.getText().empty() && event.getText()[0] == '\t' ) ||
		 ( input->isLeftControlPressed() && !input->isLeftAltPressed() &&
		   !input->isAltGrPressed() ) ||
		 input->isMetaPressed() || ( input->isLeftAltPressed() && !input->isLeftControlPressed() ) )
		return 0;

	mTerm->onTextInput( event.getChar() );
	return 1;
}

Uint32 UITerminal::onTextEditing( const TextEditingEvent& event ) {
	UIWidget::onTextEditing( event );
	if ( mTerm ) {
		mTerm->onTextEditing( event.getText(), event.getStart(), event.getLength() );
		invalidateDraw();
		return 1;
	}
	return 0;
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
	if ( mViewType == Inclusive && ScrollBarMode::Auto == mVScrollMode ) {
		mMouseClock.restart();
		bool visible = !mTerm->getTerminal()->tisaltscr() && getContentSize() > getVisibleArea() &&
					   !mTerm->getTerminal()->hasSelection();
		mVScroll->setVisible( visible )->setEnabled( visible );
	}

	if ( getUISceneNode()->getUIEventDispatcher()->isNodeDragging() )
		return 0;

	mTerm->onMouseMove( position, flags );
	return 1;
}

Uint32 UITerminal::onMouseDown( const Vector2i& position, const Uint32& flags ) {
	if ( getUISceneNode()->getUIEventDispatcher()->isNodeDragging() )
		return 0;

	mTerm->onMouseDown( position, flags );
	return 1;
}

Uint32 UITerminal::onMouseDoubleClick( const Vector2i& position, const Uint32& flags ) {
	mTerm->onMouseDoubleClick( position, flags );
	return 1;
}

Uint32 UITerminal::onMouseUp( const Vector2i& position, const Uint32& flags ) {
	if ( flags & EE_BUTTON_RMASK ) {
		onCreateContextMenu( position, flags );
		return 1;
	}
	mTerm->onMouseUp( position, flags );
	return 1;
}

void UITerminal::onPositionChange() {
	mTerm->setPosition( mScreenPos.trunc() );
	UIWidget::onPositionChange();
}

void UITerminal::onSizeChange() {
	mTerm->setSize( { getPixelsSize().getWidth(), getPixelsSize().getHeight() } );
	mTerm->setPadding(
		{ mPaddingPx.Left, mPaddingPx.Top,
		  mPaddingPx.Right +
			  ( mViewType == Exclusive ? mVScroll->getPixelsSize().getWidth() : 0.f ),
		  mPaddingPx.Bottom } );
	onContentSizeChange();
	UIWidget::onSizeChange();
}

Uint32 UITerminal::onFocus( NodeFocusReason reason ) {
	getUISceneNode()->getWindow()->startTextInput();
	updateScreenPos();
	mTerm->setPosition( mScreenPos.trunc() );
	mTerm->setFocus( true );
	invalidateDraw();
	return UIWidget::onFocus( reason );
}

Uint32 UITerminal::onFocusLoss() {
	getUISceneNode()->getWindow()->stopTextInput();
	mTerm->setFocus( false );
	invalidateDraw();
	return UIWidget::onFocusLoss();
}

void UITerminal::createDefaultContextMenuOptions( UIPopUpMenu* menu ) {
	if ( !mCreateDefaultContextMenuOptions )
		return;

	if ( mTerm->getTerminal()->hasSelection() ) {
		auto sel( mTerm->getTerminal()->getSelection() );

		if ( LuaPattern::hasMatches( sel, LuaPattern::getURIPattern() ) ) {
			menuAdd( menu, i18n( "uiterminal_open_link", "Open Link" ), "earth",
					 "terminal-open-link" );
		}
	}

	menuAdd( menu, i18n( "uiterminal_copy", "Copy" ), "copy", "terminal-copy" )
		->setEnabled( mTerm->getTerminal() && mTerm->getTerminal()->hasSelection() );
	menuAdd( menu, i18n( "uiterminal_paste", "Paste" ), "paste", "terminal-paste" )
		->setEnabled( !getUISceneNode()->getWindow()->getClipboard()->getText().empty() );
}

Drawable* UITerminal::findIcon( const std::string& name ) {
	UIIcon* icon = getUISceneNode()->findIcon( name );
	if ( icon )
		return icon->getSize( mMenuIconSize );
	return nullptr;
}

UIMenuItem* UITerminal::menuAdd( UIPopUpMenu* menu, const String& translateString,
								 const std::string& icon, const std::string& cmd ) {
	UIMenuItem* menuItem =
		menu->add( translateString, findIcon( icon ), mKeyBindings.getCommandKeybindString( cmd ) );
	menuItem->setId( cmd );
	return menuItem;
}

bool UITerminal::onCreateContextMenu( const Vector2i& position, const Uint32& flags ) {
	if ( mCurrentMenu )
		return false;

	UIPopUpMenu* menu = UIPopUpMenu::New();

	createDefaultContextMenuOptions( menu );

	ContextMenuEvent event( this, menu, Event::OnCreateContextMenu, position, flags );
	sendEvent( &event );

	if ( menu->getCount() == 0 ) {
		menu->close();
		return false;
	}

	menu->setCloseOnHide( true );
	menu->on( Event::OnItemClicked, [this]( const Event* event ) {
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
	menu->on( Event::OnClose, [this]( const Event* ) { mCurrentMenu = nullptr; } );
	mCurrentMenu = menu;
	return true;
}

}} // namespace eterm::UI
