#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/ui/css/propertydefinition.hpp>
#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uithememanager.hpp>
#include <pugixml/pugixml.hpp>

namespace EE { namespace UI {

UIMenu* UIMenu::New() {
	return eeNew( UIMenu, () );
}

UIMenu::UIMenu() :
	UIWidget( "menu" ),
	mMaxWidth( 0 ),
	mNextPosY( 0 ),
	mBiggestIcon( 0 ),
	mItemSelected( NULL ),
	mItemSelectedIndex( eeINDEX_NOT_FOUND ),
	mClickHide( false ),
	mLastTickMove( 0 ),
	mOwnerNode( NULL ) {
	mFlags |= UI_AUTO_SIZE;

	onSizeChange();

	applyDefaultTheme();
}

UIMenu::~UIMenu() {}

Uint32 UIMenu::getType() const {
	return UI_TYPE_MENU;
}

bool UIMenu::isType( const Uint32& type ) const {
	return UIMenu::getType() == type ? true : UIWidget::isType( type );
}

void UIMenu::setTheme( UITheme* Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "menu" );
	onThemeLoaded();
}

void UIMenu::onThemeLoaded() {
	UIWidget::onThemeLoaded();

	autoPadding();

	onSizeChange();
}

UIMenuItem* UIMenu::createMenuItem( const String& Text, Drawable* Icon ) {
	UIMenuItem* tCtrl = UIMenuItem::New();
	tCtrl->setHorizontalAlign( UI_HALIGN_LEFT );
	tCtrl->setParent( this );
	tCtrl->setIconMinimumSize(
		Sizei( mStyleConfig.MinSpaceForIcons, mStyleConfig.MinSpaceForIcons ) );
	tCtrl->setIcon( Icon );
	tCtrl->setText( Text );

	return tCtrl;
}

Uint32 UIMenu::add( const String& Text, Drawable* Icon ) {
	return add( createMenuItem( Text, Icon ) );
}

UIMenuCheckBox* UIMenu::createMenuCheckBox( const String& Text, const bool& Active ) {
	UIMenuCheckBox* tCtrl = UIMenuCheckBox::New();
	tCtrl->setHorizontalAlign( UI_HALIGN_LEFT );
	tCtrl->setParent( this );
	tCtrl->setIconMinimumSize(
		Sizei( mStyleConfig.MinSpaceForIcons, mStyleConfig.MinSpaceForIcons ) );
	tCtrl->setText( Text );

	if ( Active )
		tCtrl->setActive( Active );

	return tCtrl;
}

Uint32 UIMenu::addCheckBox( const String& Text, const bool& Active ) {
	return add( createMenuCheckBox( Text, Active ) );
}

UIMenuSubMenu* UIMenu::createSubMenu( const String& Text, Drawable* Icon, UIMenu* SubMenu ) {
	UIMenuSubMenu* tCtrl = UIMenuSubMenu::New();
	tCtrl->setHorizontalAlign( UI_HALIGN_LEFT );
	tCtrl->setParent( this );
	tCtrl->setIconMinimumSize(
		Sizei( mStyleConfig.MinSpaceForIcons, mStyleConfig.MinSpaceForIcons ) );
	tCtrl->setIcon( Icon );
	tCtrl->setText( Text );
	tCtrl->setSubMenu( SubMenu );

	return tCtrl;
}

Uint32 UIMenu::addSubMenu( const String& Text, Drawable* Icon, UIMenu* SubMenu ) {
	return add( createSubMenu( Text, Icon, SubMenu ) );
}

bool UIMenu::checkControlSize( UINode* Control, const bool& Resize ) {
	if ( Control->isType( UI_TYPE_MENUITEM ) ) {
		UIMenuItem* tItem = Control->asType<UIMenuItem>();

		if ( NULL != tItem->getIcon() &&
			 tItem->getIconHorizontalMargin() + tItem->getIcon()->getSize().getWidth() >
				 (Int32)mBiggestIcon ) {
			mBiggestIcon =
				tItem->getIconHorizontalMargin() + tItem->getIcon()->getSize().getWidth();
		}

		if ( mFlags & UI_AUTO_SIZE ) {
			if ( Control->isType( UI_TYPE_MENUSUBMENU ) ) {
				Int32 textWidth = tItem->getTextBox()->getTextWidth();

				UIMenuSubMenu* tMenu = tItem->asType<UIMenuSubMenu>();

				if ( textWidth + PixelDensity::dpToPxI( mBiggestIcon ) +
						 tMenu->getArrow()->getPixelsSize().getWidth() +
						 PixelDensity::dpToPxI( mStyleConfig.MinRightMargin ) >
					 (Int32)mMaxWidth ) {
					mMaxWidth =
						textWidth + mRealPadding.Left + mRealPadding.Right +
						PixelDensity::dpToPxI( mBiggestIcon + mStyleConfig.MinRightMargin ) +
						tMenu->getArrow()->getPixelsSize().getWidth();

					if ( Resize ) {
						resizeControls();

						return true;
					}
				}
			} else {
				if ( Control->getPixelsSize().getWidth() > (Int32)mMaxWidth ) {
					mMaxWidth = Control->getPixelsSize().getWidth();

					if ( Resize ) {
						resizeControls();

						return true;
					}
				}
			}
		}
	}

	return false;
}

Uint32 UIMenu::add( UINode* Control ) {
	if ( this != Control->getParent() )
		Control->setParent( this );

	checkControlSize( Control );

	setControlSize( Control, getCount() );

	Control->setPixelsPosition( mRealPadding.Left, mRealPadding.Top + mNextPosY );

	mNextPosY += Control->getPixelsSize().getHeight();

	mItems.push_back( Control );

	resizeMe();

	return mItems.size() - 1;
}

void UIMenu::setControlSize( UINode* Control, const Uint32& ) {
	Control->setPixelsSize( mSize.getWidth(), Control->getPixelsSize().getHeight() );
}

Uint32 UIMenu::addSeparator() {
	UIMenuSeparator* Control = UIMenuSeparator::New();
	Control->setParent( this );
	Control->setPixelsPosition( mRealPadding.Left, mRealPadding.Top + mNextPosY );
	Control->setPixelsSize( mSize.getWidth() - mRealPadding.Left - mRealPadding.Right,
							PixelDensity::dpToPxI( Control->getSkinSize().getHeight() ) );

	mNextPosY += Control->getPixelsSize().getHeight();

	mItems.push_back( Control );

	resizeMe();

	return mItems.size() - 1;
}

UINode* UIMenu::getItem( const Uint32& Index ) {
	eeASSERT( Index < mItems.size() );
	return mItems[Index];
}

UINode* UIMenu::getItem( const String& Text ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i]->isType( UI_TYPE_MENUITEM ) ) {
			UIMenuItem* tMenuItem = mItems[i]->asType<UIMenuItem>();

			if ( tMenuItem->getText() == Text )
				return tMenuItem;
		}
	}

	return NULL;
}

Uint32 UIMenu::getItemIndex( UINode* Item ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i] == Item )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Uint32 UIMenu::getCount() const {
	return mItems.size();
}

void UIMenu::remove( const Uint32& Index ) {
	eeASSERT( Index < mItems.size() );

	mItems[Index]->close();

	mItems.erase( mItems.begin() + Index );

	rePosControls();
	resizeControls();
}

void UIMenu::remove( UINode* Ctrl ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i] == Ctrl ) {
			remove( i );
			break;
		}
	}
}

void UIMenu::removeAll() {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		mItems[i]->close();
	}

	mItems.clear();
	mNextPosY = 0;
	mMaxWidth = 0;

	resizeMe();
}

void UIMenu::insert( const String& Text, Drawable* Icon, const Uint32& Index ) {
	insert( createMenuItem( Text, Icon ), Index );
}

void UIMenu::insert( UINode* Control, const Uint32& Index ) {
	mItems.insert( mItems.begin() + Index, Control );

	childAddAt( Control, Index );

	rePosControls();
	resizeControls();
}

bool UIMenu::isSubMenu( Node* Ctrl ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( NULL != mItems[i] && mItems[i]->isType( UI_TYPE_MENUSUBMENU ) ) {
			UIMenuSubMenu* tMenu = mItems[i]->asType<UIMenuSubMenu>();

			if ( tMenu->getSubMenu() == Ctrl )
				return true;
		}
	}

	return false;
}

Uint32 UIMenu::onMessage( const NodeMessage* Msg ) {
	switch ( Msg->getMsg() ) {
		case NodeMessage::MouseUp: {
			if ( Msg->getSender()->getParent() == this && ( Msg->getFlags() & EE_BUTTONS_LRM ) ) {
				Event ItemEvent( Msg->getSender(), Event::OnItemClicked );
				sendEvent( &ItemEvent );
			}

			return 1;
		}
		case NodeMessage::FocusLoss: {
			if ( NULL != getEventDispatcher() ) {
				Node* focusCtrl = getEventDispatcher()->getFocusControl();

				if ( this != focusCtrl && !isParentOf( focusCtrl ) && !isSubMenu( focusCtrl ) &&
					 mOwnerNode != focusCtrl ) {
					mClickHide = true;

					onWidgetFocusLoss();
				}

				return 1;
			}
		}
	}

	return 0;
}

void UIMenu::onSizeChange() {
	if ( 0 != mStyleConfig.MinWidth && getSize().getWidth() < (Int32)mStyleConfig.MinWidth ) {
		setSize( mStyleConfig.MinWidth,
				 PixelDensity::pxToDpI( mNextPosY ) + mPadding.Top + mPadding.Bottom );
	}

	UIWidget::onSizeChange();
}

void UIMenu::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPadding = makePadding();
	}
}

void UIMenu::resizeControls() {
	resizeMe();

	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		setControlSize( mItems[i], i );
	}
}

void UIMenu::rePosControls() {
	Uint32 i;
	mNextPosY = 0;
	mBiggestIcon = mStyleConfig.MinSpaceForIcons;

	if ( mFlags & UI_AUTO_SIZE ) {
		mMaxWidth = 0;

		for ( i = 0; i < mItems.size(); i++ ) {
			checkControlSize( mItems[i], false );
		}
	}

	for ( i = 0; i < mItems.size(); i++ ) {
		UINode* ctrl = mItems[i];

		ctrl->setPixelsPosition( mRealPadding.Left, mRealPadding.Top + mNextPosY );

		mNextPosY += ctrl->getPixelsSize().getHeight();
	}

	resizeMe();
	invalidateDraw();
}

void UIMenu::resizeMe() {
	if ( mFlags & UI_AUTO_SIZE ) {
		setPixelsSize( mMaxWidth, mNextPosY + mRealPadding.Top + mRealPadding.Bottom );
	} else {
		setPixelsSize( mSize.getWidth(), mNextPosY + mRealPadding.Top + mRealPadding.Bottom );
	}
}

bool UIMenu::show() {
	setEnabled( true );
	setVisible( true );
	return true;
}

bool UIMenu::hide() {
	setEnabled( false );
	setVisible( false );

	if ( NULL != mItemSelected )
		mItemSelected->popState( UIState::StateSelected );

	mItemSelected = NULL;
	mItemSelectedIndex = eeINDEX_NOT_FOUND;

	return true;
}

void UIMenu::setItemSelected( UINode* Item ) {
	if ( NULL != mItemSelected ) {
		if ( mItemSelected->isType( UI_TYPE_MENUSUBMENU ) ) {
			UIMenuSubMenu* tMenu = mItemSelected->asType<UIMenuSubMenu>();

			if ( NULL != tMenu->getSubMenu() )
				tMenu->getSubMenu()->hide();
		}

		mItemSelected->popState( UIState::StateSelected );
	}

	if ( NULL != Item )
		Item->pushState( UIState::StateSelected );

	if ( mItemSelected != Item ) {
		mItemSelected = Item;
		mItemSelectedIndex = getItemIndex( mItemSelected );
	}
}

void UIMenu::trySelect( UINode* Ctrl, bool Up ) {
	if ( mItems.size() ) {
		if ( !Ctrl->isType( UI_TYPE_MENU_SEPARATOR ) ) {
			setItemSelected( Ctrl );
		} else {
			Uint32 Index = getItemIndex( Ctrl );

			if ( Index != eeINDEX_NOT_FOUND ) {
				if ( Up ) {
					if ( Index > 0 ) {
						for ( Int32 i = (Int32)Index - 1; i >= 0; i-- ) {
							if ( !mItems[i]->isType( UI_TYPE_MENU_SEPARATOR ) ) {
								setItemSelected( mItems[i] );
								return;
							}
						}
					}

					setItemSelected( mItems[mItems.size()] );
				} else {
					for ( Uint32 i = Index + 1; i < mItems.size(); i++ ) {
						if ( !mItems[i]->isType( UI_TYPE_MENU_SEPARATOR ) ) {
							setItemSelected( mItems[i] );
							return;
						}
					}

					setItemSelected( mItems[0] );
				}
			}
		}
	}
}

void UIMenu::nextSel() {
	if ( mItems.size() ) {
		if ( mItemSelectedIndex != eeINDEX_NOT_FOUND ) {
			if ( mItemSelectedIndex + 1 < mItems.size() ) {
				trySelect( mItems[mItemSelectedIndex + 1], false );
			} else {
				trySelect( mItems[0], false );
			}
		} else {
			trySelect( mItems[0], false );
		}
	}
}

void UIMenu::prevSel() {
	if ( mItems.size() ) {
		if ( mItemSelectedIndex != eeINDEX_NOT_FOUND ) {
			if ( mItemSelectedIndex >= 1 ) {
				trySelect( mItems[mItemSelectedIndex - 1], true );
			} else {
				trySelect( mItems[mItems.size() - 1], true );
			}
		} else {
			trySelect( mItems[0], true );
		}
	}
}

Uint32 UIMenu::onKeyDown( const KeyEvent& Event ) {
	if ( Sys::getTicks() - mLastTickMove > 50 ) {
		switch ( Event.getKeyCode() ) {
			case KEY_DOWN:
				mLastTickMove = Sys::getTicks();
				nextSel();

				break;
			case KEY_UP:
				mLastTickMove = Sys::getTicks();
				prevSel();

				break;
			case KEY_RIGHT:
				if ( NULL != mItemSelected && ( mItemSelected->isType( UI_TYPE_MENUSUBMENU ) ) ) {
					UIMenuSubMenu* tMenu = mItemSelected->asType<UIMenuSubMenu>();

					tMenu->showSubMenu();
				}

				break;
			case KEY_LEFT:
				hide();

				break;
			case KEY_ESCAPE:
				hide();

				break;
			case KEY_RETURN:
				if ( NULL != mItemSelected && NULL != getEventDispatcher() ) {
					mItemSelected->sendMouseEvent(
						Event::MouseClick, getEventDispatcher()->getMousePos(), EE_BUTTONS_ALL );

					NodeMessage Msg( mItemSelected, NodeMessage::MouseUp, EE_BUTTONS_ALL );
					mItemSelected->messagePost( &Msg );
				}

				break;
		}
	}

	return UIWidget::onKeyDown( Event );
}

Uint32 UIMenu::getMinRightMargin() const {
	return mStyleConfig.MinRightMargin;
}

void UIMenu::setMinRightMargin( const Uint32& minRightMargin ) {
	mStyleConfig.MinRightMargin = minRightMargin;
	rePosControls();
}

const UIMenu::StyleConfig& UIMenu::getStyleConfig() const {
	return mStyleConfig;
}

static Drawable* getIconDrawable( const std::string& name, UITheme* theme ) {
	Drawable* iconDrawable = NULL;

	if ( NULL != theme )
		iconDrawable = theme->getIconByName( name );

	if ( NULL == iconDrawable )
		iconDrawable = DrawableSearcher::searchByName( name );

	return iconDrawable;
}

void UIMenu::loadFromXmlNode( const pugi::xml_node& node ) {
	beginAttributesTransaction();

	UIWidget::loadFromXmlNode( node );

	for ( pugi::xml_node item = node.first_child(); item; item = item.next_sibling() ) {
		std::string name( item.name() );
		String::toLowerInPlace( name );

		if ( name == "menuitem" || name == "item" ) {
			std::string text( item.attribute( "text" ).as_string() );
			std::string icon( item.attribute( "icon" ).as_string() );

			if ( NULL != mSceneNode && mSceneNode->isUISceneNode() )
				add( static_cast<UISceneNode*>( mSceneNode )->getTranslatorString( text ),
					 getIconDrawable( icon,
									  getUISceneNode()->getUIThemeManager()->getDefaultTheme() ) );
		} else if ( name == "menuseparator" || name == "separator" ) {
			addSeparator();
		} else if ( name == "menucheckbox" || name == "checkbox" ) {
			std::string text( item.attribute( "text" ).as_string() );
			bool active( item.attribute( "active" ).as_bool() );

			if ( NULL != mSceneNode && mSceneNode->isUISceneNode() )
				addCheckBox( static_cast<UISceneNode*>( mSceneNode )->getTranslatorString( text ),
							 active );
		} else if ( name == "menusubmenu" || name == "submenu" ) {
			std::string text( item.attribute( "text" ).as_string() );
			std::string icon( item.attribute( "icon" ).as_string() );

			UIPopUpMenu* subMenu = UIPopUpMenu::New();

			if ( NULL != getDrawInvalidator() )
				subMenu->setParent( getDrawInvalidator() );

			subMenu->loadFromXmlNode( item );

			if ( NULL != mSceneNode && mSceneNode->isUISceneNode() )
				addSubMenu( static_cast<UISceneNode*>( mSceneNode )->getTranslatorString( text ),
							getIconDrawable(
								icon, getUISceneNode()->getUIThemeManager()->getDefaultTheme() ),
							subMenu );
		}
	}

	endAttributesTransaction();
}

std::string UIMenu::getPropertyString( const PropertyDefinition* propertyDef ) {
	if ( NULL == propertyDef )
		return "";

	switch ( propertyDef->getPropertyId() ) {
		case PropertyId::MinWidth:
			return String::format( "%udp", mStyleConfig.MinWidth );
		case PropertyId::MinMarginRight:
			return String::format( "%udp", getMinRightMargin() );
		case PropertyId::MinIconSpace:
			return String::format( "%udp", mStyleConfig.MinSpaceForIcons );
		default:
			return UIWidget::getPropertyString( propertyDef );
	}
}

bool UIMenu::applyProperty( const StyleSheetProperty& attribute ) {
	if ( !checkPropertyDefinition( attribute ) )
		return false;

	switch ( attribute.getPropertyDefinition()->getPropertyId() ) {
		case PropertyId::MinWidth:
			mStyleConfig.MinWidth = attribute.asDpDimensionI();
			onSizeChange();
			break;
		case PropertyId::MinMarginRight:
			setMinRightMargin( attribute.asDpDimensionUint() );
			break;
		case PropertyId::MinIconSpace:
			setMinSpaceForIcons( attribute.asDpDimensionUint() );
			break;
		default:
			return UIWidget::applyProperty( attribute );
	}

	return true;
}

UINode* UIMenu::getOwnerNode() const {
	return mOwnerNode;
}

void UIMenu::setOwnerNode( UINode* ownerNode ) {
	mOwnerNode = ownerNode;
}

void UIMenu::setMinSpaceForIcons( const Uint32& minSpaceForIcons ) {
	// if ( minSpaceForIcons != mStyleConfig.MinSpaceForIcons )
	{
		mStyleConfig.MinSpaceForIcons = minSpaceForIcons;
		mBiggestIcon = eemax( mBiggestIcon, mStyleConfig.MinSpaceForIcons );

		for ( Uint32 i = 0; i < mItems.size(); i++ ) {
			if ( mItems[i]->isType( UI_TYPE_MENUITEM ) ) {
				UIMenuItem* menuItem = static_cast<UIMenuItem*>( mItems[i] );

				menuItem->setIconMinimumSize( Sizei( minSpaceForIcons, minSpaceForIcons ) );
			}
		}

		rePosControls();
		resizeControls();
	}
}

void UIMenu::fixMenuPos( Vector2f& Pos, UIMenu* Menu, UIMenu* Parent, UIMenuSubMenu* SubMenu ) {
	SceneNode* sceneNode = Menu->getSceneNode();

	if ( NULL == sceneNode )
		return;

	Rectf qScreen( 0.f, 0.f, sceneNode->getPixelsSize().getWidth(),
				   sceneNode->getPixelsSize().getHeight() );
	Rectf qPos( Pos.x, Pos.y, Pos.x + Menu->getPixelsSize().getWidth(),
				Pos.y + Menu->getPixelsSize().getHeight() );

	if ( NULL != Parent && NULL != SubMenu ) {
		Vector2f sPos = SubMenu->getPixelsPosition();
		SubMenu->nodeToWorldTranslation( sPos );

		Vector2f pPos = Parent->getPixelsPosition();
		Parent->nodeToWorldTranslation( pPos );

		Rectf qParent( pPos.x, pPos.y, pPos.x + Parent->getPixelsSize().getWidth(),
					   pPos.y + Parent->getPixelsSize().getHeight() );

		Pos.x = qParent.Right;
		Pos.y = sPos.y;
		qPos.Left = Pos.x;
		qPos.Right = qPos.Left + Menu->getPixelsSize().getWidth();
		qPos.Top = Pos.y;
		qPos.Bottom = qPos.Top + Menu->getPixelsSize().getHeight();

		if ( !qScreen.contains( qPos ) ) {
			Pos.y =
				sPos.y + SubMenu->getPixelsSize().getHeight() - Menu->getPixelsSize().getHeight();
			qPos.Top = Pos.y;
			qPos.Bottom = qPos.Top + Menu->getPixelsSize().getHeight();

			if ( !qScreen.contains( qPos ) ) {
				Pos.x = qParent.Left - Menu->getPixelsSize().getWidth();
				Pos.y = sPos.y;
				qPos.Left = Pos.x;
				qPos.Right = qPos.Left + Menu->getPixelsSize().getWidth();
				qPos.Top = Pos.y;
				qPos.Bottom = qPos.Top + Menu->getPixelsSize().getHeight();

				if ( !qScreen.contains( qPos ) ) {
					Pos.y = sPos.y + SubMenu->getPixelsSize().getHeight() -
							Menu->getPixelsSize().getHeight();
					qPos.Top = Pos.y;
					qPos.Bottom = qPos.Top + Menu->getPixelsSize().getHeight();
				}
			}
		}
	} else {
		if ( !qScreen.contains( qPos ) ) {
			Pos.y -= Menu->getPixelsSize().getHeight();
			qPos.Top -= Menu->getPixelsSize().getHeight();
			qPos.Bottom -= Menu->getPixelsSize().getHeight();

			if ( !qScreen.contains( qPos ) ) {
				Pos.x -= Menu->getPixelsSize().getWidth();
				qPos.Left -= Menu->getPixelsSize().getWidth();
				qPos.Right -= Menu->getPixelsSize().getWidth();

				if ( !qScreen.contains( qPos ) ) {
					Pos.y += Menu->getPixelsSize().getHeight();
					qPos.Top += Menu->getPixelsSize().getHeight();
					qPos.Bottom += Menu->getPixelsSize().getHeight();
				}
			}
		}
	}
}

}} // namespace EE::UI
