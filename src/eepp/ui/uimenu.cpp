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
	OnSizeChange();

	ApplyDefaultTheme();
}

UIMenu::~UIMenu() {
}

Uint32 UIMenu::Type() const {
	return UI_TYPE_MENU;
}

bool UIMenu::IsType( const Uint32& type ) const {
	return UIMenu::Type() == type ? true : UIComplexControl::IsType( type );
}

void UIMenu::SetTheme( UITheme * Theme ) {
	UIControl::SetThemeControl( Theme, "menu" );
	DoAfterSetTheme();
}

void UIMenu::DoAfterSetTheme() {
	AutoPadding();

	OnSizeChange();
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

	tCtrl->Text( Text );
	tCtrl->Visible( true );
	tCtrl->Enabled( true );

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

	tCtrl->Text( Text );
	tCtrl->Visible( true );
	tCtrl->Enabled( true );

	if ( Active )
		tCtrl->Active( Active );

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

	tCtrl->Text( Text );
	tCtrl->Visible( true );
	tCtrl->Enabled( true );

	return tCtrl;
}

Uint32 UIMenu::AddSubMenu( const String& Text, SubTexture * Icon, UIMenu * SubMenu ) {
	return Add( CreateSubMenu( Text, Icon, SubMenu ) );
}

bool UIMenu::CheckControlSize( UIControl * Control, const bool& Resize ) {
	if ( Control->IsType( UI_TYPE_MENUITEM ) ) {
		UIMenuItem * tItem = reinterpret_cast<UIMenuItem*> ( Control );

		if ( NULL != tItem->Icon() && tItem->IconHorizontalMargin() + tItem->Icon()->Size().width() > (Int32)mBiggestIcon ) {
			mBiggestIcon = tItem->IconHorizontalMargin() + tItem->Icon()->Size().width();
		}

		if ( mFlags & UI_AUTO_SIZE ) {
			if ( Control->IsType( UI_TYPE_MENUSUBMENU ) ) {
				UIMenuSubMenu * tMenu = reinterpret_cast<UIMenuSubMenu*> ( tItem );

				if ( tMenu->TextBox()->GetTextWidth() + mBiggestIcon + tMenu->Arrow()->Size().width() + mMinRightMargin > (Int32)mMaxWidth - mPadding.Left - mPadding.Right ) {
					mMaxWidth = tMenu->TextBox()->GetTextWidth() + mBiggestIcon + mPadding.Left + mPadding.Right + tMenu->Arrow()->Size().width() + mMinRightMargin;

					if ( Resize ) {
						ResizeControls();

						return true;
					}
				}
			}
			else {
				if ( tItem->TextBox()->GetTextWidth() + mBiggestIcon + mMinRightMargin > (Int32)mMaxWidth - mPadding.Left - mPadding.Right ) {
					mMaxWidth = tItem->TextBox()->GetTextWidth() + mBiggestIcon + mPadding.Left + mPadding.Right + mMinRightMargin;

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
	if ( this != Control->Parent() )
		Control->Parent( this );

	CheckControlSize( Control );

	SetControlSize( Control, Count() );

	Control->Pos( mPadding.Left, mPadding.Top + mNextPosY );

	mNextPosY += mRowHeight;

	mItems.push_back( Control );

	ResizeMe();

	return mItems.size() - 1;
}

void UIMenu::SetControlSize( UIControl * Control, const Uint32& Pos ) {
	if ( Control->IsType( UI_TYPE_MENUITEM ) ) {
		Control->Size( mSize.width() - mPadding.Left - mPadding.Right, mRowHeight );
	} else {
		Control->Size( mSize.width() - mPadding.Left - mPadding.Right, Control->Size().height() );
	}
}

Uint32 UIMenu::AddSeparator() {
	UISeparator::CreateParams Params;
	Params.Parent( this );
	Params.PosSet( mPadding.Left, mPadding.Top + mNextPosY );
	Params.Size = Sizei( mSize.width() - mPadding.Left - mPadding.Right, 3 );

	UISeparator * Control = eeNew( UISeparator, ( Params ) );

	Control->Visible( true );
	Control->Enabled( true );

	mNextPosY += Control->Size().height();

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
		if ( mItems[i]->IsType( UI_TYPE_MENUITEM ) ) {
			UIMenuItem * tMenuItem = reinterpret_cast<UIMenuItem*>( mItems[i] );
			
			if ( tMenuItem->Text() == Text )
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

	ChildAddAt( Control, Index );

	ReposControls();
	ResizeControls();
}

bool UIMenu::IsSubMenu( UIControl * Ctrl ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i]->IsType( UI_TYPE_MENUSUBMENU ) ) {
			UIMenuSubMenu * tMenu = reinterpret_cast<UIMenuSubMenu*> ( mItems[i] );

			if ( tMenu->SubMenu() == Ctrl )
				return true;
		}
	}

	return false;
}

Uint32 UIMenu::OnMessage( const UIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case UIMessage::MsgMouseUp:
		{
			if ( Msg->Sender()->Parent() == this && ( Msg->Flags() & EE_BUTTONS_LRM ) ) {
				UIEvent ItemEvent( Msg->Sender(), UIEvent::EventOnItemClicked );
				SendEvent( &ItemEvent );
			}

			return 1;
		}
		case UIMessage::MsgFocusLoss:
		{
			UIControl * FocusCtrl = UIManager::instance()->FocusControl();

			if ( this != FocusCtrl && !IsParentOf( FocusCtrl ) && !IsSubMenu( FocusCtrl ) ) {
				OnComplexControlFocusLoss();
			}

			return 1;
		}
	}

	return 0;
}

void UIMenu::OnSizeChange() {
	if ( NULL != mFont && ( ( mFlags & UI_AUTO_SIZE ) || 0 == mRowHeight ) ) {
		mRowHeight = mFont->GetFontHeight() + 8;
	}

	if ( 0 != mMinWidth && mSize.width() < (Int32)mMinWidth ) {
		Size( mMinWidth, mNextPosY + mPadding.Top + mPadding.Bottom );
	}
}

void UIMenu::AutoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPadding = MakePadding();
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
		mItems[i]->Pos( mPadding.Left, mPadding.Top + mNextPosY );

		mNextPosY += mItems[i]->Size().height();
	}

	ResizeMe();
}

void UIMenu::ResizeMe() {
	if ( mFlags & UI_AUTO_SIZE ) {
		Size( mMaxWidth, mNextPosY + mPadding.Top + mPadding.Bottom );
	} else {
		Size( mSize.width(), mNextPosY + mPadding.Top + mPadding.Bottom );
	}
}

bool UIMenu::Show() {
	Enabled( true );
	Visible( true );
	return true;
}

bool UIMenu::Hide() {
	Enabled( false );
	Visible( false );

	if ( NULL != mItemSelected )
		mItemSelected->SetSkinState( UISkinState::StateNormal );

	mItemSelected		= NULL;
	mItemSelectedIndex	= eeINDEX_NOT_FOUND;

	return true;
}

void UIMenu::SetItemSelected( UIControl * Item ) {
	if ( NULL != mItemSelected ) {
		if ( mItemSelected->IsType( UI_TYPE_MENUSUBMENU ) ) {
			UIMenuSubMenu * tMenu = reinterpret_cast<UIMenuSubMenu*> ( mItemSelected );

			if ( NULL != tMenu->SubMenu() )
				tMenu->SubMenu()->Hide();
		}

		mItemSelected->SetSkinState( UISkinState::StateNormal );
	}

	if ( NULL != Item )
		Item->SetSkinState( UISkinState::StateSelected );

	if ( mItemSelected != Item ) {
		mItemSelected		= Item;
		mItemSelectedIndex	= GetItemIndex( mItemSelected );
	}
}

void UIMenu::TrySelect( UIControl * Ctrl, bool Up ) {
	if ( mItems.size() ) {
		if ( !Ctrl->IsType( UI_TYPE_SEPARATOR ) ) {
			SetItemSelected( Ctrl );
		} else {
			Uint32 Index = GetItemIndex( Ctrl );

			if ( Index != eeINDEX_NOT_FOUND ) {
				if ( Up ) {
					if ( Index > 0 ) {
						for ( Int32 i = (Int32)Index - 1; i >= 0; i-- ) {
							if ( !mItems[i]->IsType( UI_TYPE_SEPARATOR ) ) {
								SetItemSelected( mItems[i] );
								return;
							}
						}
					}

					SetItemSelected( mItems[ mItems.size() ] );
				} else {
					for ( Uint32 i = Index + 1; i < mItems.size(); i++ ) {
						if ( !mItems[i]->IsType( UI_TYPE_SEPARATOR ) ) {
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

Uint32 UIMenu::OnKeyDown( const UIEventKey& Event ) {
	if ( Sys::getTicks() - mLastTickMove > 50 ) {
		switch ( Event.KeyCode() ) {
			case KEY_DOWN:
				mLastTickMove = Sys::getTicks();
				NextSel();

				break;
			case KEY_UP:
				mLastTickMove = Sys::getTicks();
				PrevSel();

				break;
			case KEY_RIGHT:
				if ( NULL != mItemSelected && ( mItemSelected->IsType( UI_TYPE_MENUSUBMENU ) ) ) {
					UIMenuSubMenu * tMenu = reinterpret_cast<UIMenuSubMenu*> ( mItemSelected );

					tMenu->ShowSubMenu();
				}

				break;
			case KEY_LEFT:
				Hide();

				break;
			case KEY_ESCAPE:
				Hide();

				break;
			case KEY_RETURN:
				if ( NULL != mItemSelected ) {
					mItemSelected->SendMouseEvent(UIEvent::EventMouseClick, UIManager::instance()->GetMousePos(), EE_BUTTONS_ALL );

					UIMessage Msg( mItemSelected, UIMessage::MsgMouseUp, EE_BUTTONS_ALL );
					mItemSelected->MessagePost( &Msg );
				}

				break;
		}
	}

	return UIComplexControl::OnKeyDown( Event );
}

const Recti& UIMenu::Padding() const {
	return mPadding;
}

void UIMenu::FixMenuPos( Vector2i& Pos, UIMenu * Menu, UIMenu * Parent, UIMenuSubMenu * SubMenu ) {
	eeAABB qScreen( 0.f, 0.f, UIManager::instance()->MainControl()->Size().width(), UIManager::instance()->MainControl()->Size().height() );
	eeAABB qPos( Pos.x, Pos.y, Pos.x + Menu->Size().width(), Pos.y + Menu->Size().height() );

	if ( NULL != Parent && NULL != SubMenu ) {
		Vector2i addToPos( 0, 0 );

		if ( NULL != SubMenu ) {
			addToPos.y = SubMenu->Size().height();
		}

		Vector2i sPos = SubMenu->Pos();
		SubMenu->ControlToScreen( sPos );

		Vector2i pPos = Parent->Pos();
		Parent->ControlToScreen( pPos );

		eeAABB qParent( pPos.x, pPos.y, pPos.x + Parent->Size().width(), pPos.y + Parent->Size().height() );

		Pos.x		= qParent.Right;
		Pos.y		= sPos.y;
		qPos.Left	= Pos.x;
		qPos.Right	= qPos.Left + Menu->Size().width();
		qPos.Top	= Pos.y;
		qPos.Bottom	= qPos.Top + Menu->Size().height();

		if ( !qScreen.contains( qPos ) ) {
			Pos.y		= sPos.y + SubMenu->Size().height() - Menu->Size().height();
			qPos.Top	= Pos.y;
			qPos.Bottom	= qPos.Top + Menu->Size().height();

			if ( !qScreen.contains( qPos ) ) {
				Pos.x 		= qParent.Left - Menu->Size().width();
				Pos.y 		= sPos.y;
				qPos.Left	= Pos.x;
				qPos.Right	= qPos.Left + Menu->Size().width();
				qPos.Top	= Pos.y;
				qPos.Bottom	= qPos.Top + Menu->Size().height();

				if ( !qScreen.contains( qPos ) ) {
					Pos.y		= sPos.y + SubMenu->Size().height() - Menu->Size().height();
					qPos.Top	= Pos.y;
					qPos.Bottom	= qPos.Top + Menu->Size().height();
				}
			}
		}
	} else {
		if ( !qScreen.contains( qPos ) ) {
			Pos.y		-= Menu->Size().height();
			qPos.Top	-= Menu->Size().height();
			qPos.Bottom	-= Menu->Size().height();

			if ( !qScreen.contains( qPos ) ) {
				Pos.x		-= Menu->Size().width();
				qPos.Left	-= Menu->Size().width();
				qPos.Right	-= Menu->Size().width();

				if ( !qScreen.contains( qPos ) ) {
					Pos.y		+= Menu->Size().height();
					qPos.Top	+= Menu->Size().height();
					qPos.Bottom	+= Menu->Size().height();
				}
			}
		}
	}
}

}}
