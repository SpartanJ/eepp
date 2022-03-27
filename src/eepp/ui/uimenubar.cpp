#include <eepp/graphics/textureregion.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uimenubar.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
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

UIMenuBar::~UIMenuBar() {
	destroyMenues();
}

void UIMenuBar::destroyMenues() {
	if ( !SceneManager::instance()->isShootingDown() ) {
		for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
			if ( it->second->getParent() != this ) {
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
	eeASSERT( nullptr != menu );

	UISelectButton* button = UISelectButton::NewWithTag( "menubar::button" );
	button->setParent( this );
	button->setText( buttonText );
	button->setVisible( true );
	button->setEnabled( true );
	button->addEventListener( Event::OnSizeChange, [&]( const Event* ) { refreshButtons(); } );
	button->addEventListener( Event::OnFocus, [&, button]( const Event* ) {
		if ( getEventDispatcher()->getReleaseTrigger() & EE_BUTTON_LMASK ) {
			getMenuFromButton( button )->setFocus();
		}
	} );

	menu->setVisible( false );
	menu->setEnabled( false );
	menu->setOwnerNode( button );
	// This will force to change the parent when shown, and force the CSS style reload.
	menu->setParent( this );
	menu->addEventListener( Event::OnVisibleChange, [&, button]( const Event* event ) {
		if ( event->getNode()->isVisible() ) {
			button->select();
			mCurrentMenu = event->getNode()->asType<UIPopUpMenu>();
		} else if ( button->isSelected() ) {
			button->unselect();
		}
	} );
	menu->addEventListener( Event::OnItemClicked, [&]( const Event* ) {
		mWaitingUp = nullptr;
		mCurrentMenu = nullptr;
	} );

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
			it->second->close();
			mButtons.erase( it );
			refreshButtons();
			break;
		}
	}
}

UISelectButton* UIMenuBar::getButton( const String& buttonText ) {
	for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		if ( it->first->getText() == buttonText ) {
			return it->first;
		}
	}
	return nullptr;
}

UIPopUpMenu* UIMenuBar::getPopUpMenu( const String& buttonText ) {
	for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		if ( it->first->getText() == buttonText ) {
			return it->second;
		}
	}
	return nullptr;
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
				ycenter = eefloor( ( h - th ) / 2 );
				break;
			case UI_VALIGN_BOTTOM:
				ycenter = ( h - th );
				break;
			case UI_VALIGN_TOP:
				ycenter = 0;
				break;
		}
	}

	Uint32 xpos = getPadding().Left;

	for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		UISelectButton* pbut = it->first;
		pbut->setPosition( xpos, ycenter );
		xpos += pbut->getSize().getWidth() + pbut->getLayoutMargin().Left;
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
		case NodeMessage::MouseDown:
		case NodeMessage::MouseOver: {
			if ( msg->getSender()->isType( UI_TYPE_SELECTBUTTON ) ) {
				UISelectButton* tbut = msg->getSender()->asType<UISelectButton>();
				UIPopUpMenu* tpop = getMenuFromButton( tbut );

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
	for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		if ( it->first == Button ) {
			return it->second;
		}
	}

	return nullptr;
}

bool UIMenuBar::isPopUpMenuChild( Node* node ) {
	for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		if ( it->second == node || it->second->isParentOf( node ) ) {
			return true;
		}
	}
	return false;
}

void UIMenuBar::autoHeight() {
	if ( 0 == mMenuHeight && nullptr != getSkin() ) {
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
