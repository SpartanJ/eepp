#include <eepp/graphics/textureregion.hpp>
#include <eepp/scene/scenemanager.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <eepp/ui/uiwinmenu.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIWinMenu* UIWinMenu::New() {
	return eeNew( UIWinMenu, () );
}

UIWinMenu::UIWinMenu() : UIWidget( "winmenu" ), mMenuHeight( 0 ), mCurrentMenu( NULL ) {
	if ( !( mFlags & UI_ANCHOR_RIGHT ) )
		mFlags |= UI_ANCHOR_RIGHT;

	onParentChange();

	applyDefaultTheme();
}

UIWinMenu::~UIWinMenu() {
	destroyMenues();
}

void UIWinMenu::destroyMenues() {
	if ( !SceneManager::instance()->isShootingDown() ) {
		for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
			if ( it->second->getParent() != this ) {
				// Changing the parent ensures that the menu will be destroyed when the win menu is
				// destroyed
				it->second->setParent( this );
			}
		}
	}
}

Uint32 UIWinMenu::getType() const {
	return UI_TYPE_WINMENU;
}

bool UIWinMenu::isType( const Uint32& type ) const {
	return UIWinMenu::getType() == type ? true : UIWidget::isType( type );
}

void UIWinMenu::addMenuButton( const String& ButtonText, UIPopUpMenu* Menu ) {
	eeASSERT( NULL != Menu );

	UISelectButton* Button = UISelectButton::NewWithTag( "winmenu::button" );
	Button->setParent( this );
	Button->setText( ButtonText );
	Button->setVisible( true );
	Button->setEnabled( true );
	Button->addEventListener( Event::OnSizeChange, [&]( const Event* ) { refreshButtons(); } );

	Menu->setVisible( false );
	Menu->setEnabled( false );
	// This will force to change the parent when shown, and force the CSS style reload.
	Menu->setParent( this );
	Menu->addEventListener( Event::OnWidgetFocusLoss,
							cb::Make1( this, &UIWinMenu::onMenuFocusLoss ) );
	Menu->addEventListener( Event::OnHideByClick, cb::Make1( this, &UIWinMenu::onHideByClick ) );

	mButtons.push_back( std::make_pair( Button, Menu ) );

	if ( NULL != mTheme )
		Button->setThemeSkin( mTheme, "winmenubutton" );

	refreshButtons();
}

void UIWinMenu::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "winmenu" );

	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		it->first->setThemeSkin( Theme, "winmenubutton" );
	}

	autoHeight();
	onThemeLoaded();
}

void UIWinMenu::removeMenuButton( const String& ButtonText ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		if ( it->first->getText() == ButtonText ) {
			it->first->close();
			it->second->close();

			mButtons.erase( it );

			refreshButtons();

			break;
		}
	}
}

UISelectButton* UIWinMenu::getButton( const String& ButtonText ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		if ( it->first->getText() == ButtonText ) {
			return it->first;
		}
	}

	return NULL;
}

UIPopUpMenu* UIWinMenu::getPopUpMenu( const String& ButtonText ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		if ( it->first->getText() == ButtonText ) {
			return it->second;
		}
	}

	return NULL;
}

Uint32 UIWinMenu::getMenuHeight() const {
	return mMenuHeight;
}

void UIWinMenu::setMenuHeight( const Uint32& menuHeight ) {
	mMenuHeight = menuHeight;

	if ( 0 != mMenuHeight ) {
		setSize( getParent()->getSize().getWidth(), mMenuHeight );
	} else {
		autoHeight();
	}

	refreshButtons();
}

void UIWinMenu::refreshButtons() {
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

	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		UISelectButton* pbut = it->first;
		pbut->setPosition( xpos, ycenter );
		xpos += pbut->getSize().getWidth() + pbut->getLayoutMargin().Left;
	}
}

bool UIWinMenu::applyProperty( const StyleSheetProperty& attribute ) {
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

Uint32 UIWinMenu::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::MouseUp:
		case NodeMessage::MouseOver: {
			if ( Msg->getSender()->isType( UI_TYPE_SELECTBUTTON ) ) {
				UISelectButton* tbut = Msg->getSender()->asType<UISelectButton>();
				UIPopUpMenu* tpop = getMenuFromButton( tbut );

				Vector2f pos( tbut->getPosition().x,
							  tbut->getPosition().y + tbut->getSize().getHeight() );
				tpop->setPosition( pos );

				if ( Msg->getMsg() == NodeMessage::MouseOver ) {
					if ( NULL != mCurrentMenu ) {
						mCurrentMenu = tpop;

						tbut->select();
						tpop->setParent( getWindowContainer() );
						tpop->show();
					}
				} else {
					if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
						if ( mCurrentMenu != tpop ) {
							mCurrentMenu = tpop;

							tbut->select();
							tpop->setParent( getWindowContainer() );
							tpop->show();
						} else {
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
			for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
				if ( it->first != Msg->getSender() ) {
					it->first->unselect();
				}
			}

			return 1;
		}
		case NodeMessage::FocusLoss: {
			if ( NULL != getEventDispatcher() ) {
				Node* FocusCtrl = getEventDispatcher()->getFocusNode();

				if ( !isParentOf( FocusCtrl ) && !isPopUpMenuChild( FocusCtrl ) ) {
					onWidgetFocusLoss();
				}

				return 1;
			}
		}
	}

	return 0;
}

void UIWinMenu::onParentChange() {
	setSize( getParent()->getSize().getWidth(), mMenuHeight );

	updateAnchorsDistances();

	UIWidget::onParentChange();
}

void UIWinMenu::onPaddingChange() {
	refreshButtons();
}

void UIWinMenu::unselectButtons() {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		it->first->unselect();
	}
}

UIPopUpMenu* UIWinMenu::getMenuFromButton( UISelectButton* Button ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		if ( it->first == Button ) {
			return it->second;
		}
	}

	return NULL;
}

bool UIWinMenu::isPopUpMenuChild( Node* Ctrl ) {
	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		if ( it->second == Ctrl || it->second->isParentOf( Ctrl ) ) {
			return true;
		}
	}

	return false;
}

void UIWinMenu::onMenuFocusLoss( const Event* ) {
	Node* FocusCtrl = getEventDispatcher()->getFocusNode();

	if ( !isParentOf( FocusCtrl ) && !isPopUpMenuChild( FocusCtrl ) ) {
		onWidgetFocusLoss();
	}
}

void UIWinMenu::onHideByClick( const Event* ) {
	onWidgetFocusLoss();
}

void UIWinMenu::onWidgetFocusLoss() {
	UIWidget::onWidgetFocusLoss();

	if ( NULL != mCurrentMenu ) {
		mCurrentMenu->hide();

		mCurrentMenu = NULL;
	}

	unselectButtons();
}

void UIWinMenu::autoHeight() {
	if ( 0 == mMenuHeight && NULL != getSkin() ) {
		mMenuHeight = getSkinSize().getHeight();

		setSize( getParent()->getSize().getWidth(), mMenuHeight );

		updateAnchorsDistances();
	}
}

void UIWinMenu::loadFromXmlNode( const pugi::xml_node& node ) {
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
