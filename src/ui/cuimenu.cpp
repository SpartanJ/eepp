#include "cuimenu.hpp"
#include "cuimanager.hpp"

namespace EE { namespace UI {

cUIMenu::cUIMenu( cUIMenu::CreateParams& Params ) :
	cUIControlAnim( Params ),
	mPadding( Params.PaddingContainer ),
	mFont( Params.Font ),
	mFontColor( Params.FontColor ),
	mFontShadowColor( Params.FontShadowColor ),
	mFontOverColor( Params.FontOverColor ),
	mMinWidth( Params.MinWidth ),
	mMaxWidth( 0 ),
	mRowHeight( Params.RowHeight ),
	mNextPosY( 0 ),
	mBiggestIcon( 0 )
{
	mType |= UI_TYPE_GET( UI_TYPE_MENU );

	OnSizeChange();

	ApplyDefaultTheme();
}

cUIMenu::~cUIMenu() {
}

void cUIMenu::SetTheme( cUITheme * Theme ) {
	cUIControl::SetTheme( Theme, "menu" );
	DoAfterSetTheme();
}

void cUIMenu::DoAfterSetTheme() {
	AutoPadding();

	OnSizeChange();
}

cUIMenuItem * cUIMenu::CreateMenuItem( const std::wstring& Text, cShape * Icon ) {
	cUIMenuItem::CreateParams Params;
	Params.Parent( this );
	Params.Font 			= mFont;
	Params.FontColor 		= mFontColor;
	Params.FontShadowColor 	= mFontShadowColor;
	Params.FontOverColor 	= mFontOverColor;
	Params.Flags			= UI_CLIP_ENABLE | UI_VALIGN_CENTER | UI_HALIGN_LEFT;

	cUIMenuItem * tCtrl 	= eeNew( cUIMenuItem, ( Params ) );

	tCtrl->Text( Text );

	if ( NULL != Icon )
		tCtrl->Icon( Icon );

	tCtrl->Visible( true );
	tCtrl->Enabled( true );

	return tCtrl;
}

Uint32 cUIMenu::Add( const std::wstring& Text, cShape * Icon ) {
	return Add( CreateMenuItem( Text, Icon ) );
}

cUIMenuCheckBox * cUIMenu::CreateMenuCheckBox( const std::wstring& Text ) {
	cUIMenuCheckBox::CreateParams Params;
	Params.Parent( this );
	Params.Font 			= mFont;
	Params.FontColor 		= mFontColor;
	Params.FontShadowColor 	= mFontShadowColor;
	Params.FontOverColor	= mFontOverColor;
	Params.Flags			= UI_CLIP_ENABLE | UI_VALIGN_CENTER | UI_HALIGN_LEFT;
	Params.Size				= eeSize( 0, mRowHeight );

	cUIMenuCheckBox * tCtrl 	= eeNew( cUIMenuCheckBox, ( Params ) );

	tCtrl->Text( Text );
	tCtrl->Visible( true );
	tCtrl->Enabled( true );

	return tCtrl;
}

Uint32 cUIMenu::AddCheckBox( const std::wstring& Text ) {
	return Add( CreateMenuCheckBox( Text ) );
}

bool cUIMenu::CheckControlSize( cUIControl * Control, const bool& Resize ) {
	if ( Control->IsType( UI_TYPE_MENUITEM ) ) {
		cUIMenuItem * tItem = reinterpret_cast<cUIMenuItem*> ( Control );

		if ( NULL != tItem->Icon() && tItem->IconHorizontalMargin() + tItem->Icon()->Size().Width() > (Int32)mBiggestIcon ) {
			mBiggestIcon = tItem->IconHorizontalMargin() + tItem->Icon()->Size().Width();
		}

		if ( mFlags & UI_AUTO_SIZE ) {
			if ( tItem->TextBox()->GetTextWidth() + mBiggestIcon > (Int32)mMaxWidth - mPadding.Left - mPadding.Right ) {
				mMaxWidth = tItem->TextBox()->GetTextWidth() + mBiggestIcon + mPadding.Left + mPadding.Right;

				if ( Resize ) {
					ResizeControls();

					return true;
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

		cUIMenuItem * tItem = reinterpret_cast<cUIMenuItem*> (Control);

		if ( NULL == tItem->Icon()->Shape() ) {
			tItem->Padding( eeRecti( mBiggestIcon, 0, 0, 0 ) );
		} else {
			tItem->Padding( eeRecti( 0, 0, 0, 0 ) );
		}
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

cUIControl * cUIMenu::GetItem( const std::wstring& Text ) {
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		if ( mItems[i]->IsType( UI_TYPE_MENUITEM ) ) {
			cUIMenuItem * tMenuItem = reinterpret_cast<cUIMenuItem*>( mItems[i] );
			
			if ( tMenuItem->Text() == Text )
				return tMenuItem;
		}
	}
	
	return NULL;
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

void cUIMenu::Insert( const std::wstring& Text, cShape * Icon, const Uint32& Index ) {
	Insert( CreateMenuItem( Text, Icon ), Index );
}

void cUIMenu::Insert( cUIControl * Control, const Uint32& Index ) {
	mItems.insert( mItems.begin() + Index, Control );

	ChildAddAt( Control, Index );

	ReposControls();
	ResizeControls();
}

Uint32 cUIMenu::OnMessage( const cUIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case cUIMessage::MsgClick:
		{
			cUIEvent ItemEvent( Msg->Sender(), cUIEvent::EventOnItemClicked );
			SendEvent( &ItemEvent );
			
			return 1;
		}
		case cUIMessage::MsgFocusLoss:
		{
			cUIControl * FocusCtrl = cUIManager::instance()->FocusControl();

			if ( this != FocusCtrl && !IsParentOf( FocusCtrl ) ) {
				SendCommonEvent( cUIEvent::EventOnComplexControlFocusLoss );
				OnComplexControlFocusLoss();
			}

			return 1;
		}
	}

	return 0;
}

void cUIMenu::OnSizeChange() {
	if ( NULL != mFont && ( ( mFlags & UI_AUTO_SIZE ) || 0 == mRowHeight ) ) {
		mRowHeight = mFont->GetFontSize() * 1.5f;
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
	for ( Uint32 i = 0; i < mItems.size(); i++ ) {
		SetControlSize( mItems[i], i );
	}

	ResizeMe();
}

void cUIMenu::ReposControls() {
	Uint32 i;
	mNextPosY = 0;
	mBiggestIcon = 0;

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

}}
