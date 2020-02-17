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

UIWinMenu::UIWinMenu() : UIWidget( "winmenu" ), mCurrentMenu( NULL ) {
	if ( !( mFlags & UI_ANCHOR_RIGHT ) )
		mFlags |= UI_ANCHOR_RIGHT;

	onParentChange();

	applyDefaultTheme();
}

UIWinMenu::~UIWinMenu() {
	destroyMenues();
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

	if ( NULL != mTheme )
		Button->setThemeSkin( mTheme, "winmenubutton" );

	Menu->setVisible( false );
	Menu->setEnabled( false );
	Menu->setParent( getWindowContainer() );
	Menu->addEventListener( Event::OnWidgetFocusLoss,
							cb::Make1( this, &UIWinMenu::onMenuFocusLoss ) );
	Menu->addEventListener( Event::OnHideByClick, cb::Make1( this, &UIWinMenu::onHideByClick ) );

	mButtons.push_back( std::make_pair( Button, Menu ) );

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

Uint32 UIWinMenu::getMarginBetweenButtons() const {
	return mStyleConfig.MarginBetweenButtons;
}

void UIWinMenu::setMarginBetweenButtons( const Uint32& marginBetweenButtons ) {
	mStyleConfig.MarginBetweenButtons = marginBetweenButtons;
	refreshButtons();
}

Uint32 UIWinMenu::getButtonMargin() const {
	return mStyleConfig.ButtonMargin;
}

void UIWinMenu::setButtonMargin( const Uint32& buttonMargin ) {
	mStyleConfig.ButtonMargin = buttonMargin;
	refreshButtons();
}

const UIWinMenu::StyleConfig& UIWinMenu::getStyleConfig() const {
	return mStyleConfig;
}

void UIWinMenu::setStyleConfig( const StyleConfig& styleConfig ) {
	mStyleConfig = styleConfig;
	refreshButtons();
}

Uint32 UIWinMenu::getMenuHeight() const {
	return mStyleConfig.MenuHeight;
}

void UIWinMenu::setMenuHeight( const Uint32& menuHeight ) {
	mStyleConfig.MenuHeight = menuHeight;

	if ( 0 != mStyleConfig.MenuHeight ) {
		setSize( getParent()->getSize().getWidth(), mStyleConfig.MenuHeight );
	} else {
		autoHeight();
	}

	refreshButtons();
}

Uint32 UIWinMenu::getFirstButtonMargin() const {
	return mStyleConfig.FirstButtonMargin;
}

void UIWinMenu::setFirstButtonMargin( const Uint32& buttonMargin ) {
	mStyleConfig.FirstButtonMargin = buttonMargin;
	refreshButtons();
}

std::string UIWinMenu::getPropertyString( const PropertyDefinition* propertyDef,
										  const Uint32& propertyIndex ) {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::MarginBetweenButtons:
			return String::format( "%ddp", getMarginBetweenButtons() );
		case PropertyId::ButtonMargin:
			return String::format( "%ddp", getButtonMargin() );
		case PropertyId::MenuHeight:
			return String::format( "%ddp", getMenuHeight() );
		case PropertyId::FirstButtonMarginLeft:
			return String::format( "%ddp", getFirstButtonMargin() );
		default:
			return UIWidget::getPropertyString( propertyDef, propertyIndex );
	}
}

bool UIWinMenu::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::MarginBetweenButtons:
			setMarginBetweenButtons( attribute.asDpDimensionUint() );
			break;
		case PropertyId::ButtonMargin:
			setButtonMargin( attribute.asDpDimensionUint() );
			break;
		case PropertyId::MenuHeight:
			setMenuHeight( attribute.asDpDimensionUint() );
			break;
		case PropertyId::FirstButtonMarginLeft:
			setFirstButtonMargin( attribute.asDpDimensionUint() );
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

void UIWinMenu::refreshButtons() {
	Uint32 xpos = mStyleConfig.FirstButtonMargin;
	Int32 h = 0, th = 0, ycenter = 0;

	UISkin* skin = getSkin();

	if ( NULL != skin ) {
		h = skin->getSize().getHeight();

		if ( mButtons.begin() != mButtons.end() ) {
			UISelectButton* tbut = mButtons.begin()->first;

			skin = tbut->getSkin();

			if ( NULL != skin ) {
				th = skin->getSize( UIState::StateFlagSelected ).getHeight();

				switch ( Font::getVerticalAlign( getFlags() ) ) {
					case UI_VALIGN_CENTER:
						ycenter = ( h - th ) / 2;
						break;
					case UI_VALIGN_BOTTOM:
						ycenter = ( h - th );
						break;
					case UI_VALIGN_TOP:
						ycenter = 0;
						break;
				}
			}
		}
	}

	for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
		UISelectButton* pbut = it->first;
		UITextView* tbox = pbut->getTextBox();

		pbut->setLayoutSizeRules( LayoutSizeRule::Fixed, LayoutSizeRule::Fixed );
		pbut->setPixelsSize( tbox->getTextWidth() +
								 PixelDensity::dpToPx( mStyleConfig.ButtonMargin ),
							 getPixelsSize().getHeight() );
		pbut->setPosition( xpos, ycenter );

		xpos += pbut->getSize().getWidth() + mStyleConfig.MarginBetweenButtons;
	}
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
						tpop->show();
					}
				} else {
					if ( Msg->getFlags() & EE_BUTTON_LMASK ) {
						if ( mCurrentMenu != tpop ) {
							mCurrentMenu = tpop;

							tbut->select();
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
				Node* FocusCtrl = getEventDispatcher()->getFocusControl();

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
	setSize( getParent()->getSize().getWidth(), mStyleConfig.MenuHeight );

	updateAnchorsDistances();

	UIWidget::onParentChange();
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
	Node* FocusCtrl = getEventDispatcher()->getFocusControl();

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

void UIWinMenu::destroyMenues() {
	if ( !SceneManager::instance()->isShootingDown() ) {
		for ( WinMenuList::iterator it = mButtons.begin(); it != mButtons.end(); ++it ) {
			it->second->close();
		}
	}
}

void UIWinMenu::autoHeight() {
	if ( 0 == mStyleConfig.MenuHeight && NULL != getSkin() ) {
		mStyleConfig.MenuHeight = getSkinSize().getHeight();

		setSize( getParent()->getSize().getWidth(), mStyleConfig.MenuHeight );

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
