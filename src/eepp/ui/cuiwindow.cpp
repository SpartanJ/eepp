#include <eepp/ui/cuiwindow.hpp>
#include <eepp/ui/cuimanager.hpp>
#include <eepp/graphics/cprimitives.hpp>

namespace EE { namespace UI {

cUIWindow::cUIWindow( const cUIWindow::CreateParams& Params ) :
	cUIComplexControl( Params ),
	mWinFlags( Params.WinFlags ),
	mWindowDecoration( NULL ),
	mButtonClose( NULL ),
	mButtonMinimize( NULL ),
	mButtonMaximize( NULL ),
	mTitle( NULL ),
	mModalCtrl( NULL ),
	mDecoSize( Params.DecorationSize ),
	mBorderSize( Params.BorderSize ),
	mMinWindowSize( Params.MinWindowSize ),
	mButtonsPositionFixer( Params.ButtonsPositionFixer ),
	mButtonsSeparation( Params.ButtonsSeparation ),
	mMinCornerDistance( Params.MinCornerDistance ),
	mResizeType( RESIZE_NONE ),
	mTitleFontColor( Params.TitleFontColor ),
	mBaseAlpha( Params.BaseAlpha ),
	mDecoAutoSize( Params.DecorationAutoSize ),
	mBorderAutoSize( Params.BorderAutoSize )
{
	cUIManager::instance()->WindowAdd( this );

	cUIComplexControl::CreateParams tcParams;
	tcParams.Parent( this );
	tcParams.Flags |= UI_REPORT_SIZE_CHANGE_TO_CHILDS;

	mContainer		= eeNew( cUIComplexControl, ( tcParams ) );
	mContainer->Enabled( true );
	mContainer->Visible( true );
	mContainer->Size( mSize );
	mContainer->AddEventListener( cUIEvent::EventOnPosChange, cb::Make1( this, &cUIWindow::ContainerPosChange ) );

	if ( !( mWinFlags & UI_WIN_NO_BORDER ) ) {
		cUIControlAnim::CreateParams tParams;
		tParams.Parent( this );

		mWindowDecoration = eeNew( cUIControlAnim, ( tParams ) );
		mWindowDecoration->Visible( true );
		mWindowDecoration->Enabled( false );

		mBorderLeft		= eeNew( cUIControlAnim, ( tParams ) );
		mBorderLeft->Enabled( true );
		mBorderLeft->Visible( true );

		mBorderRight	= eeNew( cUIControlAnim, ( tParams ) );
		mBorderRight->Enabled( true );
		mBorderRight->Visible( true );

		mBorderBottom	= eeNew( cUIControlAnim, ( tParams ) );
		mBorderBottom->Enabled( true );
		mBorderBottom->Visible( true );

		if ( mWinFlags & UI_WIN_DRAGABLE_CONTAINER )
			mContainer->DragEnable( true );

		cUIComplexControl::CreateParams ButtonParams;
		ButtonParams.Parent( this );

		if ( mWinFlags & UI_WIN_CLOSE_BUTTON ) {
			mButtonClose = eeNew( cUIComplexControl, ( ButtonParams ) );
			mButtonClose->Visible( true );
			mButtonClose->Enabled( true );

			if ( mWinFlags & UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS ) {
				mButtonClose->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cUIWindow::ButtonCloseClick ) );
			}
		}

		if ( ( mWinFlags & UI_WIN_RESIZEABLE ) && ( mWinFlags & UI_WIN_MAXIMIZE_BUTTON ) ) {
			mButtonMaximize = eeNew( cUIComplexControl, ( ButtonParams ) );
			mButtonMaximize->Visible( true );
			mButtonMaximize->Enabled( true );

			if ( mWinFlags & UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS ) {
				mButtonMaximize->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cUIWindow::ButtonMaximizeClick ) );
			}
		}

		if ( mWinFlags & UI_WIN_MINIMIZE_BUTTON ) {
			mButtonMinimize = eeNew( cUIComplexControl, ( ButtonParams ) );
			mButtonMinimize->Visible( true );
			mButtonMinimize->Enabled( true );

			if ( mWinFlags & UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS ) {
				mButtonMinimize->AddEventListener( cUIEvent::EventMouseClick, cb::Make1( this, &cUIWindow::ButtonMinimizeClick ) );
			}
		}

		DragEnable( true );
	}

	if ( IsModal() ) {
		CreateModalControl();
	}

	Alpha( mBaseAlpha );

	ApplyDefaultTheme();
}

cUIWindow::~cUIWindow() {
	cUIManager::instance()->WindowRemove( this );

	cUIManager::instance()->SetFocusLastWindow( this );

	SendCommonEvent( cUIEvent::EventOnWindowClose );
}

void cUIWindow::CreateModalControl() {
	if ( NULL == mModalCtrl ) {
		cUIControl * Ctrl = cUIManager::instance()->MainControl();
		mModalCtrl = eeNew( cUIControlAnim, ( cUIControlAnim::CreateParams( Ctrl , eeVector2i(0,0), Ctrl->Size(), UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM ) ) );
	}
}

Uint32 cUIWindow::Type() const {
	return UI_TYPE_WINDOW;
}

bool cUIWindow::IsType( const Uint32& type ) const {
	return cUIWindow::Type() == type ? true : cUIComplexControl::IsType( type );
}

void cUIWindow::ContainerPosChange( const cUIEvent * Event ) {
	eeVector2i PosDiff = mContainer->Pos() - eeVector2i( mBorderLeft->Size().Width(), mWindowDecoration->Size().Height() );

	if ( PosDiff.x != 0 || PosDiff.y != 0 ) {
		mContainer->Pos( mBorderLeft->Size().Width(), mWindowDecoration->Size().Height() );

		Pos( mPos + PosDiff );
	}
}

void cUIWindow::ButtonCloseClick( const cUIEvent * Event ) {
	CloseWindow();

	SendCommonEvent( cUIEvent::EventOnWindowCloseClick );
}

void cUIWindow::CloseWindow() {
	if ( NULL != mButtonClose )
		mButtonClose->Enabled( false );

	if ( NULL != mButtonMaximize )
		mButtonMaximize->Enabled( false );

	if ( NULL != mButtonMinimize )
		mButtonMinimize->Enabled( false );

	if ( NULL != mModalCtrl ) {
		mModalCtrl->Close();
		mModalCtrl = NULL;
	}

	if ( 0 != cUIThemeManager::instance()->ControlsFadeOutTime() )
		CloseFadeOut( cUIThemeManager::instance()->ControlsFadeOutTime() );
	else
		Close();
}

void cUIWindow::ButtonMaximizeClick( const cUIEvent * Event ) {
	Maximize();

	SendCommonEvent( cUIEvent::EventOnWindowMaximizeClick );
}

void cUIWindow::ButtonMinimizeClick( const cUIEvent * Event ) {
	Hide();

	SendCommonEvent( cUIEvent::EventOnWindowMinimizeClick );
}

void cUIWindow::SetTheme( cUITheme *Theme ) {
	cUIComplexControl::SetTheme( Theme );

	mContainer->SetThemeControl			( Theme, "winback"			);

	if ( !( mWinFlags & UI_WIN_NO_BORDER ) ) {
		mWindowDecoration->SetThemeControl	( Theme, "windeco"			);
		mBorderLeft->SetThemeControl		( Theme, "winborderleft"	);
		mBorderRight->SetThemeControl		( Theme, "winborderright"	);
		mBorderBottom->SetThemeControl		( Theme, "winborderbottom"	);

		if ( NULL != mButtonClose ) {
			mButtonClose->SetThemeControl( Theme, "winclose" );
			mButtonClose->Size( mButtonClose->GetSkinSize() );
		}

		if ( NULL != mButtonMaximize ) {
			mButtonMaximize->SetThemeControl( Theme, "winmax" );
			mButtonMaximize->Size( mButtonMaximize->GetSkinSize() );
		}

		if ( NULL != mButtonMinimize ) {
			mButtonMinimize->SetThemeControl( Theme, "winmin" );
			mButtonMinimize->Size( mButtonMinimize->GetSkinSize() );
		}

		FixChildsSize();
		GetMinWinSize();
	}
}

void cUIWindow::GetMinWinSize() {
	if ( NULL == mWindowDecoration || ( mMinWindowSize.x != 0 && mMinWindowSize.y != 0 ) )
		return;

	eeSize tSize;

	tSize.x = mBorderLeft->Size().Width() + mBorderRight->Size().Width() - mButtonsPositionFixer.x;
	tSize.y = mWindowDecoration->Size().Height() + mBorderBottom->Size().Height();

	if ( NULL != mButtonClose )
		tSize.x += mButtonClose->Size().Width();

	if ( NULL != mButtonMaximize )
		tSize.x += mButtonMaximize->Size().Width();

	if ( NULL != mButtonMinimize )
		tSize.x += mButtonMinimize->Size().Width();

	if ( mMinWindowSize.x < tSize.x )
		mMinWindowSize.x = tSize.x;

	if ( mMinWindowSize.y < tSize.y )
		mMinWindowSize.y = tSize.y;
}

void cUIWindow::OnSizeChange() {
	if ( mSize.x < mMinWindowSize.x || mSize.y < mMinWindowSize.y ) {
		if ( mSize.x < mMinWindowSize.x && mSize.y < mMinWindowSize.y ) {
			Size( mMinWindowSize );
		} else if ( mSize.x < mMinWindowSize.x ) {
			Size( eeSize( mMinWindowSize.x, mSize.y ) );
		} else {
			Size( eeSize( mSize.x, mMinWindowSize.y ) );
		}
	} else {
		FixChildsSize();

		cUIComplexControl::OnSizeChange();
	}
}

void cUIWindow::Size( const eeSize& Size ) {
	if ( NULL != mWindowDecoration ) {
		eeSize size = Size;

		size.x += mBorderLeft->Size().Width() + mBorderRight->Size().Width();
		size.y += mWindowDecoration->Size().Height() + mBorderBottom->Size().Height();

		cUIComplexControl::Size( size );
	} else {
		cUIComplexControl::Size( Size );
	}
}

void cUIWindow::Size( const Int32& Width, const Int32& Height ) {
	Size( eeSize( Width, Height ) );
}

const eeSize& cUIWindow::Size() {
	return cUIComplexControl::Size();
}

void cUIWindow::FixChildsSize() {
	if ( NULL == mWindowDecoration ) {
		mContainer->Size( mSize.Width(), mSize.Height() );
		return;
	}

	if ( mDecoAutoSize ) {
		mDecoSize = eeSize( mSize.Width(), mWindowDecoration->GetSkinSize().Height() );
	}

	mWindowDecoration->Size( mDecoSize );

	if ( mBorderAutoSize ) {
		mBorderBottom->Size( mSize.Width(), mBorderBottom->GetSkinSize().Height() );
	} else {
		mBorderBottom->Size( mSize.Width(), mBorderSize.Height() );
	}

	Uint32 BorderHeight = mSize.Height() - mDecoSize.Height() - mBorderBottom->Size().Height();

	if ( mBorderAutoSize ) {
		mBorderLeft->Size( mBorderLeft->GetSkinSize().Width()	, BorderHeight );
		mBorderRight->Size( mBorderRight->GetSkinSize().Width(), BorderHeight );
	} else {
		mBorderLeft->Size( mBorderSize.Width(), BorderHeight );
		mBorderRight->Size( mBorderSize.Width(), BorderHeight );
	}

	mBorderLeft->Pos( 0, mWindowDecoration->Size().Height() );
	mBorderRight->Pos( mSize.Width() - mBorderRight->Size().Width(), mWindowDecoration->Size().Height() );
	mBorderBottom->Pos( 0, mSize.Height() - mBorderBottom->Size().Height() );

	mContainer->Pos( mBorderLeft->Size().Width(), mWindowDecoration->Size().Height() );
	mContainer->Size( mSize.Width() - mBorderLeft->Size().Width() - mBorderRight->Size().Width(), mSize.Height() - mWindowDecoration->Size().Height() - mBorderBottom->Size().Height() );

	Uint32 yPos;

	if ( NULL != mButtonClose ) {
		yPos = mWindowDecoration->Size().Height() / 2 - mButtonClose->Size().Height() / 2 + mButtonsPositionFixer.y;

		mButtonClose->Pos( mWindowDecoration->Size().Width() - mBorderRight->Size().Width() - mButtonClose->Size().Width() + mButtonsPositionFixer.x, yPos );
	}

	if ( NULL != mButtonMaximize ) {
		yPos = mWindowDecoration->Size().Height() / 2 - mButtonMaximize->Size().Height() / 2 + mButtonsPositionFixer.y;

		if ( NULL != mButtonClose ) {
			mButtonMaximize->Pos( mButtonClose->Pos().x - mButtonsSeparation - mButtonMaximize->Size().Width(), yPos );
		} else {
			mButtonMaximize->Pos( mWindowDecoration->Size().Width() - mBorderRight->Size().Width() - mButtonMaximize->Size().Width() + mButtonsPositionFixer.x, yPos );
		}
	}

	if ( NULL != mButtonMinimize ) {
		yPos = mWindowDecoration->Size().Height() / 2 - mButtonMinimize->Size().Height() / 2 + mButtonsPositionFixer.y;

		if ( NULL != mButtonMaximize ) {
			mButtonMinimize->Pos( mButtonMaximize->Pos().x - mButtonsSeparation - mButtonMinimize->Size().Width(), yPos );
		} else {
			if ( NULL != mButtonClose ) {
				mButtonMinimize->Pos( mButtonClose->Pos().x - mButtonsSeparation - mButtonMinimize->Size().Width(), yPos );
			} else {
				mButtonMinimize->Pos( mWindowDecoration->Size().Width() - mBorderRight->Size().Width() - mButtonMinimize->Size().Width() + mButtonsPositionFixer.x, yPos );
			}
		}
	}

	FixTitleSize();
}

Uint32 cUIWindow::OnMessage( const cUIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case cUIMessage::MsgFocus:
		{
			ToFront();
			break;
		}
		case cUIMessage::MsgMouseDown:
		{
			DoResize( Msg );
			break;
		}
		case cUIMessage::MsgWindowResize:
		{
			if ( IsModal() && NULL != mModalCtrl ) {
				mModalCtrl->Size( cUIManager::instance()->MainControl()->Size() );
			}
		}
	}

	return cUIComplexControl::OnMessage( Msg );
}

void cUIWindow::DoResize ( const cUIMessage * Msg ) {
	if ( NULL == mWindowDecoration )
		return;

	if (	!( mWinFlags & UI_WIN_RESIZEABLE ) ||
			!( Msg->Flags() & EE_BUTTON_LMASK ) ||
			RESIZE_NONE != mResizeType ||
			( cUIManager::instance()->LastPressTrigger() & EE_BUTTON_LMASK )
	)
		return;

	DecideResizeType( Msg->Sender() );
}

void cUIWindow::DecideResizeType( cUIControl * Control ) {
	eeVector2i Pos = cUIManager::instance()->GetMousePos();

	ScreenToControl( Pos );

	if ( Control == this ) {
		if ( Pos.x <= mBorderLeft->Size().Width() ) {
			TryResize( RESIZE_TOPLEFT );
		} else if ( Pos.x >= ( mSize.Width() - mBorderRight->Size().Width() ) ) {
			TryResize( RESIZE_TOPRIGHT );
		} else if ( Pos.y <= mBorderBottom->Size().Height() ) {
			if ( Pos.x < mMinCornerDistance ) {
				TryResize( RESIZE_TOPLEFT );
			} else if ( Pos.x > mSize.Width() - mMinCornerDistance ) {
				TryResize( RESIZE_TOPRIGHT );
			} else {
				TryResize( RESIZE_TOP );
			}
		}
	} else if ( Control == mBorderBottom ) {
		if ( Pos.x < mMinCornerDistance ) {
			TryResize( RESIZE_LEFTBOTTOM );
		} else if ( Pos.x > mSize.Width() - mMinCornerDistance ) {
			TryResize( RESIZE_RIGHTBOTTOM );
		} else {
			TryResize( RESIZE_BOTTOM );
		}
	} else if ( Control == mBorderLeft )  {
		if ( Pos.y >= mSize.Height() - mMinCornerDistance ) {
			TryResize( RESIZE_LEFTBOTTOM );
		} else {
			TryResize( RESIZE_LEFT );
		}
	} else if ( Control == mBorderRight ) {
		if ( Pos.y >= mSize.Height() - mMinCornerDistance ) {
			TryResize( RESIZE_RIGHTBOTTOM );
		} else {
			TryResize( RESIZE_RIGHT );
		}
	}
}

void cUIWindow::TryResize( const UI_RESIZE_TYPE& Type ) {
	if ( RESIZE_NONE != mResizeType )
		return;

	DragEnable( false );

	eeVector2i Pos = cUIManager::instance()->GetMousePos();

	ScreenToControl( Pos );
	
	mResizeType = Type;
	
	switch ( mResizeType )
	{
		case RESIZE_RIGHT:
		{
			mResizePos.x = mSize.Width() - Pos.x;
			break;
		}
		case RESIZE_LEFT:
		{
			mResizePos.x = Pos.x;
			break;
		}
		case RESIZE_TOP:
		{
			mResizePos.y = Pos.y;
			break;
		}
		case RESIZE_BOTTOM:
		{
			mResizePos.y = mSize.Height() - Pos.y;
			break;
		}
		case RESIZE_RIGHTBOTTOM:
		{
			mResizePos.x = mSize.Width() - Pos.x;
			mResizePos.y = mSize.Height() - Pos.y;
			break;
		}
		case RESIZE_LEFTBOTTOM:
		{
			mResizePos.x = Pos.x;
			mResizePos.y = mSize.Height() - Pos.y;
			break;
		}
		case RESIZE_TOPLEFT:
		{
			mResizePos.x = Pos.x;
			mResizePos.y = Pos.y;
			break;
		}
		case RESIZE_TOPRIGHT:
		{
			mResizePos.y = Pos.y;
			mResizePos.x = mSize.Width() - Pos.x;
			break;
		}
		case RESIZE_NONE:
		{
		}
	}
}

void cUIWindow::EndResize() {
	mResizeType = RESIZE_NONE;
}

void cUIWindow::UpdateResize() {
	if ( RESIZE_NONE == mResizeType )
		return;

	if ( !( cUIManager::instance()->PressTrigger() & EE_BUTTON_LMASK ) ) {
		EndResize();
		DragEnable( true );
		return;
	}

	eeVector2i Pos = cUIManager::instance()->GetMousePos();

	ScreenToControl( Pos );

	switch ( mResizeType ) {
		case RESIZE_RIGHT:
		{
			InternalSize( Pos.x + mResizePos.x, mSize.Height() );
			break;
		}
		case RESIZE_BOTTOM:
		{
			InternalSize( mSize.Width(), Pos.y + mResizePos.y );
			break;
		}
		case RESIZE_LEFT:
		{
			Pos.x -= mResizePos.x;
			cUIControl::Pos( mPos.x + Pos.x, mPos.y );
			InternalSize( mSize.Width() - Pos.x, mSize.Height() );
			break;
		}
		case RESIZE_TOP:
		{
			Pos.y -= mResizePos.y;
			cUIControl::Pos( mPos.x, mPos.y + Pos.y );
			InternalSize( mSize.Width(), mSize.Height() - Pos.y );
			break;
		}
		case RESIZE_RIGHTBOTTOM:
		{
			Pos += mResizePos;
			InternalSize( Pos.x, Pos.y );
			break;
		}
		case RESIZE_TOPLEFT:
		{
			Pos -= mResizePos;
			cUIControl::Pos( mPos.x + Pos.x, mPos.y + Pos.y );
			InternalSize( mSize.Width() - Pos.x, mSize.Height() - Pos.y );
			break;
		}
		case RESIZE_TOPRIGHT:
		{
			Pos.y -= mResizePos.y;
			Pos.x += mResizePos.x;
			cUIControl::Pos( mPos.x, mPos.y + Pos.y );
			InternalSize( Pos.x, mSize.Height() - Pos.y );
			break;
		}
		case RESIZE_LEFTBOTTOM:
		{
			Pos.x -= mResizePos.x;
			Pos.y += mResizePos.y;
			cUIControl::Pos( mPos.x + Pos.x, mPos.y );
			InternalSize( mSize.Width() - Pos.x, Pos.y );
			break;
		}
		case RESIZE_NONE:
		{
		}
	}
}

void cUIWindow::InternalSize( const Int32& w, const Int32& h ) {
	InternalSize( eeSize( w, h ) );
}

void cUIWindow::InternalSize( eeSize Size ) {
	if ( Size.x < mMinWindowSize.x || Size.y < mMinWindowSize.y ) {
		if ( Size.x < mMinWindowSize.x && Size.y < mMinWindowSize.y ) {
			Size = mMinWindowSize;
		} else if ( Size.x < mMinWindowSize.x ) {
			Size.x = mMinWindowSize.x;
		} else {
			Size.y = mMinWindowSize.y;
		}
	}

	if ( Size != mSize ) {
		mSize = Size;
		OnSizeChange();
	}
}

void cUIWindow::Draw() {
	cUIComplexControl::Draw();

	if ( mWinFlags & UI_WIN_DRAW_SHADOW ) {
		cPrimitives P;
		P.ForceDraw( false );

		eeColorA BeginC( 0, 0, 0, 25 * ( Alpha() / (eeFloat)255 ) );
		eeColorA EndC( 0, 0, 0, 0 );
		eeFloat SSize = 16.f;

		eeVector2i ShadowPos = mScreenPos + eeVector2i( 0, 16 );

		P.DrawRectangle( eeRectf( eeVector2f( ShadowPos.x, ShadowPos.y ), eeSizef( mSize.Width(), mSize.Height() ) ), BeginC, BeginC, BeginC, BeginC );

		P.DrawRectangle( eeRectf( eeVector2f( ShadowPos.x, ShadowPos.y - SSize ), eeSizef( mSize.Width(), SSize ) ), EndC, BeginC, BeginC, EndC );

		P.DrawRectangle( eeRectf( eeVector2f( ShadowPos.x - SSize, ShadowPos.y ), eeSizef( SSize, mSize.Height() ) ), EndC, EndC, BeginC, BeginC );

		P.DrawRectangle( eeRectf( eeVector2f( ShadowPos.x + mSize.Width(), ShadowPos.y ), eeSizef( SSize, mSize.Height() ) ), BeginC, BeginC, EndC, EndC );

		P.DrawRectangle( eeRectf( eeVector2f( ShadowPos.x, ShadowPos.y + mSize.Height() ), eeSizef( mSize.Width(), SSize ) ), BeginC, EndC, EndC, BeginC );

		P.DrawTriangle( eeTriangle2f( eeVector2f( ShadowPos.x + mSize.Width(), ShadowPos.y ), eeVector2f( ShadowPos.x + mSize.Width(), ShadowPos.y - SSize ), eeVector2f( ShadowPos.x + mSize.Width() + SSize, ShadowPos.y ) ), BeginC, EndC, EndC );

		P.DrawTriangle( eeTriangle2f( eeVector2f( ShadowPos.x, ShadowPos.y ), eeVector2f( ShadowPos.x, ShadowPos.y - SSize ), eeVector2f( ShadowPos.x - SSize, ShadowPos.y ) ), BeginC, EndC, EndC );

		P.DrawTriangle( eeTriangle2f( eeVector2f( ShadowPos.x + mSize.Width(), ShadowPos.y + mSize.Height() ), eeVector2f( ShadowPos.x + mSize.Width(), ShadowPos.y + mSize.Height() + SSize ), eeVector2f( ShadowPos.x + mSize.Width() + SSize, ShadowPos.y + mSize.Height() ) ), BeginC, EndC, EndC );

		P.DrawTriangle( eeTriangle2f( eeVector2f( ShadowPos.x, ShadowPos.y + mSize.Height() ), eeVector2f( ShadowPos.x - SSize, ShadowPos.y + mSize.Height() ), eeVector2f( ShadowPos.x, ShadowPos.y + mSize.Height() + SSize ) ), BeginC, EndC, EndC );

		P.ForceDraw( true );
	}
}

void cUIWindow::Update() {
	cUIComplexControl::Update();

	UpdateResize();
}

cUIControlAnim * cUIWindow::Container() const {
	return mContainer;
}

cUIComplexControl * cUIWindow::ButtonClose() const {
	return mButtonClose;
}

cUIComplexControl * cUIWindow::ButtonMaximize() const {
	return mButtonMaximize;
}

cUIComplexControl * cUIWindow::ButtonMinimize() const {
	return mButtonMinimize;
}

bool cUIWindow::Show() {
	if ( !Visible() ) {
		Enabled( true );
		Visible( true );

		SetFocus();

		if ( mBaseAlpha == Alpha() ) {
			StartAlphaAnim( 0.f, mBaseAlpha, cUIThemeManager::instance()->ControlsFadeInTime() );
		} else {
			StartAlphaAnim( mAlpha, mBaseAlpha, cUIThemeManager::instance()->ControlsFadeInTime() );
		}

		if ( IsModal() ) {
			CreateModalControl();

			mModalCtrl->Enabled( true );
			mModalCtrl->Visible( true );
			mModalCtrl->ToFront();

			ToFront();
		}

		return true;
	}

	return false;
}

bool cUIWindow::Hide() {
	if ( Visible() ) {
		if ( cUIThemeManager::instance()->DefaultEffectsEnabled() ) {
			DisableFadeOut( cUIThemeManager::instance()->ControlsFadeOutTime() );
		} else {
			Enabled( false );
			Visible( false );
		}

		cUIManager::instance()->MainControl()->SetFocus();

		if ( NULL != mModalCtrl ) {
			mModalCtrl->Enabled( false );
			mModalCtrl->Visible( false );
		}

		return true;
	}

	return false;
}

void cUIWindow::OnAlphaChange() {
	if ( mWinFlags & UI_WIN_SHARE_ALPHA_WITH_CHILDS ) {
		cUIControlAnim * AnimChild;
		cUIControl * CurChild = mChild;

		while ( NULL != CurChild ) {
			if ( CurChild->IsAnimated() ) {
				AnimChild = reinterpret_cast<cUIControlAnim*> ( CurChild );
				AnimChild->Alpha( mAlpha );
			}

			CurChild = CurChild->NextGet();
		}
	}

	cUIComplexControl::OnAlphaChange();
}

void cUIWindow::BaseAlpha( const Uint8& Alpha ) {
	if ( mAlpha == mBaseAlpha ) {
		cUIControlAnim::Alpha( Alpha );
	}

	mBaseAlpha = Alpha;
}

const Uint8& cUIWindow::BaseAlpha() const {
	return mBaseAlpha;
}

void cUIWindow::Title( const String& Text ) {
	if ( NULL == mTitle ) {
		cUITextBox::CreateParams Params;
		Params.Parent( this );
		Params.Flags		= UI_CLIP_ENABLE | UI_VALIGN_CENTER;
		Params.FontColor	= mTitleFontColor;

		if ( mFlags & UI_HALIGN_CENTER )
			Params.Flags |= UI_HALIGN_CENTER;

		if ( mFlags & UI_DRAW_SHADOW )
			Params.Flags |= UI_DRAW_SHADOW;

		mTitle = eeNew( cUITextBox, ( Params ) );
		mTitle->Enabled( false );
		mTitle->Visible( true );
	}

	mTitle->Text( Text );

	FixTitleSize();
}

void cUIWindow::FixTitleSize() {
	if ( NULL != mWindowDecoration && NULL != mTitle ) {
		mTitle->Size( mWindowDecoration->Size().Width() - mBorderLeft->Size().Width() - mBorderRight->Size().Width(), mWindowDecoration->Size().Height() );
		mTitle->Pos( mBorderLeft->Size().Width(), 0 );
	}
}

String cUIWindow::Title() const {
	if ( NULL != mTitle )
		return mTitle->Text();

	return String();
}

cUITextBox * cUIWindow::TitleTextBox() const {
	return mTitle;
}

void cUIWindow::Maximize() {
	cUIControl * Ctrl = cUIManager::instance()->MainControl();

	if ( Ctrl->Size() == mSize ) {
		Pos( mNonMaxPos );
		InternalSize( mNonMaxSize );
	} else {
		mNonMaxPos	= mPos;
		mNonMaxSize = mSize;

		Pos( 0, 0 );
		InternalSize( cUIManager::instance()->MainControl()->Size() );
	}
}

Uint32 cUIWindow::OnMouseDoubleClick( const eeVector2i &Pos, const Uint32 Flags ) {
	if ( ( mWinFlags & UI_WIN_RESIZEABLE ) && ( NULL != mButtonMaximize ) && ( Flags & EE_BUTTON_LMASK ) ) {
		ButtonMaximizeClick( NULL );
	}

	return 1;
}

Uint32 cUIWindow::OnKeyDown( const cUIEventKey &Event ) {
	CheckShortcuts( Event.KeyCode(), Event.Mod() );

	return cUIComplexControl::OnKeyDown( Event );
}

void cUIWindow::CheckShortcuts( const Uint32& KeyCode, const Uint32& Mod ) {
	for ( KeyboardShortcuts::iterator it = mKbShortcuts.begin(); it != mKbShortcuts.end(); it++ ) {
		KeyboardShortcut kb = (*it);

		if ( KeyCode == kb.KeyCode && ( Mod & kb.Mod ) ) {
			cUIManager::instance()->SendMouseUp( kb.Button, eeVector2i(0,0), EE_BUTTON_LMASK );
			cUIManager::instance()->SendMouseClick( kb.Button, eeVector2i(0,0), EE_BUTTON_LMASK );
		}
	}
}

cUIWindow::KeyboardShortcuts::iterator cUIWindow::ExistsShortcut( const Uint32& KeyCode, const Uint32& Mod ) {
	for ( KeyboardShortcuts::iterator it = mKbShortcuts.begin(); it != mKbShortcuts.end(); it++ ) {
		if ( (*it).KeyCode == KeyCode && (*it).Mod == Mod )
			return it;
	}

	return mKbShortcuts.end();
}

bool cUIWindow::AddShortcut( const Uint32& KeyCode, const Uint32& Mod, cUIPushButton * Button ) {
	if ( InParentTreeOf( Button ) && mKbShortcuts.end() == ExistsShortcut( KeyCode, Mod ) ) {
		mKbShortcuts.push_back( KeyboardShortcut( KeyCode, Mod, Button ) );

		return true;
	}

	return false;
}

bool cUIWindow::RemoveShortcut( const Uint32& KeyCode, const Uint32& Mod ) {
	KeyboardShortcuts::iterator it = ExistsShortcut( KeyCode, Mod );

	if ( mKbShortcuts.end() != it ) {
		mKbShortcuts.erase( it );

		return true;
	}

	return false;
}

bool cUIWindow::IsMaximixable() {
	return 0 != ( ( mWinFlags & UI_WIN_RESIZEABLE ) && ( mWinFlags & UI_WIN_MAXIMIZE_BUTTON ) );
}

bool cUIWindow::IsModal() {
	return 0 != ( mWinFlags & UI_WIN_MODAL );
}

cUIControlAnim * cUIWindow::GetModalControl() const {
	return mModalCtrl;
}

}}
