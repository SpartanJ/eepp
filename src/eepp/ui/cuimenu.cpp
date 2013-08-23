#include <eepp/ui/cuimenu.hpp>
#include <eepp/ui/cuimanager.hpp>
#include <eepp/graphics/cfont.hpp>

namespace EE { namespace UI {

cUIMenu::cUIMenu( cUIMenu::CreateParams& Params ) :
	cUIComplexControl( Params ),
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

cUIMenu::~cUIMenu() {
}

Uint32 cUIMenu::Type() const {
	return UI_TYPE_MENU;
}

bool cUIMenu::IsType( const Uint32& type ) const {
	return cUIMenu::Type() == type ? true : cUIComplexControl::IsType( type );
}

void cUIMenu::SetTheme( cUITheme * Theme ) {
	cUIControl::SetThemeControl( Theme, "menu" );
	DoAfterSetTheme();
}

void cUIMenu::DoAfterSetTheme() {
	AutoPadding();

	OnSizeChange();
}

cUIMenuItem * cUIMenu::CreateMenuItem( const String& Text, cSubTexture * Icon ) {
	cUIMenuItem::CreateParams Params;
	Params.Parent( this );
	Params.Font 			= mFont;
	Params.FontColor 		= mFontColor;
	Params.FontShadowColor 	= mFontShadowColor;
	Params.FontOverColor 	= mFontOverColor;
	Params.Icon				= Icon;

	if ( mRowHeight < mMinSpaceForIcons )
		Params.IconMinSize		= eeSize( mMinSpaceForIcons, mRowHeight );
	else
		Params.IconMinSize		= eeSize( mMinSpaceForIcons, mMinSpaceForIcons );

	if ( mFlags & UI_AUTO_SIZE ) {
		Params.Flags		= UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	} else {
		Params.Flags		= UI_CLIP_ENABLE | UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	}

	cUIMenuItem * tCtrl 	= eeNew( cUIMenuItem, ( Params ) );

	tCtrl->Text( Text );
	tCtrl->Visible( true );
	tCtrl->Enabled( true );

	return tCtrl;
}

Uint32 cUIMenu::Add( const String& Text, cSubTexture * Icon ) {
	return Add( CreateMenuItem( Text, Icon ) );
}

cUIMenuCheckBox * cUIMenu::CreateMenuCheckBox( const String& Text, const bool &Active ) {
	cUIMenuCheckBox::CreateParams Params;
	Params.Parent( this );
	Params.Font 			= mFont;
	Params.FontColor 		= mFontColor;
	Params.FontShadowColor 	= mFontShadowColor;
	Params.FontOverColor	= mFontOverColor;
	Params.Size				= eeSize( 0, mRowHeight );

	if ( mRowHeight < mMinSpaceForIcons )
		Params.IconMinSize		= eeSize( mMinSpaceForIcons, mRowHeight );
	else
		Params.IconMinSize		= eeSize( mMinSpaceForIcons, mMinSpaceForIcons );

	if ( mFlags & UI_AUTO_SIZE ) {
		Params.Flags		= UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	} else {
		Params.Flags		= UI_CLIP_ENABLE | UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	}

	cUIMenuCheckBox * tCtrl 	= eeNew( cUIMenuCheckBox, ( Params ) );

	tCtrl->Text( Text );
	tCtrl->Visible( true );
	tCtrl->Enabled( true );

	if ( Active )
		tCtrl->Active( Active );

	return tCtrl;
}

Uint32 cUIMenu::AddCheckBox( const String& Text, const bool& Active ) {
	return Add( CreateMenuCheckBox( Text, Active ) );
}

cUIMenuSubMenu * cUIMenu::CreateSubMenu( const String& Text, cSubTexture * Icon, cUIMenu * SubMenu ) {
	cUIMenuSubMenu::CreateParams Params;
	Params.Parent( this );
	Params.Font 			= mFont;
	Params.FontColor 		= mFontColor;
	Params.FontShadowColor 	= mFontShadowColor;
	Params.FontOverColor 	= mFontOverColor;
	Params.SubMenu			= SubMenu;
	Params.Icon				= Icon;

	if ( mRowHeight < mMinSpaceForIcons )
		Params.IconMinSize		= eeSize( mMinSpaceForIcons, mRowHeight );
	else
		Params.IconMinSize		= eeSize( mMinSpaceForIcons, mMinSpaceForIcons );

	if ( mFlags & UI_AUTO_SIZE ) {
		Params.Flags		= UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	} else {
		Params.Flags		= UI_CLIP_ENABLE | UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	}

	cUIMenuSubMenu * tCtrl 	= eeNew( cUIMenuSubMenu, ( Params ) );

	tCtrl->Text( Text );
	tCtrl->Visible( true );
	tCtrl->Enabled( true );

	return tCtrl;
}

Uint32 cUIMenu::AddSubMenu( const String& Text, cSubTexture * Icon, cUIMenu * SubMenu ) {
	return Add( CreateSubMenu( Text, Icon, SubMenu ) );
}

bool cUIMenu::CheckControlSize( cUIControl * Control, const bool& Resize ) {
	if ( Control->IsType( UI_TYPE_MENUITEM ) ) {
		cUIMenuItem * tItem = reinterpret_cast<cUIMenuItem*> ( Control );

		if ( NULL != tItem->Icon() && tItem->IconHorizontalMargin() + tItem->Icon()->Size().Width() > (Int32)mBiggestIcon ) {
			mBiggestIcon = tItem->IconHorizontalMargin() + tItem->Icon()->Size().Width();
		}

		if ( mFlags & UI_AUTO_SIZE ) {
			if ( Control->IsType( UI_TYPE_MENUSUBMENU ) ) {
				cUIMenuSubMenu * tMenu = reinterpret_cast<cUIMenuSubMenu*> ( tItem );

				if ( tMenu->TextBox()->GetTextWidth() + mBiggestIcon + tMenu->Arrow()->Size().Width() + mMinRightMargin > (Int32)mMaxWidth - mPadding.Left - mPadding.Right ) {
					mMaxWidth = tMenu->TextBox()->GetTextWidth() + mBiggestIcon + mPadding.Left + mPadding.Right + tMenu->Arrow()->Size().Width() + mMinRightMargin;

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

Uint32 cUIMenu::Add( cUIControl * Control ) {
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

void cUIMenu::SetControlSize( cUIControl * Control, const Uint32& Pos ) {
	if ( Control->IsType( UI_TYPE_MENUITEM ) ) {
		Control->Size( mSize.Width() - mPadding.Left - mPadding.Right, mRowHeight );
	} else {
		Control->Size( mSize.Width() - mPadding.Left - mPadding.Right, Control->Size().Height() );
	}
}

Uint32 cUIMenu::AddSeparator() {
	cUISeparator::CreateParams Params;
	Params.Parent( this );
	Params.PosSet( mPadding.Left, mPadding.Top + mNextPosY );
	Params.Size = eeSize( mSize.Width() - mPadding.Left - mPadding.Right, 3 );

	cUISeparator * Control = eeNew( cUISeparator, ( Params ) );

	Control->Visible( true );
	Control->Enabled( true );

	mNextPosY += Control->Size().Height();

	mItems.push_back( Control );

	ResizeMe();

	return mItems.size() - 1;
}

cUIControl * cUIMenu::GetItem( const Uint32& Index ) {
	eeASSERT( Index < mItems.size() );
	return mItems[ Index ];
}

cUIControl * cUIMenu::GetItem( const String& Text ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i]->IsType( UI_TYPE_MENUITEM ) ) {
			cUIMenuItem * tMenuItem = reinterpret_cast<cUIMenuItem*>( mItems[i] );
			
			if ( tMenuItem->Text() == Text )
				return tMenuItem;
		}
	}
	
	return NULL;
}

Uint32 cUIMenu::GetItemIndex( cUIControl * Item ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i] == Item )
			return i;
	}

	return eeINDEX_NOT_FOUND;
}

Uint32 cUIMenu::Count() const {
	return mItems.size();
}

void cUIMenu::Remove( const Uint32& Index ) {
	eeASSERT( Index < mItems.size() );

	eeSAFE_DELETE( mItems[ Index ] );

	mItems.erase( mItems.begin() + Index );

	ReposControls();
	ResizeControls();
}

void cUIMenu::Remove( cUIControl * Ctrl ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i] == Ctrl ) {
			Remove( i );
			break;
		}
	}
}

void cUIMenu::RemoveAll() {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		eeSAFE_DELETE( mItems[ i ] );
	}

	mItems.clear();
	mNextPosY = 0;
	mMaxWidth = 0;

	ResizeMe();
}

void cUIMenu::Insert( const String& Text, cSubTexture * Icon, const Uint32& Index ) {
	Insert( CreateMenuItem( Text, Icon ), Index );
}

void cUIMenu::Insert( cUIControl * Control, const Uint32& Index ) {
	mItems.insert( mItems.begin() + Index, Control );

	ChildAddAt( Control, Index );

	ReposControls();
	ResizeControls();
}

bool cUIMenu::IsSubMenu( cUIControl * Ctrl ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i]->IsType( UI_TYPE_MENUSUBMENU ) ) {
			cUIMenuSubMenu * tMenu = reinterpret_cast<cUIMenuSubMenu*> ( mItems[i] );

			if ( tMenu->SubMenu() == Ctrl )
				return true;
		}
	}

	return false;
}

Uint32 cUIMenu::OnMessage( const cUIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case cUIMessage::MsgMouseUp:
		{
			if ( Msg->Sender()->Parent() == this && ( Msg->Flags() & EE_BUTTONS_LRM ) ) {
				cUIEvent ItemEvent( Msg->Sender(), cUIEvent::EventOnItemClicked );
				SendEvent( &ItemEvent );
			}

			return 1;
		}
		case cUIMessage::MsgFocusLoss:
		{
			cUIControl * FocusCtrl = cUIManager::instance()->FocusControl();

			if ( this != FocusCtrl && !IsParentOf( FocusCtrl ) && !IsSubMenu( FocusCtrl ) ) {
				OnComplexControlFocusLoss();
			}

			return 1;
		}
	}

	return 0;
}

void cUIMenu::OnSizeChange() {
	if ( NULL != mFont && ( ( mFlags & UI_AUTO_SIZE ) || 0 == mRowHeight ) ) {
		mRowHeight = mFont->GetFontHeight() + 8;
	}

	if ( 0 != mMinWidth && mSize.Width() < (Int32)mMinWidth ) {
		Size( mMinWidth, mNextPosY + mPadding.Top + mPadding.Bottom );
	}
}

void cUIMenu::AutoPadding() {
	if ( mFlags & UI_AUTO_PADDING ) {
		mPadding = MakePadding();
	}
}

void cUIMenu::ResizeControls() {
	ResizeMe();

	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		SetControlSize( mItems[i], i );
	}
}

void cUIMenu::ReposControls() {
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

		mNextPosY += mItems[i]->Size().Height();
	}

	ResizeMe();
}

void cUIMenu::ResizeMe() {
	if ( mFlags & UI_AUTO_SIZE ) {
		Size( mMaxWidth, mNextPosY + mPadding.Top + mPadding.Bottom );
	} else {
		Size( mSize.Width(), mNextPosY + mPadding.Top + mPadding.Bottom );
	}
}

bool cUIMenu::Show() {
	Enabled( true );
	Visible( true );
	return true;
}

bool cUIMenu::Hide() {
	Enabled( false );
	Visible( false );

	if ( NULL != mItemSelected )
		mItemSelected->SetSkinState( cUISkinState::StateNormal );

	mItemSelected		= NULL;
	mItemSelectedIndex	= eeINDEX_NOT_FOUND;

	return true;
}

void cUIMenu::SetItemSelected( cUIControl * Item ) {
	if ( NULL != mItemSelected ) {
		if ( mItemSelected->IsType( UI_TYPE_MENUSUBMENU ) ) {
			cUIMenuSubMenu * tMenu = reinterpret_cast<cUIMenuSubMenu*> ( mItemSelected );

			if ( NULL != tMenu->SubMenu() )
				tMenu->SubMenu()->Hide();
		}

		mItemSelected->SetSkinState( cUISkinState::StateNormal );
	}

	if ( NULL != Item )
		Item->SetSkinState( cUISkinState::StateSelected );

	if ( mItemSelected != Item ) {
		mItemSelected		= Item;
		mItemSelectedIndex	= GetItemIndex( mItemSelected );
	}
}

void cUIMenu::TrySelect( cUIControl * Ctrl, bool Up ) {
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

void cUIMenu::NextSel() {
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

void cUIMenu::PrevSel() {
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

Uint32 cUIMenu::OnKeyDown( const cUIEventKey& Event ) {
	if ( Sys::GetTicks() - mLastTickMove > 50 ) {
		switch ( Event.KeyCode() ) {
			case KEY_DOWN:
				mLastTickMove = Sys::GetTicks();
				NextSel();

				break;
			case KEY_UP:
				mLastTickMove = Sys::GetTicks();
				PrevSel();

				break;
			case KEY_RIGHT:
				if ( NULL != mItemSelected && ( mItemSelected->IsType( UI_TYPE_MENUSUBMENU ) ) ) {
					cUIMenuSubMenu * tMenu = reinterpret_cast<cUIMenuSubMenu*> ( mItemSelected );

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
					mItemSelected->SendMouseEvent(cUIEvent::EventMouseClick, cUIManager::instance()->GetMousePos(), EE_BUTTONS_ALL );

					cUIMessage Msg( mItemSelected, cUIMessage::MsgMouseUp, EE_BUTTONS_ALL );
					mItemSelected->MessagePost( &Msg );
				}

				break;
		}
	}

	return cUIComplexControl::OnKeyDown( Event );
}

const eeRecti& cUIMenu::Padding() const {
	return mPadding;
}

void cUIMenu::FixMenuPos( eeVector2i& Pos, cUIMenu * Menu, cUIMenu * Parent, cUIMenuSubMenu * SubMenu ) {
	eeAABB qScreen( 0.f, 0.f, cUIManager::instance()->MainControl()->Size().Width(), cUIManager::instance()->MainControl()->Size().Height() );
	eeAABB qPos( Pos.x, Pos.y, Pos.x + Menu->Size().Width(), Pos.y + Menu->Size().Height() );

	if ( NULL != Parent && NULL != SubMenu ) {
		eeVector2i addToPos( 0, 0 );

		if ( NULL != SubMenu ) {
			addToPos.y = SubMenu->Size().Height();
		}

		eeVector2i sPos = SubMenu->Pos();
		SubMenu->ControlToScreen( sPos );

		eeVector2i pPos = Parent->Pos();
		Parent->ControlToScreen( pPos );

		eeAABB qParent( pPos.x, pPos.y, pPos.x + Parent->Size().Width(), pPos.y + Parent->Size().Height() );

		Pos.x		= qParent.Right;
		Pos.y		= sPos.y;
		qPos.Left	= Pos.x;
		qPos.Right	= qPos.Left + Menu->Size().Width();
		qPos.Top	= Pos.y;
		qPos.Bottom	= qPos.Top + Menu->Size().Height();

		if ( !qScreen.Contains( qPos ) ) {
			Pos.y		= sPos.y + SubMenu->Size().Height() - Menu->Size().Height();
			qPos.Top	= Pos.y;
			qPos.Bottom	= qPos.Top + Menu->Size().Height();

			if ( !qScreen.Contains( qPos ) ) {
				Pos.x 		= qParent.Left - Menu->Size().Width();
				Pos.y 		= sPos.y;
				qPos.Left	= Pos.x;
				qPos.Right	= qPos.Left + Menu->Size().Width();
				qPos.Top	= Pos.y;
				qPos.Bottom	= qPos.Top + Menu->Size().Height();

				if ( !qScreen.Contains( qPos ) ) {
					Pos.y		= sPos.y + SubMenu->Size().Height() - Menu->Size().Height();
					qPos.Top	= Pos.y;
					qPos.Bottom	= qPos.Top + Menu->Size().Height();
				}
			}
		}
	} else {
		if ( !qScreen.Contains( qPos ) ) {
			Pos.y		-= Menu->Size().Height();
			qPos.Top	-= Menu->Size().Height();
			qPos.Bottom	-= Menu->Size().Height();

			if ( !qScreen.Contains( qPos ) ) {
				Pos.x		-= Menu->Size().Width();
				qPos.Left	-= Menu->Size().Width();
				qPos.Right	-= Menu->Size().Width();

				if ( !qScreen.Contains( qPos ) ) {
					Pos.y		+= Menu->Size().Height();
					qPos.Top	+= Menu->Size().Height();
					qPos.Bottom	+= Menu->Size().Height();
				}
			}
		}
	}
}

}}
