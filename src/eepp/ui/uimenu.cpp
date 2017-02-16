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
	AutoPadding();

	onSizeChange();
}

UIMenuItem * UIMenu::CreateMenuItem( const String& Text, SubTexture * Icon ) {
	UIMenuItem::CreateParams Params;
	Params.Parent( this );
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

	tCtrl->text( Text );
	tCtrl->visible( true );
	tCtrl->enabled( true );

	return tCtrl;
}

Uint32 UIMenu::Add( const String& Text, SubTexture * Icon ) {
	return Add( CreateMenuItem( Text, Icon ) );
}

UIMenuCheckBox * UIMenu::CreateMenuCheckBox( const String& Text, const bool &Active ) {
	UIMenuCheckBox::CreateParams Params;
	Params.Parent( this );
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

	tCtrl->text( Text );
	tCtrl->visible( true );
	tCtrl->enabled( true );

	if ( Active )
		tCtrl->active( Active );

	return tCtrl;
}

Uint32 UIMenu::AddCheckBox( const String& Text, const bool& Active ) {
	return Add( CreateMenuCheckBox( Text, Active ) );
}

UIMenuSubMenu * UIMenu::CreateSubMenu( const String& Text, SubTexture * Icon, UIMenu * SubMenu ) {
	UIMenuSubMenu::CreateParams Params;
	Params.Parent( this );
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

	tCtrl->text( Text );
	tCtrl->visible( true );
	tCtrl->enabled( true );

	return tCtrl;
}

Uint32 UIMenu::AddSubMenu( const String& Text, SubTexture * Icon, UIMenu * SubMenu ) {
	return Add( CreateSubMenu( Text, Icon, SubMenu ) );
}

bool UIMenu::CheckControlSize( UIControl * Control, const bool& Resize ) {
	if ( Control->isType( UI_TYPE_MENUITEM ) ) {
		UIMenuItem * tItem = reinterpret_cast<UIMenuItem*> ( Control );

		if ( NULL != tItem->icon() && tItem->iconHorizontalMargin() + tItem->icon()->size().width() > (Int32)mBiggestIcon ) {
			mBiggestIcon = tItem->iconHorizontalMargin() + tItem->icon()->size().width();
		}

		if ( mFlags & UI_AUTO_SIZE ) {
			if ( Control->isType( UI_TYPE_MENUSUBMENU ) ) {
				UIMenuSubMenu * tMenu = reinterpret_cast<UIMenuSubMenu*> ( tItem );

				if ( tMenu->getTextBox()->getTextWidth() + mBiggestIcon + tMenu->getArrow()->size().width() + mMinRightMargin > (Int32)mMaxWidth - mPadding.Left - mPadding.Right ) {
					mMaxWidth = tMenu->getTextBox()->getTextWidth() + mBiggestIcon + mPadding.Left + mPadding.Right + tMenu->getArrow()->size().width() + mMinRightMargin;

					if ( Resize ) {
						ResizeControls();

						return true;
					}
				}
			}
			else {
				if ( tItem->getTextBox()->getTextWidth() + mBiggestIcon + mMinRightMargin > (Int32)mMaxWidth - mPadding.Left - mPadding.Right ) {
					mMaxWidth = tItem->getTextBox()->getTextWidth() + mBiggestIcon + mPadding.Left + mPadding.Right + mMinRightMargin;

					if ( Resize ) {
						ResizeControls();

						return true;
					}
				}
			}
		}
	}

	return false;
}

Uint32 UIMenu::Add( UIControl * Control ) {
	if ( this != Control->parent() )
		Control->parent( this );

	CheckControlSize( Control );

	SetControlSize( Control, Count() );

	Control->position( mPadding.Left, mPadding.Top + mNextPosY );

	mNextPosY += mRowHeight;

	mItems.push_back( Control );

	ResizeMe();

	return mItems.size() - 1;
}

void UIMenu::SetControlSize( UIControl * Control, const Uint32& Pos ) {
	if ( Control->isType( UI_TYPE_MENUITEM ) ) {
		Control->size( mSize.width() - mPadding.Left - mPadding.Right, mRowHeight );
	} else {
		Control->size( mSize.width() - mPadding.Left - mPadding.Right, Control->size().height() );
	}
}

Uint32 UIMenu::AddSeparator() {
	UISeparator::CreateParams Params;
	Params.Parent( this );
	Params.PosSet( mPadding.Left, mPadding.Top + mNextPosY );
	Params.Size = Sizei( mSize.width() - mPadding.Left - mPadding.Right, 3 );

	UISeparator * Control = eeNew( UISeparator, ( Params ) );

	Control->visible( true );
	Control->enabled( true );

	mNextPosY += Control->size().height();

	mItems.push_back( Control );

	ResizeMe();

	return mItems.size() - 1;
}

UIControl * UIMenu::GetItem( const Uint32& Index ) {
	eeASSERT( Index < mItems.size() );
	return mItems[ Index ];
}

UIControl * UIMenu::GetItem( const String& Text ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i]->isType( UI_TYPE_MENUITEM ) ) {
			UIMenuItem * tMenuItem = reinterpret_cast<UIMenuItem*>( mItems[i] );
			
			if ( tMenuItem->text() == Text )
				return tMenuItem;
		}
	}
	
	return NULL;
}

Uint32 UIMenu::GetItemIndex( UIControl * Item ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i] == Item )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Uint32 UIMenu::Count() const {
	return mItems.size();
}

void UIMenu::Remove( const Uint32& Index ) {
	eeASSERT( Index < mItems.size() );

	eeSAFE_DELETE( mItems[ Index ] );

	mItems.erase( mItems.begin() + Index );

	ReposControls();
	ResizeControls();
}

void UIMenu::Remove( UIControl * Ctrl ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i] == Ctrl ) {
			Remove( i );
			break;
		}
	}
}

void UIMenu::RemoveAll() {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		eeSAFE_DELETE( mItems[ i ] );
	}

	mItems.clear();
	mNextPosY = 0;
	mMaxWidth = 0;

	ResizeMe();
}

void UIMenu::Insert( const String& Text, SubTexture * Icon, const Uint32& Index ) {
	Insert( CreateMenuItem( Text, Icon ), Index );
}

void UIMenu::Insert( UIControl * Control, const Uint32& Index ) {
	mItems.insert( mItems.begin() + Index, Control );

	childAddAt( Control, Index );

	ReposControls();
	ResizeControls();
}

bool UIMenu::IsSubMenu( UIControl * Ctrl ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i]->isType( UI_TYPE_MENUSUBMENU ) ) {
			UIMenuSubMenu * tMenu = reinterpret_cast<UIMenuSubMenu*> ( mItems[i] );

			if ( tMenu->subMenu() == Ctrl )
				return true;
		}
	}

	return false;
}

Uint32 UIMenu::onMessage( const UIMessage * Msg ) {
	switch ( Msg->getMsg() ) {
		case UIMessage::MsgMouseUp:
		{
			if ( Msg->getSender()->parent() == this && ( Msg->getFlags() & EE_BUTTONS_LRM ) ) {
				UIEvent ItemEvent( Msg->getSender(), UIEvent::EventOnItemClicked );
				sendEvent( &ItemEvent );
			}

			return 1;
		}
		case UIMessage::MsgFocusLoss:
		{
			UIControl * FocusCtrl = UIManager::instance()->focusControl();

			if ( this != FocusCtrl && !isParentOf( FocusCtrl ) && !IsSubMenu( FocusCtrl ) ) {
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

	if ( 0 != mMinWidth && mSize.width() < (Int32)mMinWidth ) {
		size( mMinWidth, mNextPosY + mPadding.Top + mPadding.Bottom );
	}
}

void UIMenu::AutoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPadding = makePadding();
	}
}

void UIMenu::ResizeControls() {
	ResizeMe();

	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		SetControlSize( mItems[i], i );
	}
}

void UIMenu::ReposControls() {
	Uint32 i;
	mNextPosY = 0;
	mBiggestIcon = mMinSpaceForIcons;

	if ( mFlags & UI_AUTO_SIZE ) {
		mMaxWidth = 0;

		for ( i = 0; i < mItems.size(); i++ ) {
			CheckControlSize( mItems[i], false );
		}
	}

	for ( i = 0; i < mItems.size(); i++ ) {
		mItems[i]->position( mPadding.Left, mPadding.Top + mNextPosY );

		mNextPosY += mItems[i]->size().height();
	}

	ResizeMe();
}

void UIMenu::ResizeMe() {
	if ( mFlags & UI_AUTO_SIZE ) {
		size( mMaxWidth, mNextPosY + mPadding.Top + mPadding.Bottom );
	} else {
		size( mSize.width(), mNextPosY + mPadding.Top + mPadding.Bottom );
	}
}

bool UIMenu::show() {
	enabled( true );
	visible( true );
	return true;
}

bool UIMenu::hide() {
	enabled( false );
	visible( false );

	if ( NULL != mItemSelected )
		mItemSelected->setSkinState( UISkinState::StateNormal );

	mItemSelected		= NULL;
	mItemSelectedIndex	= eeINDEX_NOT_FOUND;

	return true;
}

void UIMenu::SetItemSelected( UIControl * Item ) {
	if ( NULL != mItemSelected ) {
		if ( mItemSelected->isType( UI_TYPE_MENUSUBMENU ) ) {
			UIMenuSubMenu * tMenu = reinterpret_cast<UIMenuSubMenu*> ( mItemSelected );

			if ( NULL != tMenu->subMenu() )
				tMenu->subMenu()->hide();
		}

		mItemSelected->setSkinState( UISkinState::StateNormal );
	}

	if ( NULL != Item )
		Item->setSkinState( UISkinState::StateSelected );

	if ( mItemSelected != Item ) {
		mItemSelected		= Item;
		mItemSelectedIndex	= GetItemIndex( mItemSelected );
	}
}

void UIMenu::TrySelect( UIControl * Ctrl, bool Up ) {
	if ( mItems.size() ) {
		if ( !Ctrl->isType( UI_TYPE_SEPARATOR ) ) {
			SetItemSelected( Ctrl );
		} else {
			Uint32 Index = GetItemIndex( Ctrl );

			if ( Index != eeINDEX_NOT_FOUND ) {
				if ( Up ) {
					if ( Index > 0 ) {
						for ( Int32 i = (Int32)Index - 1; i >= 0; i-- ) {
							if ( !mItems[i]->isType( UI_TYPE_SEPARATOR ) ) {
								SetItemSelected( mItems[i] );
								return;
							}
						}
					}

					SetItemSelected( mItems[ mItems.size() ] );
				} else {
					for ( Uint32 i = Index + 1; i < mItems.size(); i++ ) {
						if ( !mItems[i]->isType( UI_TYPE_SEPARATOR ) ) {
							SetItemSelected( mItems[i] );
							return;
						}
					}

					SetItemSelected( mItems[0] );
				}
			}
		}
	}
}

void UIMenu::NextSel() {
	if ( mItems.size() ) {
		if ( mItemSelectedIndex != eeINDEX_NOT_FOUND ) {
			if ( mItemSelectedIndex + 1 < mItems.size() ) {
				TrySelect( mItems[ mItemSelectedIndex + 1 ], false );
			} else {
				TrySelect( mItems[0], false );
			}
		} else {
			TrySelect( mItems[0], false );
		}
	}
}

void UIMenu::PrevSel() {
	if ( mItems.size() ) {
		if (  mItemSelectedIndex != eeINDEX_NOT_FOUND  ) {
			if ( mItemSelectedIndex >= 1 ) {
				TrySelect( mItems[ mItemSelectedIndex - 1 ], true );
			} else {
				TrySelect( mItems[ mItems.size() - 1 ], true );
			}
		} else {
			TrySelect( mItems[0], true );
		}
	}
}

Uint32 UIMenu::onKeyDown( const UIEventKey& Event ) {
	if ( Sys::getTicks() - mLastTickMove > 50 ) {
		switch ( Event.getKeyCode() ) {
			case KEY_DOWN:
				mLastTickMove = Sys::getTicks();
				NextSel();

				break;
			case KEY_UP:
				mLastTickMove = Sys::getTicks();
				PrevSel();

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

const Recti& UIMenu::Padding() const {
	return mPadding;
}

void UIMenu::FixMenuPos( Vector2i& Pos, UIMenu * Menu, UIMenu * Parent, UIMenuSubMenu * SubMenu ) {
	eeAABB qScreen( 0.f, 0.f, UIManager::instance()->mainControl()->size().width(), UIManager::instance()->mainControl()->size().height() );
	eeAABB qPos( Pos.x, Pos.y, Pos.x + Menu->size().width(), Pos.y + Menu->size().height() );

	if ( NULL != Parent && NULL != SubMenu ) {
		Vector2i addToPos( 0, 0 );

		if ( NULL != SubMenu ) {
			addToPos.y = SubMenu->size().height();
		}

		Vector2i sPos = SubMenu->position();
		SubMenu->controlToScreen( sPos );

		Vector2i pPos = Parent->position();
		Parent->controlToScreen( pPos );

		eeAABB qParent( pPos.x, pPos.y, pPos.x + Parent->size().width(), pPos.y + Parent->size().height() );

		Pos.x		= qParent.Right;
		Pos.y		= sPos.y;
		qPos.Left	= Pos.x;
		qPos.Right	= qPos.Left + Menu->size().width();
		qPos.Top	= Pos.y;
		qPos.Bottom	= qPos.Top + Menu->size().height();

		if ( !qScreen.contains( qPos ) ) {
			Pos.y		= sPos.y + SubMenu->size().height() - Menu->size().height();
			qPos.Top	= Pos.y;
			qPos.Bottom	= qPos.Top + Menu->size().height();

			if ( !qScreen.contains( qPos ) ) {
				Pos.x 		= qParent.Left - Menu->size().width();
				Pos.y 		= sPos.y;
				qPos.Left	= Pos.x;
				qPos.Right	= qPos.Left + Menu->size().width();
				qPos.Top	= Pos.y;
				qPos.Bottom	= qPos.Top + Menu->size().height();

				if ( !qScreen.contains( qPos ) ) {
					Pos.y		= sPos.y + SubMenu->size().height() - Menu->size().height();
					qPos.Top	= Pos.y;
					qPos.Bottom	= qPos.Top + Menu->size().height();
				}
			}
		}
	} else {
		if ( !qScreen.contains( qPos ) ) {
			Pos.y		-= Menu->size().height();
			qPos.Top	-= Menu->size().height();
			qPos.Bottom	-= Menu->size().height();

			if ( !qScreen.contains( qPos ) ) {
				Pos.x		-= Menu->size().width();
				qPos.Left	-= Menu->size().width();
				qPos.Right	-= Menu->size().width();

				if ( !qScreen.contains( qPos ) ) {
					Pos.y		+= Menu->size().height();
					qPos.Top	+= Menu->size().height();
					qPos.Bottom	+= Menu->size().height();
				}
			}
		}
	}
}

}}
