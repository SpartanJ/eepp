#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/font.hpp>
#include <eepp/graphics/drawablesearcher.hpp>
#include <eepp/helper/pugixml/pugixml.hpp>
#include <eepp/ui/uipopupmenu.hpp>
#include <eepp/ui/uimanager.hpp>

namespace EE { namespace UI {

UIMenu *UIMenu::New() {
	return eeNew( UIMenu, () );
}

UIMenu::UIMenu() :
	UIWidget(),
	mMaxWidth( 0 ),
	mNextPosY( 0 ),
	mBiggestIcon( 0 ),
	mItemSelected( NULL ),
	mItemSelectedIndex( eeINDEX_NOT_FOUND ),
	mClickHide( false ),
	mLastTickMove( 0 )
{
	mFlags |= UI_AUTO_SIZE;

	mStyleConfig = UIThemeManager::instance()->getDefaultFontStyleConfig();

	UITheme * Theme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL != Theme ) {
		mStyleConfig = Theme->getMenuStyleConfig();
		mBiggestIcon = mStyleConfig.MinSpaceForIcons;
	}

	onSizeChange();

	applyDefaultTheme();
}

UIMenu::~UIMenu() {
}

Uint32 UIMenu::getType() const {
	return UI_TYPE_MENU;
}

bool UIMenu::isType( const Uint32& type ) const {
	return UIMenu::getType() == type ? true : UIWidget::isType( type );
}

void UIMenu::setTheme( UITheme * Theme ) {
	UIWidget::setTheme( Theme );

	setThemeSkin( Theme, "menu" );
	onThemeLoaded();
}

void UIMenu::onThemeLoaded() {
	autoPadding();

	onSizeChange();
}

UIMenuItem * UIMenu::createMenuItem( const String& Text, Drawable * Icon ) {
	UIPushButtonStyleConfig styleConfig( mStyleConfig );
	styleConfig.IconMinSize = Sizei( mStyleConfig.MinSpaceForIcons, mStyleConfig.MinSpaceForIcons );

	UIMenuItem * tCtrl 	= UIMenuItem::New();
	tCtrl->setHorizontalAlign( UI_HALIGN_LEFT );
	tCtrl->setParent( this );
	tCtrl->setStyleConfig( styleConfig );
	tCtrl->setIcon( Icon );
	tCtrl->setText( Text );;

	return tCtrl;
}

Uint32 UIMenu::add( const String& Text, Drawable * Icon ) {
	return add( createMenuItem( Text, Icon ) );
}

UIMenuCheckBox * UIMenu::createMenuCheckBox( const String& Text, const bool &Active ) {
	UIPushButtonStyleConfig styleConfig( mStyleConfig );
	styleConfig.IconMinSize = Sizei( mStyleConfig.MinSpaceForIcons, mStyleConfig.MinSpaceForIcons );

	UIMenuCheckBox * tCtrl 	= UIMenuCheckBox::New();
	tCtrl->setHorizontalAlign( UI_HALIGN_LEFT );
	tCtrl->setParent( this );
	tCtrl->setStyleConfig( styleConfig );
	tCtrl->setText( Text );

	if ( Active )
		tCtrl->setActive( Active );

	return tCtrl;
}

Uint32 UIMenu::addCheckBox( const String& Text, const bool& Active ) {
	return add( createMenuCheckBox( Text, Active ) );
}

UIMenuSubMenu * UIMenu::createSubMenu( const String& Text, Drawable * Icon, UIMenu * SubMenu ) {
	UIPushButtonStyleConfig styleConfig( mStyleConfig );
	styleConfig.IconMinSize = Sizei( mStyleConfig.MinSpaceForIcons, mStyleConfig.MinSpaceForIcons );

	UIMenuSubMenu * tCtrl 	= UIMenuSubMenu::New();
	tCtrl->setHorizontalAlign( UI_HALIGN_LEFT );
	tCtrl->setParent( this );
	tCtrl->setStyleConfig( styleConfig );
	tCtrl->setIcon( Icon );
	tCtrl->setText( Text );
	tCtrl->setSubMenu( SubMenu );

	return tCtrl;
}

Uint32 UIMenu::addSubMenu( const String& Text, Drawable * Icon, UIMenu * SubMenu ) {
	return add( createSubMenu( Text, Icon, SubMenu ) );
}

bool UIMenu::checkControlSize( UIControl * Control, const bool& Resize ) {
	if ( Control->isType( UI_TYPE_MENUITEM ) ) {
		UIMenuItem * tItem = reinterpret_cast<UIMenuItem*> ( Control );

		if ( NULL != tItem->getIcon() && tItem->getIconHorizontalMargin() + tItem->getIcon()->getSize().getWidth() > (Int32)mBiggestIcon ) {
			mBiggestIcon = tItem->getIconHorizontalMargin() + tItem->getIcon()->getSize().getWidth();
		}

		if ( mFlags & UI_AUTO_SIZE ) {
			if ( Control->isType( UI_TYPE_MENUSUBMENU ) ) {
				Int32 textWidth = PixelDensity::pxToDpI( tItem->getTextBox()->getTextWidth() );

				UIMenuSubMenu * tMenu = reinterpret_cast<UIMenuSubMenu*> ( tItem );

				if ( textWidth + (Int32)mBiggestIcon + tMenu->getArrow()->getSize().getWidth() + (Int32)mStyleConfig.MinRightMargin > (Int32)mMaxWidth ) {
					mMaxWidth = textWidth + mBiggestIcon + mStyleConfig.Padding.Left + mStyleConfig.Padding.Right + tMenu->getArrow()->getSize().getWidth() + mStyleConfig.MinRightMargin;

					if ( Resize ) {
						resizeControls();

						return true;
					}
				}
			} else {
				if ( Control->getSize().getWidth() > (Int32)mMaxWidth ) {
					mMaxWidth = Control->getSize().getWidth();

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

Uint32 UIMenu::add( UIControl * Control ) {
	if ( this != Control->getParent() )
		Control->setParent( this );

	checkControlSize( Control );

	setControlSize( Control, getCount() );

	Control->setPosition( mStyleConfig.Padding.Left, mStyleConfig.Padding.Top + mNextPosY );

	mNextPosY += Control->getSize().getHeight();

	mItems.push_back( Control );

	resizeMe();

	return mItems.size() - 1;
}

void UIMenu::setControlSize( UIControl * Control, const Uint32& Pos ) {
	Control->setSize( mSize.getWidth(), Control->getSize().getHeight() );
}

Uint32 UIMenu::addSeparator() {
	UIMenuSeparator * Control = UIMenuSeparator::New();
	Control->setParent( this );
	Control->setPosition( mStyleConfig.Padding.Left, mStyleConfig.Padding.Top + mNextPosY );
	Control->setSize( mSize.getWidth() - mStyleConfig.Padding.Left - mStyleConfig.Padding.Right, 3 );

	mNextPosY += Control->getSize().getHeight();

	mItems.push_back( Control );

	resizeMe();

	return mItems.size() - 1;
}

UIControl * UIMenu::getItem( const Uint32& Index ) {
	eeASSERT( Index < mItems.size() );
	return mItems[ Index ];
}

UIControl * UIMenu::getItem( const String& Text ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i]->isType( UI_TYPE_MENUITEM ) ) {
			UIMenuItem * tMenuItem = reinterpret_cast<UIMenuItem*>( mItems[i] );
			
			if ( tMenuItem->getText() == Text )
				return tMenuItem;
		}
	}
	
	return NULL;
}

Uint32 UIMenu::getItemIndex( UIControl * Item ) {
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

	eeSAFE_DELETE( mItems[ Index ] );

	mItems.erase( mItems.begin() + Index );

	rePosControls();
	resizeControls();
}

void UIMenu::remove( UIControl * Ctrl ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i] == Ctrl ) {
			remove( i );
			break;
		}
	}
}

void UIMenu::removeAll() {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		eeSAFE_DELETE( mItems[ i ] );
	}

	mItems.clear();
	mNextPosY = 0;
	mMaxWidth = 0;

	resizeMe();
}

void UIMenu::insert( const String& Text, Drawable * Icon, const Uint32& Index ) {
	insert( createMenuItem( Text, Icon ), Index );
}

void UIMenu::insert( UIControl * Control, const Uint32& Index ) {
	mItems.insert( mItems.begin() + Index, Control );

	childAddAt( Control, Index );

	rePosControls();
	resizeControls();
}

bool UIMenu::isSubMenu( UIControl * Ctrl ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i]->isType( UI_TYPE_MENUSUBMENU ) ) {
			UIMenuSubMenu * tMenu = reinterpret_cast<UIMenuSubMenu*> ( mItems[i] );

			if ( tMenu->getSubMenu() == Ctrl )
				return true;
		}
	}

	return false;
}

Uint32 UIMenu::onMessage( const UIMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case UIMessage::MsgMouseUp:
		{
			if ( Msg->getSender()->getParent() == this && ( Msg->getFlags() & EE_BUTTONS_LRM ) ) {
				UIEvent ItemEvent( Msg->getSender(), UIEvent::EventOnItemClicked );
				sendEvent( &ItemEvent );
			}

			return 1;
		}
		case UIMessage::MsgFocusLoss:
		{
			UIControl * FocusCtrl = UIManager::instance()->getFocusControl();

			if ( this != FocusCtrl && !isParentOf( FocusCtrl ) && !isSubMenu( FocusCtrl ) ) {
				onWidgetFocusLoss();
			}

			return 1;
		}
	}

	return 0;
}

void UIMenu::onSizeChange() {
	if ( 0 != mStyleConfig.MinWidth && mSize.getWidth() < (Int32)mStyleConfig.MinWidth ) {
		setSize( mStyleConfig.MinWidth, mNextPosY + mStyleConfig.Padding.Top + mStyleConfig.Padding.Bottom );
	}
}

void UIMenu::autoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mStyleConfig.Padding = makePadding();
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
		mItems[i]->setPosition( mStyleConfig.Padding.Left, mStyleConfig.Padding.Top + mNextPosY );

		mNextPosY += mItems[i]->getSize().getHeight();
	}

	resizeMe();
}

void UIMenu::resizeMe() {
	if ( mFlags & UI_AUTO_SIZE ) {
		setSize( mMaxWidth, mNextPosY + mStyleConfig.Padding.Top + mStyleConfig.Padding.Bottom );
	} else {
		setSize( mSize.getWidth(), mNextPosY + mStyleConfig.Padding.Top + mStyleConfig.Padding.Bottom );
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
		mItemSelected->setSkinState( UISkinState::StateNormal );

	mItemSelected		= NULL;
	mItemSelectedIndex	= eeINDEX_NOT_FOUND;

	return true;
}

void UIMenu::setItemSelected( UIControl * Item ) {
	if ( NULL != mItemSelected ) {
		if ( mItemSelected->isType( UI_TYPE_MENUSUBMENU ) ) {
			UIMenuSubMenu * tMenu = reinterpret_cast<UIMenuSubMenu*> ( mItemSelected );

			if ( NULL != tMenu->getSubMenu() )
				tMenu->getSubMenu()->hide();
		}

		mItemSelected->setSkinState( UISkinState::StateNormal );
	}

	if ( NULL != Item )
		Item->setSkinState( UISkinState::StateSelected );

	if ( mItemSelected != Item ) {
		mItemSelected		= Item;
		mItemSelectedIndex	= getItemIndex( mItemSelected );
	}
}

void UIMenu::trySelect( UIControl * Ctrl, bool Up ) {
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

					setItemSelected( mItems[ mItems.size() ] );
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
				trySelect( mItems[ mItemSelectedIndex + 1 ], false );
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
		if (  mItemSelectedIndex != eeINDEX_NOT_FOUND  ) {
			if ( mItemSelectedIndex >= 1 ) {
				trySelect( mItems[ mItemSelectedIndex - 1 ], true );
			} else {
				trySelect( mItems[ mItems.size() - 1 ], true );
			}
		} else {
			trySelect( mItems[0], true );
		}
	}
}

Uint32 UIMenu::onKeyDown( const UIEventKey& Event ) {
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
					UIMenuSubMenu * tMenu = reinterpret_cast<UIMenuSubMenu*> ( mItemSelected );

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
				if ( NULL != mItemSelected ) {
					mItemSelected->sendMouseEvent(UIEvent::EventMouseClick, UIManager::instance()->getMousePos(), EE_BUTTONS_ALL );

					UIMessage Msg( mItemSelected, UIMessage::MsgMouseUp, EE_BUTTONS_ALL );
					mItemSelected->messagePost( &Msg );
				}

				break;
		}
	}

	return UIWidget::onKeyDown( Event );
}

const Rect& UIMenu::getPadding() const {
	return mStyleConfig.Padding;
}

Uint32 UIMenu::getMinRightMargin() const {
	return mStyleConfig.MinRightMargin;
}

void UIMenu::setMinRightMargin(const Uint32 & minRightMargin) {
	mStyleConfig.MinRightMargin = minRightMargin;
	rePosControls();
}

UITooltipStyleConfig UIMenu::getFontStyleConfig() const {
	return mStyleConfig;
}

void UIMenu::setFontStyleConfig(const UITooltipStyleConfig & fontStyleConfig) {
	mStyleConfig = fontStyleConfig;
}

static Drawable * getIconDrawable( const std::string& name ) {
	Drawable * iconDrawable = NULL;
	UITheme * theme = UIThemeManager::instance()->getDefaultTheme();

	if ( NULL != theme )
		iconDrawable = theme->getIconByName( name );

	if ( NULL == iconDrawable )
		iconDrawable = DrawableSearcher::searchByName( name );

	return iconDrawable;
}

void UIMenu::loadFromXmlNode( const pugi::xml_node& node ) {
	UIWidget::loadFromXmlNode( node );

	for ( pugi::xml_node item = node.first_child(); item; item = item.next_sibling() ) {
		std::string name( item.name() );
		String::toLowerInPlace( name );

		if ( name == "menuitem" || name == "item" ) {
			std::string text( item.attribute("text").as_string() );
			std::string icon( item.attribute("icon").as_string() );
			add( UIManager::instance()->getTranslatorString( text ), getIconDrawable( icon ) );
		} else if ( name == "menuseparator" || name == "separator" ) {
			addSeparator();
		} else if ( name == "menucheckbox" || name == "checkbox" ) {
			std::string text( item.attribute("text").as_string() );
			bool active( item.attribute("active").as_bool() );

			addCheckBox( UIManager::instance()->getTranslatorString( text ), active );
		} else if ( name == "menusubmenu" || name == "submenu" ) {
			std::string text( item.attribute("text").as_string() );
			std::string icon( item.attribute("icon").as_string() );

			UIPopUpMenu * subMenu = UIPopUpMenu::New();

			subMenu->loadFromXmlNode( item );

			addSubMenu( UIManager::instance()->getTranslatorString( text ), getIconDrawable( icon ), subMenu );
		}
	}
}

void UIMenu::fixMenuPos( Vector2i& Pos, UIMenu * Menu, UIMenu * Parent, UIMenuSubMenu * SubMenu ) {
	Rectf qScreen( 0.f, 0.f, UIManager::instance()->getMainControl()->getRealSize().getWidth(), UIManager::instance()->getMainControl()->getRealSize().getHeight() );
	Rectf qPos( Pos.x, Pos.y, Pos.x + Menu->getRealSize().getWidth(), Pos.y + Menu->getRealSize().getHeight() );

	if ( NULL != Parent && NULL != SubMenu ) {
		Vector2i addToPos( 0, 0 );

		if ( NULL != SubMenu ) {
			addToPos.y = SubMenu->getRealSize().getHeight();
		}

		Vector2i sPos = SubMenu->getRealPosition();
		SubMenu->controlToScreen( sPos );

		Vector2i pPos = Parent->getRealPosition();
		Parent->controlToScreen( pPos );

		Rectf qParent( pPos.x, pPos.y, pPos.x + Parent->getRealSize().getWidth(), pPos.y + Parent->getRealSize().getHeight() );

		Pos.x		= qParent.Right;
		Pos.y		= sPos.y;
		qPos.Left	= Pos.x;
		qPos.Right	= qPos.Left + Menu->getRealSize().getWidth();
		qPos.Top	= Pos.y;
		qPos.Bottom	= qPos.Top + Menu->getRealSize().getHeight();

		if ( !qScreen.contains( qPos ) ) {
			Pos.y		= sPos.y + SubMenu->getRealSize().getHeight() - Menu->getRealSize().getHeight();
			qPos.Top	= Pos.y;
			qPos.Bottom	= qPos.Top + Menu->getRealSize().getHeight();

			if ( !qScreen.contains( qPos ) ) {
				Pos.x 		= qParent.Left - Menu->getRealSize().getWidth();
				Pos.y 		= sPos.y;
				qPos.Left	= Pos.x;
				qPos.Right	= qPos.Left + Menu->getRealSize().getWidth();
				qPos.Top	= Pos.y;
				qPos.Bottom	= qPos.Top + Menu->getRealSize().getHeight();

				if ( !qScreen.contains( qPos ) ) {
					Pos.y		= sPos.y + SubMenu->getRealSize().getHeight() - Menu->getRealSize().getHeight();
					qPos.Top	= Pos.y;
					qPos.Bottom	= qPos.Top + Menu->getRealSize().getHeight();
				}
			}
		}
	} else {
		if ( !qScreen.contains( qPos ) ) {
			Pos.y		-= Menu->getRealSize().getHeight();
			qPos.Top	-= Menu->getRealSize().getHeight();
			qPos.Bottom	-= Menu->getRealSize().getHeight();

			if ( !qScreen.contains( qPos ) ) {
				Pos.x		-= Menu->getRealSize().getWidth();
				qPos.Left	-= Menu->getRealSize().getWidth();
				qPos.Right	-= Menu->getRealSize().getWidth();

				if ( !qScreen.contains( qPos ) ) {
					Pos.y		+= Menu->getRealSize().getHeight();
					qPos.Top	+= Menu->getRealSize().getHeight();
					qPos.Bottom	+= Menu->getRealSize().getHeight();
				}
			}
		}
	}
}

}}
