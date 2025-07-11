#include <eepp/graphics/textureregion.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uimenubar.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#define PUGIXML_HEADER_ONLY
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIMenuBar* UIMenuBar::New() {
	return eeNew( UIMenuBar, () );
}

UIMenuBar::UIMenuBar() :
	UIWidget( "menubar" ), mMenuHeight( 0 ), mCurrentMenu( nullptr ), mWaitingUp( nullptr ) {
	if ( !( mFlags & UI_ANCHOR_RIGHT ) )
		mFlags |= UI_ANCHOR_RIGHT;

	onParentChange();

	applyDefaultTheme();
}

Uint32 UIMenuBar::getMenuIndex( UIPopUpMenu* menu ) {
	Uint32 index = 0;
	for ( const auto& [_, tmenu] : mButtons ) {
		if ( tmenu == menu )
			return index;
		index++;
	}
	return eeINDEX_NOT_FOUND;
}

UIPopUpMenu* UIMenuBar::getCurrentMenu() const {
	return mCurrentMenu;
}

void UIMenuBar::setCurrentMenu( UIPopUpMenu* currentMenu ) {
	mCurrentMenu = currentMenu;
	mWaitingUp = nullptr;
}

void UIMenuBar::showMenu( const Uint32& index ) {
	eeASSERT( index < mButtons.size() );
	auto but = mButtons[index];
	auto tbut = but.first;
	auto tpop = but.second;

	Vector2f pos( tbut->getPosition().x, tbut->getPosition().y + tbut->getSize().getHeight() );
	tpop->setPosition( pos );

	if ( !tpop->isVisible() ) {
		mCurrentMenu = tpop;
		tbut->select();
		tpop->setParent( getWindowContainer() );
		tpop->show();
		mWaitingUp = tpop;
	} else if ( mCurrentMenu != tpop || mWaitingUp == nullptr ) {
		mCurrentMenu = nullptr;
		tbut->unselect();
		tpop->hide();
	}
}

void UIMenuBar::showNextMenu() {
	if ( mCurrentMenu ) {
		auto index = getMenuIndex( mCurrentMenu );
		if ( index != eeINDEX_NOT_FOUND )
			showMenu( index + 1 < mButtons.size() ? index + 1 : 0 );
	} else if ( !mButtons.empty() ) {
		showMenu( 0 );
	}
}

void UIMenuBar::showPrevMenu() {
	if ( mCurrentMenu ) {
		auto index = getMenuIndex( mCurrentMenu );
		if ( index != eeINDEX_NOT_FOUND )
			showMenu( static_cast<int>( index ) - 1 >= 0 ? index - 1 : mButtons.size() - 1 );
	} else if ( !mButtons.empty() ) {
		showMenu( 0 );
	}
}

UIMenuBar::~UIMenuBar() {
	destroyMenues();
}

void UIMenuBar::destroyMenues() {
	if ( !SceneManager::instance()->isShuttingDown() ) {
		for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
			if ( it->second && it->second->getParent() != this ) {
				// Changing the parent ensures that the menu will be destroyed when the menubar is
				// destroyed
				it->second->setParent( this );
			}
		}
	}
}

Uint32 UIMenuBar::getType() const {
	return UI_TYPE_MENUBAR;
}

bool UIMenuBar::isType( const Uint32& type ) const {
	return UIMenuBar::getType() == type ? true : UIWidget::isType( type );
}

void UIMenuBar::addMenuButton( const String& buttonText, UIPopUpMenu* menu ) {
	UISelectButton* button = UISelectButton::NewWithTag( "menubar::button" );
	button->setParent( this );
	button->setText( buttonText );
	button->setVisible( true );
	button->setEnabled( true );
	button->on( Event::OnSizeChange, [this]( const Event* ) { refreshButtons(); } );
	button->on( Event::OnFocus, [this, button]( const Event* ) {
		if ( getEventDispatcher()->getReleaseTrigger() & EE_BUTTON_LMASK ) {
			UIPopUpMenu* menu = getMenuFromButton( button );
			if ( menu )
				menu->setFocus();
		}
	} );

	if ( menu ) {
		menu->setVisible( false );
		menu->setEnabled( false );
		menu->setOwnerNode( button );
		// This will force to change the parent when shown, and force the CSS style reload.
		menu->setParent( this );
		menu->on( Event::OnVisibleChange, [this, button]( const Event* event ) {
			if ( event->getNode()->isVisible() ) {
				button->select();
				mCurrentMenu = event->getNode()->asType<UIPopUpMenu>();
			} else if ( button->isSelected() ) {
				button->unselect();
				mCurrentMenu = nullptr;
			}
		} );
		menu->on( Event::OnItemClicked, [this]( const Event* ) {
			mWaitingUp = nullptr;
			mCurrentMenu = nullptr;
		} );
	}

	mButtons.push_back( std::make_pair( button, menu ) );

	if ( nullptr != mTheme )
		button->setThemeSkin( mTheme, "menubarbutton" );

	refreshButtons();
}

void UIMenuBar::setTheme( UITheme* theme ) {
	UIWidget::setTheme( theme );

	setThemeSkin( theme, "menubar" );

	for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		it->first->setThemeSkin( theme, "menubarbutton" );
	}

	autoHeight();
	onThemeLoaded();
}

void UIMenuBar::removeMenuButton( const String& buttonText ) {
	for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		if ( it->first->getText() == buttonText ) {
			it->first->close();
			if ( it->second )
				it->second->close();
			mButtons.erase( it );
			refreshButtons();
			break;
		}
	}
}

UISelectButton* UIMenuBar::getButton( const String& buttonText ) const {
	for ( const auto& it : mButtons )
		if ( it.first->getText() == buttonText )
			return it.first;
	return nullptr;
}

UISelectButton* UIMenuBar::getButton( const Uint32& index ) const {
	eeASSERT( index < mButtons.size() );
	return mButtons[index].first;
}

UIPopUpMenu* UIMenuBar::getPopUpMenu( const String& buttonText ) const {
	for ( const auto& it : mButtons )
		if ( it.first->getText() == buttonText )
			return it.second;
	return nullptr;
}

UIPopUpMenu* UIMenuBar::getPopUpMenu( const Uint32& index ) const {
	eeASSERT( index < mButtons.size() );
	return mButtons[index].second;
}

UIMenuBar* UIMenuBar::setPopUpMenu( const Uint32& index, UIPopUpMenu* menu ) {
	eeASSERT( index < mButtons.size() );
	mButtons[index].second = menu;
	return this;
}

size_t UIMenuBar::getButtonsCount() const {
	return mButtons.size();
}

Uint32 UIMenuBar::getMenuHeight() const {
	return mMenuHeight;
}

void UIMenuBar::setMenuHeight( const Uint32& menuHeight ) {
	mMenuHeight = menuHeight;

	if ( 0 != mMenuHeight ) {
		setSize( getParent()->getSize().getWidth(), mMenuHeight );
	} else {
		autoHeight();
	}

	refreshButtons();
}

void UIMenuBar::refreshButtons() {
	Int32 ycenter = 0;

	if ( !mButtons.empty() ) {
		UISelectButton* tbut = mButtons.begin()->first;
		Float h = mSize.getHeight();
		Float th = tbut->getPixelsSize().getHeight();
		switch ( Font::getVerticalAlign( getFlags() ) ) {
			case UI_VALIGN_CENTER:
				ycenter = eefloor( ( h - th ) * 0.5f );
				break;
			case UI_VALIGN_BOTTOM:
				ycenter = ( h - th );
				break;
			case UI_VALIGN_TOP:
				ycenter = 0;
				break;
		}
	}

	Uint32 xpos = getPixelsPadding().Left;

	for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		UISelectButton* pbut = it->first;
		pbut->setPixelsPosition( xpos, ycenter );
		xpos += pbut->getPixelsSize().getWidth() + pbut->getLayoutPixelsMargin().Left;
	}
}

bool UIMenuBar::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::Height:
			setMenuHeight( attribute.asDpDimensionUint() );
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

Uint32 UIMenuBar::onMessage( const NodeMessage* msg ) {
	switch ( msg->getMsg() ) {
		case NodeMessage::MouseUp:
			mWaitingUp = nullptr;
			break;
		case NodeMessage::MouseDown:
		case NodeMessage::MouseOver: {
			if ( msg->getSender()->isType( UI_TYPE_SELECTBUTTON ) ) {
				UISelectButton* tbut = msg->getSender()->asType<UISelectButton>();
				UIPopUpMenu* tpop = getMenuFromButton( tbut );
				if ( tpop == nullptr )
					return 1;

				Vector2f pos( tbut->getPosition().x,
							  tbut->getPosition().y + tbut->getSize().getHeight() );
				tpop->setPosition( pos );

				if ( msg->getMsg() == NodeMessage::MouseOver ) {
					if ( nullptr != mCurrentMenu && mCurrentMenu != tpop ) {
						mCurrentMenu = tpop;
						tbut->select();
						tpop->setParent( getWindowContainer() );
						tpop->show();
					}
				} else {
					if ( msg->getMsg() == NodeMessage::MouseDown &&
						 msg->getFlags() & EE_BUTTON_LMASK ) {
						if ( !tpop->isVisible() ) {
							mCurrentMenu = tpop;
							tbut->select();
							tpop->setParent( getWindowContainer() );
							tpop->show();
							mWaitingUp = tpop;
						} else if ( mCurrentMenu != tpop || mWaitingUp == nullptr ) {
							mCurrentMenu = nullptr;
							tbut->unselect();
							tpop->hide();
						}
					}
				}

				return 1;
			}

			break;
		}
		case NodeMessage::Selected: {
			for ( auto& it : mButtons ) {
				if ( it.first != msg->getSender() ) {
					it.first->unselect();
					if ( it.second )
						it.second->hide();
				}
			}
			return 1;
		}
		case NodeMessage::FocusLoss: {
			mWaitingUp = nullptr;
			if ( nullptr != getEventDispatcher() ) {
				Node* focusNode = getEventDispatcher()->getFocusNode();
				if ( !isParentOf( focusNode ) && !isPopUpMenuChild( focusNode ) ) {
					onWidgetFocusLoss();
				}
				return 1;
			}
		}
	}
	return 0;
}

void UIMenuBar::onParentChange() {
	setSize( getParent()->getSize().getWidth(), mMenuHeight );
	updateAnchorsDistances();
	UIWidget::onParentChange();
}

void UIMenuBar::onPaddingChange() {
	refreshButtons();
}

void UIMenuBar::unselectButtons() {
	for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		it->first->unselect();
	}
}

UIPopUpMenu* UIMenuBar::getMenuFromButton( UISelectButton* Button ) {
	for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it )
		if ( it->first == Button )
			return it->second;
	return nullptr;
}

bool UIMenuBar::isPopUpMenuChild( Node* node ) {
	for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it )
		if ( it->second == node || it->second->isParentOf( node ) )
			return true;
	return false;
}

void UIMenuBar::autoHeight() {
	if ( 0 == mMenuHeight ) {
		mMenuHeight = getSkinSize().getHeight();
		setSize( getParent()->getSize().getWidth(), mMenuHeight );
		updateAnchorsDistances();
	}
}

void UIMenuBar::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();
	UIWidget::loadFromXmlNode( node );

	for ( pugi::xml_node item = node.first_child(); item; item = item.next_sibling() ) {
		std::string name( item.name() );
		String::toLowerInPlace( name );

		if ( "menu" == name ) {
			std::string text( item.attribute( "text" ).as_string() );

			auto attr = item.find_attribute( []( const pugi::xml_attribute& attribute ) {
				if ( strcmp( attribute.name(), "nomenu" ) == 0 )
					return true;
				return false;
			} );

			if ( !attr.empty() ) {
				addMenuButton( getTranslatorString( text ), nullptr );
				continue;
			}

			UIPopUpMenu* subMenu = UIPopUpMenu::New();

			if ( nullptr != getDrawInvalidator() )
				subMenu->setParent( getDrawInvalidator() );

			subMenu->loadFromXmlNode( item );

			addMenuButton( getTranslatorString( text ), subMenu );
		}
	}

	endAttributesTransaction();
}

}} // namespace EE::UI
