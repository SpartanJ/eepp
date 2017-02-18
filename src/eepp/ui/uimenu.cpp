#include <eepp/ui/uimenu.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/font.hpp>

namespace EE { namespace UI {

UIMenu::UIMenu( UIMenu::CreateParams& Params ) :
	UIComplexControl( Params ),
	mPadding( Params.PaddingContainer ),
	mFont( Params.Font ),
	mFontColor( Params.FontColor ),
	mFontShadowColor( Params.FontShadowColor ),
	mFontOverColor( Params.FontOverColor ),
	mFontSelectedColor( Params.FontSelectedColor ),
	mMinWidth( Params.MinWidth ),
	mMinSpaceForIcons( Params.MinSpaceForIcons ),
	mMinRightMargin( Params.MinRightMargin ),
	mMaxWidth( 0 ),
	mRowHeight( Params.RowHeight ),
	mNextPosY( 0 ),
	mBiggestIcon( mMinSpaceForIcons ),
	mItemSelected( NULL ),
	mItemSelectedIndex( eeINDEX_NOT_FOUND ),
	mClickHide( false ),
	mLastTickMove( 0 )
{
	onSizeChange();

	applyDefaultTheme();
}

UIMenu::~UIMenu() {
}

Uint32 UIMenu::getType() const {
	return UI_TYPE_MENU;
}

bool UIMenu::isType( const Uint32& type ) const {
	return UIMenu::getType() == type ? true : UIComplexControl::isType( type );
}

void UIMenu::setTheme( UITheme * Theme ) {
	UIControl::setThemeControl( Theme, "menu" );
	doAftersetTheme();
}

void UIMenu::doAftersetTheme() {
	autoPadding();

	onSizeChange();
}

UIMenuItem * UIMenu::createMenuItem( const String& Text, SubTexture * Icon ) {
	UIMenuItem::CreateParams Params;
	Params.setParent( this );
	Params.Font 			= mFont;
	Params.FontColor 		= mFontColor;
	Params.FontShadowColor 	= mFontShadowColor;
	Params.FontOverColor 	= mFontOverColor;
	Params.Icon				= Icon;

	if ( mRowHeight < mMinSpaceForIcons )
		Params.IconMinSize		= Sizei( mMinSpaceForIcons, mRowHeight );
	else
		Params.IconMinSize		= Sizei( mMinSpaceForIcons, mMinSpaceForIcons );

	if ( mFlags & UI_AUTO_SIZE ) {
		Params.Flags		= UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	} else {
		Params.Flags		= UI_CLIP_ENABLE | UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	}

	UIMenuItem * tCtrl 	= eeNew( UIMenuItem, ( Params ) );

	tCtrl->setText( Text );
	tCtrl->setVisible( true );
	tCtrl->setEnabled( true );

	return tCtrl;
}

Uint32 UIMenu::add( const String& Text, SubTexture * Icon ) {
	return add( createMenuItem( Text, Icon ) );
}

UIMenuCheckBox * UIMenu::createMenuCheckBox( const String& Text, const bool &Active ) {
	UIMenuCheckBox::CreateParams Params;
	Params.setParent( this );
	Params.Font 			= mFont;
	Params.FontColor 		= mFontColor;
	Params.FontShadowColor 	= mFontShadowColor;
	Params.FontOverColor	= mFontOverColor;
	Params.Size				= Sizei( 0, mRowHeight );

	if ( mRowHeight < mMinSpaceForIcons )
		Params.IconMinSize		= Sizei( mMinSpaceForIcons, mRowHeight );
	else
		Params.IconMinSize		= Sizei( mMinSpaceForIcons, mMinSpaceForIcons );

	if ( mFlags & UI_AUTO_SIZE ) {
		Params.Flags		= UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	} else {
		Params.Flags		= UI_CLIP_ENABLE | UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	}

	UIMenuCheckBox * tCtrl 	= eeNew( UIMenuCheckBox, ( Params ) );

	tCtrl->setText( Text );
	tCtrl->setVisible( true );
	tCtrl->setEnabled( true );

	if ( Active )
		tCtrl->setActive( Active );

	return tCtrl;
}

Uint32 UIMenu::addCheckBox( const String& Text, const bool& Active ) {
	return add( createMenuCheckBox( Text, Active ) );
}

UIMenuSubMenu * UIMenu::createSubMenu( const String& Text, SubTexture * Icon, UIMenu * SubMenu ) {
	UIMenuSubMenu::CreateParams Params;
	Params.setParent( this );
	Params.Font 			= mFont;
	Params.FontColor 		= mFontColor;
	Params.FontShadowColor 	= mFontShadowColor;
	Params.FontOverColor 	= mFontOverColor;
	Params.SubMenu			= SubMenu;
	Params.Icon				= Icon;

	if ( mRowHeight < mMinSpaceForIcons )
		Params.IconMinSize		= Sizei( mMinSpaceForIcons, mRowHeight );
	else
		Params.IconMinSize		= Sizei( mMinSpaceForIcons, mMinSpaceForIcons );

	if ( mFlags & UI_AUTO_SIZE ) {
		Params.Flags		= UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	} else {
		Params.Flags		= UI_CLIP_ENABLE | UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	}

	UIMenuSubMenu * tCtrl 	= eeNew( UIMenuSubMenu, ( Params ) );

	tCtrl->setText( Text );
	tCtrl->setVisible( true );
	tCtrl->setEnabled( true );

	return tCtrl;
}

Uint32 UIMenu::addSubMenu( const String& Text, SubTexture * Icon, UIMenu * SubMenu ) {
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
				UIMenuSubMenu * tMenu = reinterpret_cast<UIMenuSubMenu*> ( tItem );

				if ( tMenu->getTextBox()->getTextWidth() + mBiggestIcon + tMenu->getArrow()->getSize().getWidth() + mMinRightMargin > (Int32)mMaxWidth - mPadding.Left - mPadding.Right ) {
					mMaxWidth = tMenu->getTextBox()->getTextWidth() + mBiggestIcon + mPadding.Left + mPadding.Right + tMenu->getArrow()->getSize().getWidth() + mMinRightMargin;

					if ( Resize ) {
						resizeControls();

						return true;
					}
				}
			}
			else {
				if ( tItem->getTextBox()->getTextWidth() + mBiggestIcon + mMinRightMargin > (Int32)mMaxWidth - mPadding.Left - mPadding.Right ) {
					mMaxWidth = tItem->getTextBox()->getTextWidth() + mBiggestIcon + mPadding.Left + mPadding.Right + mMinRightMargin;

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

	Control->setPosition( mPadding.Left, mPadding.Top + mNextPosY );

	mNextPosY += mRowHeight;

	mItems.push_back( Control );

	resizeMe();

	return mItems.size() - 1;
}

void UIMenu::setControlSize( UIControl * Control, const Uint32& Pos ) {
	if ( Control->isType( UI_TYPE_MENUITEM ) ) {
		Control->setSize( mSize.getWidth() - mPadding.Left - mPadding.Right, mRowHeight );
	} else {
		Control->setSize( mSize.getWidth() - mPadding.Left - mPadding.Right, Control->getSize().getHeight() );
	}
}

Uint32 UIMenu::addSeparator() {
	UISeparator::CreateParams Params;
	Params.setParent( this );
	Params.setPosition( mPadding.Left, mPadding.Top + mNextPosY );
	Params.Size = Sizei( mSize.getWidth() - mPadding.Left - mPadding.Right, 3 );

	UISeparator * Control = eeNew( UISeparator, ( Params ) );

	Control->setVisible( true );
	Control->setEnabled( true );

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

void UIMenu::insert( const String& Text, SubTexture * Icon, const Uint32& Index ) {
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
			UIControl * FocusCtrl = UIManager::instance()->focusControl();

			if ( this != FocusCtrl && !isParentOf( FocusCtrl ) && !isSubMenu( FocusCtrl ) ) {
				onComplexControlFocusLoss();
			}

			return 1;
		}
	}

	return 0;
}

void UIMenu::onSizeChange() {
	if ( NULL != mFont && ( ( mFlags & UI_AUTO_SIZE ) || 0 == mRowHeight ) ) {
		mRowHeight = mFont->getFontHeight() + 8;
	}

	if ( 0 != mMinWidth && mSize.getWidth() < (Int32)mMinWidth ) {
		setSize( mMinWidth, mNextPosY + mPadding.Top + mPadding.Bottom );
	}
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
	mBiggestIcon = mMinSpaceForIcons;

	if ( mFlags & UI_AUTO_SIZE ) {
		mMaxWidth = 0;

		for ( i = 0; i < mItems.size(); i++ ) {
			checkControlSize( mItems[i], false );
		}
	}

	for ( i = 0; i < mItems.size(); i++ ) {
		mItems[i]->setPosition( mPadding.Left, mPadding.Top + mNextPosY );

		mNextPosY += mItems[i]->getSize().getHeight();
	}

	resizeMe();
}

void UIMenu::resizeMe() {
	if ( mFlags & UI_AUTO_SIZE ) {
		setSize( mMaxWidth, mNextPosY + mPadding.Top + mPadding.Bottom );
	} else {
		setSize( mSize.getWidth(), mNextPosY + mPadding.Top + mPadding.Bottom );
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
		if ( !Ctrl->isType( UI_TYPE_SEPARATOR ) ) {
			setItemSelected( Ctrl );
		} else {
			Uint32 Index = getItemIndex( Ctrl );

			if ( Index != eeINDEX_NOT_FOUND ) {
				if ( Up ) {
					if ( Index > 0 ) {
						for ( Int32 i = (Int32)Index - 1; i >= 0; i-- ) {
							if ( !mItems[i]->isType( UI_TYPE_SEPARATOR ) ) {
								setItemSelected( mItems[i] );
								return;
							}
						}
					}

					setItemSelected( mItems[ mItems.size() ] );
				} else {
					for ( Uint32 i = Index + 1; i < mItems.size(); i++ ) {
						if ( !mItems[i]->isType( UI_TYPE_SEPARATOR ) ) {
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

	return UIComplexControl::onKeyDown( Event );
}

const Recti& UIMenu::getPadding() const {
	return mPadding;
}

void UIMenu::fixMenuPos( Vector2i& Pos, UIMenu * Menu, UIMenu * Parent, UIMenuSubMenu * SubMenu ) {
	eeAABB qScreen( 0.f, 0.f, UIManager::instance()->mainControl()->getSize().getWidth(), UIManager::instance()->mainControl()->getSize().getHeight() );
	eeAABB qPos( Pos.x, Pos.y, Pos.x + Menu->getSize().getWidth(), Pos.y + Menu->getSize().getHeight() );

	if ( NULL != Parent && NULL != SubMenu ) {
		Vector2i addToPos( 0, 0 );

		if ( NULL != SubMenu ) {
			addToPos.y = SubMenu->getSize().getHeight();
		}

		Vector2i sPos = SubMenu->getPosition();
		SubMenu->controlToScreen( sPos );

		Vector2i pPos = Parent->getPosition();
		Parent->controlToScreen( pPos );

		eeAABB qParent( pPos.x, pPos.y, pPos.x + Parent->getSize().getWidth(), pPos.y + Parent->getSize().getHeight() );

		Pos.x		= qParent.Right;
		Pos.y		= sPos.y;
		qPos.Left	= Pos.x;
		qPos.Right	= qPos.Left + Menu->getSize().getWidth();
		qPos.Top	= Pos.y;
		qPos.Bottom	= qPos.Top + Menu->getSize().getHeight();

		if ( !qScreen.contains( qPos ) ) {
			Pos.y		= sPos.y + SubMenu->getSize().getHeight() - Menu->getSize().getHeight();
			qPos.Top	= Pos.y;
			qPos.Bottom	= qPos.Top + Menu->getSize().getHeight();

			if ( !qScreen.contains( qPos ) ) {
				Pos.x 		= qParent.Left - Menu->getSize().getWidth();
				Pos.y 		= sPos.y;
				qPos.Left	= Pos.x;
				qPos.Right	= qPos.Left + Menu->getSize().getWidth();
				qPos.Top	= Pos.y;
				qPos.Bottom	= qPos.Top + Menu->getSize().getHeight();

				if ( !qScreen.contains( qPos ) ) {
					Pos.y		= sPos.y + SubMenu->getSize().getHeight() - Menu->getSize().getHeight();
					qPos.Top	= Pos.y;
					qPos.Bottom	= qPos.Top + Menu->getSize().getHeight();
				}
			}
		}
	} else {
		if ( !qScreen.contains( qPos ) ) {
			Pos.y		-= Menu->getSize().getHeight();
			qPos.Top	-= Menu->getSize().getHeight();
			qPos.Bottom	-= Menu->getSize().getHeight();

			if ( !qScreen.contains( qPos ) ) {
				Pos.x		-= Menu->getSize().getWidth();
				qPos.Left	-= Menu->getSize().getWidth();
				qPos.Right	-= Menu->getSize().getWidth();

				if ( !qScreen.contains( qPos ) ) {
					Pos.y		+= Menu->getSize().getHeight();
					qPos.Top	+= Menu->getSize().getHeight();
					qPos.Bottom	+= Menu->getSize().getHeight();
				}
			}
		}
	}
}

}}
