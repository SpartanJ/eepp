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
	UIWidget( "menubar" ), mMenuHeight( 0 ), mCurrentMenu( NULL ), mWaitingUp( NULL ) {
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

void UIMenuBar::addMenuButton( const String& ButtonText, UIPopUpMenu* Menu ) {
	eeASSERT( NULL != Menu );

	UISelectButton* Button = UISelectButton::NewWithTag( "menubar::button" );
	Button->setParent( this );
	Button->setText( ButtonText );
	Button->setVisible( true );
	Button->setEnabled( true );
	Button->addEventListener( Event::OnSizeChange, [&]( const Event* ) { refreshButtons(); } );

	Menu->setVisible( false );
	Menu->setEnabled( false );
	Menu->setOwnerNode( Button );
	// This will force to change the parent when shown, and force the CSS style reload.
	Menu->setParent( this );
	Menu->addEventListener( Event::OnWidgetFocusLoss,
							cb::Make1( this, &UIMenuBar::onMenuFocusLoss ) );
	Menu->addEventListener( Event::OnHideByClick, cb::Make1( this, &UIMenuBar::onHideByClick ) );

	mButtons.push_back( std::make_pair( Button, Menu ) );

	if ( NULL != mTheme )
		Button->setThemeSkin( mTheme, "menubarbutton" );

	refreshButtons();
}

void UIMenuBar::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "menubar" );

	for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		it->first->setThemeSkin( Theme, "menubarbutton" );
	}

	autoHeight();
	onThemeLoaded();
}

void UIMenuBar::removeMenuButton( const String& ButtonText ) {
	for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		if ( it->first->getText() == ButtonText ) {
			it->first->close();
			it->second->close();

			mButtons.erase( it );

			refreshButtons();

			break;
		}
	}
}

UISelectButton* UIMenuBar::getButton( const String& ButtonText ) {
	for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		if ( it->first->getText() == ButtonText ) {
			return it->first;
		}
	}
	return NULL;
}

UIPopUpMenu* UIMenuBar::getPopUpMenu( const String& ButtonText ) {
	for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		if ( it->first->getText() == ButtonText ) {
			return it->second;
		}
	}
	return NULL;
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
			mWaitingUp = NULL;
		case NodeMessage::MouseDown:
		case NodeMessage::MouseOver: {
			if ( msg->getSender()->isType( UI_TYPE_SELECTBUTTON ) ) {
				UISelectButton* tbut = msg->getSender()->asType<UISelectButton>();
				UIPopUpMenu* tpop = getMenuFromButton( tbut );

				Vector2f pos( tbut->getPosition().x,
							  tbut->getPosition().y + tbut->getSize().getHeight() );
				tpop->setPosition( pos );

				if ( msg->getMsg() == NodeMessage::MouseOver ) {
					if ( NULL != mCurrentMenu && mCurrentMenu != tpop ) {
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
						} else if ( mCurrentMenu != tpop || mWaitingUp == NULL ) {
							mCurrentMenu = NULL;
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
			for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
				if ( it->first != msg->getSender() ) {
					it->first->unselect();
					it->second->hide();
				}
			}
			return 1;
		}
		case NodeMessage::FocusLoss: {
			mWaitingUp = NULL;
			if ( NULL != getEventDispatcher() ) {
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

	return NULL;
}

bool UIMenuBar::isPopUpMenuChild( Node* node ) {
	for ( MenuBarList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		if ( it->second == node || it->second->isParentOf( node ) ) {
			return true;
		}
	}
	return false;
}

void UIMenuBar::onMenuFocusLoss( const Event* ) {
	Node* focusNode = getEventDispatcher()->getFocusNode();
	if ( !isParentOf( focusNode ) && !isPopUpMenuChild( focusNode ) ) {
		onWidgetFocusLoss();
	}
}

void UIMenuBar::onHideByClick( const Event* ) {
	onWidgetFocusLoss();
}

void UIMenuBar::onWidgetFocusLoss() {
	UIWidget::onWidgetFocusLoss();

	if ( NULL != mCurrentMenu ) {
		mCurrentMenu->hide();

		mCurrentMenu = NULL;
	}

	unselectButtons();
}

void UIMenuBar::autoHeight() {
	if ( 0 == mMenuHeight && NULL != getSkin() ) {
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

			if ( NULL != getDrawInvalidator() )
				subMenu->setParent( getDrawInvalidator() );

			subMenu->loadFromXmlNode( item );

			if ( NULL != mSceneNode && mSceneNode->isUISceneNode() )
				addMenuButton( static_cast<UISceneNode*>( mSceneNode )->getTranslatorString( text ),
							   subMenu );
		}
	}

	endAttributesTransaction();
}

}} // namespace EE::UI
