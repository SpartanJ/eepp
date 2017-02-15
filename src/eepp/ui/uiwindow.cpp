#include <eepp/ui/uiwindow.hpp>
#include <eepp/ui/uimanager.hpp>
#include <eepp/graphics/primitives.hpp>

namespace EE { namespace UI {

UIWindow::UIWindow( const UIWindow::CreateParams& Params ) :
	UIComplexControl( Params ),
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
	UIManager::instance()->WindowAdd( this );

	UIComplexControl::CreateParams tcParams;
	tcParams.Parent( this );
	tcParams.Flags |= UI_REPORT_SIZE_CHANGE_TO_CHILDS;

	mContainer		= eeNew( UIComplexControl, ( tcParams ) );
	mContainer->Enabled( true );
	mContainer->Visible( true );
	mContainer->Size( mSize );
	mContainer->AddEventListener( UIEvent::EventOnPosChange, cb::Make1( this, &UIWindow::ContainerPosChange ) );

	if ( !( mWinFlags & UI_WIN_NO_BORDER ) ) {
		UIControlAnim::CreateParams tParams;
		tParams.Parent( this );

		mWindowDecoration = eeNew( UIControlAnim, ( tParams ) );
		mWindowDecoration->Visible( true );
		mWindowDecoration->Enabled( false );

		mBorderLeft		= eeNew( UIControlAnim, ( tParams ) );
		mBorderLeft->Enabled( true );
		mBorderLeft->Visible( true );

		mBorderRight	= eeNew( UIControlAnim, ( tParams ) );
		mBorderRight->Enabled( true );
		mBorderRight->Visible( true );

		mBorderBottom	= eeNew( UIControlAnim, ( tParams ) );
		mBorderBottom->Enabled( true );
		mBorderBottom->Visible( true );

		if ( mWinFlags & UI_WIN_DRAGABLE_CONTAINER )
			mContainer->DragEnable( true );

		UIComplexControl::CreateParams ButtonParams;
		ButtonParams.Parent( this );

		if ( mWinFlags & UI_WIN_CLOSE_BUTTON ) {
			mButtonClose = eeNew( UIComplexControl, ( ButtonParams ) );
			mButtonClose->Visible( true );
			mButtonClose->Enabled( true );

			if ( mWinFlags & UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS ) {
				mButtonClose->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIWindow::ButtonCloseClick ) );
			}
		}

		if ( ( mWinFlags & UI_WIN_RESIZEABLE ) && ( mWinFlags & UI_WIN_MAXIMIZE_BUTTON ) ) {
			mButtonMaximize = eeNew( UIComplexControl, ( ButtonParams ) );
			mButtonMaximize->Visible( true );
			mButtonMaximize->Enabled( true );

			if ( mWinFlags & UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS ) {
				mButtonMaximize->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIWindow::ButtonMaximizeClick ) );
			}
		}

		if ( mWinFlags & UI_WIN_MINIMIZE_BUTTON ) {
			mButtonMinimize = eeNew( UIComplexControl, ( ButtonParams ) );
			mButtonMinimize->Visible( true );
			mButtonMinimize->Enabled( true );

			if ( mWinFlags & UI_WIN_USE_DEFAULT_BUTTONS_ACTIONS ) {
				mButtonMinimize->AddEventListener( UIEvent::EventMouseClick, cb::Make1( this, &UIWindow::ButtonMinimizeClick ) );
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

UIWindow::~UIWindow() {
	UIManager::instance()->WindowRemove( this );

	UIManager::instance()->SetFocusLastWindow( this );

	SendCommonEvent( UIEvent::EventOnWindowClose );
}

void UIWindow::CreateModalControl() {
	UIControl * Ctrl = UIManager::instance()->MainControl();

	if ( NULL == mModalCtrl ) {
		mModalCtrl = eeNew( UIControlAnim, ( UIControlAnim::CreateParams( Ctrl , Vector2i(0,0), Ctrl->Size(), UI_ANCHOR_LEFT | UI_ANCHOR_TOP | UI_ANCHOR_RIGHT | UI_ANCHOR_BOTTOM ) ) );
	} else {
		mModalCtrl->Pos( 0, 0 );
		mModalCtrl->Size( Ctrl->Size() );
	}

	DisableByModal();
}

void UIWindow::EnableByModal() {
	if ( IsModal() ) {
		UIControl * CtrlChild = UIManager::instance()->MainControl()->ChildGetFirst();

		while ( NULL != CtrlChild )
		{
			if ( CtrlChild != mModalCtrl &&
				 CtrlChild != this &&
				 CtrlChild->ControlFlags() & UI_CTRL_FLAG_DISABLED_BY_MODAL_WINDOW )
			{
				CtrlChild->Enabled( true );
				CtrlChild->WriteCtrlFlag( UI_CTRL_FLAG_DISABLED_BY_MODAL_WINDOW, 0 );
			}

			CtrlChild = CtrlChild->NextGet();
		}
	}
}

void UIWindow::DisableByModal() {
	if ( IsModal() ) {
		UIControl * CtrlChild = UIManager::instance()->MainControl()->ChildGetFirst();

		while ( NULL != CtrlChild )
		{
			if ( CtrlChild != mModalCtrl &&
				 CtrlChild != this &&
				 CtrlChild->Enabled() )
			{
				CtrlChild->Enabled( false );
				CtrlChild->WriteCtrlFlag( UI_CTRL_FLAG_DISABLED_BY_MODAL_WINDOW, 1 );
			}

			CtrlChild = CtrlChild->NextGet();
		}
	}
}

Uint32 UIWindow::Type() const {
	return UI_TYPE_WINDOW;
}

bool UIWindow::IsType( const Uint32& type ) const {
	return UIWindow::Type() == type ? true : UIComplexControl::IsType( type );
}

void UIWindow::ContainerPosChange( const UIEvent * Event ) {
	Vector2i PosDiff = mContainer->Pos() - Vector2i( mBorderLeft->Size().width(), mWindowDecoration->Size().height() );

	if ( PosDiff.x != 0 || PosDiff.y != 0 ) {
		mContainer->Pos( mBorderLeft->Size().width(), mWindowDecoration->Size().height() );

		Pos( mPos + PosDiff );
	}
}

void UIWindow::ButtonCloseClick( const UIEvent * Event ) {
	CloseWindow();

	SendCommonEvent( UIEvent::EventOnWindowCloseClick );
}

void UIWindow::CloseWindow() {
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

	if ( Time::Zero != UIThemeManager::instance()->ControlsFadeOutTime() )
		CloseFadeOut( UIThemeManager::instance()->ControlsFadeOutTime() );
	else
		Close();
}

void UIWindow::Close() {
	UIComplexControl::Close();

	EnableByModal();
}

void UIWindow::ButtonMaximizeClick( const UIEvent * Event ) {
	Maximize();

	SendCommonEvent( UIEvent::EventOnWindowMaximizeClick );
}

void UIWindow::ButtonMinimizeClick( const UIEvent * Event ) {
	Hide();

	SendCommonEvent( UIEvent::EventOnWindowMinimizeClick );
}

void UIWindow::SetTheme( UITheme *Theme ) {
	UIComplexControl::SetTheme( Theme );

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

void UIWindow::GetMinWinSize() {
	if ( NULL == mWindowDecoration || ( mMinWindowSize.x != 0 && mMinWindowSize.y != 0 ) )
		return;

	Sizei tSize;

	tSize.x = mBorderLeft->Size().width() + mBorderRight->Size().width() - mButtonsPositionFixer.x;
	tSize.y = mWindowDecoration->Size().height() + mBorderBottom->Size().height();

	if ( NULL != mButtonClose )
		tSize.x += mButtonClose->Size().width();

	if ( NULL != mButtonMaximize )
		tSize.x += mButtonMaximize->Size().width();

	if ( NULL != mButtonMinimize )
		tSize.x += mButtonMinimize->Size().width();

	if ( mMinWindowSize.x < tSize.x )
		mMinWindowSize.x = tSize.x;

	if ( mMinWindowSize.y < tSize.y )
		mMinWindowSize.y = tSize.y;
}

void UIWindow::OnSizeChange() {
	if ( mSize.x < mMinWindowSize.x || mSize.y < mMinWindowSize.y ) {
		if ( mSize.x < mMinWindowSize.x && mSize.y < mMinWindowSize.y ) {
			Size( mMinWindowSize );
		} else if ( mSize.x < mMinWindowSize.x ) {
			Size( Sizei( mMinWindowSize.x, mSize.y ) );
		} else {
			Size( Sizei( mSize.x, mMinWindowSize.y ) );
		}
	} else {
		FixChildsSize();

		UIComplexControl::OnSizeChange();
	}
}

void UIWindow::Size( const Sizei& Size ) {
	if ( NULL != mWindowDecoration ) {
		Sizei size = Size;

		size.x += mBorderLeft->Size().width() + mBorderRight->Size().width();
		size.y += mWindowDecoration->Size().height() + mBorderBottom->Size().height();

		UIComplexControl::Size( size );
	} else {
		UIComplexControl::Size( Size );
	}
}

void UIWindow::Size( const Int32& Width, const Int32& Height ) {
	Size( Sizei( Width, Height ) );
}

const Sizei& UIWindow::Size() {
	return UIComplexControl::Size();
}

void UIWindow::FixChildsSize() {
	if ( NULL == mWindowDecoration ) {
		mContainer->Size( mSize.width(), mSize.height() );
		return;
	}

	if ( mDecoAutoSize ) {
		mDecoSize = Sizei( mSize.width(), mWindowDecoration->GetSkinSize().height() );
	}

	mWindowDecoration->Size( mDecoSize );

	if ( mBorderAutoSize ) {
		mBorderBottom->Size( mSize.width(), mBorderBottom->GetSkinSize().height() );
	} else {
		mBorderBottom->Size( mSize.width(), mBorderSize.height() );
	}

	Uint32 BorderHeight = mSize.height() - mDecoSize.height() - mBorderBottom->Size().height();

	if ( mBorderAutoSize ) {
		mBorderLeft->Size( mBorderLeft->GetSkinSize().width()	, BorderHeight );
		mBorderRight->Size( mBorderRight->GetSkinSize().width(), BorderHeight );
	} else {
		mBorderLeft->Size( mBorderSize.width(), BorderHeight );
		mBorderRight->Size( mBorderSize.width(), BorderHeight );
	}

	mBorderLeft->Pos( 0, mWindowDecoration->Size().height() );
	mBorderRight->Pos( mSize.width() - mBorderRight->Size().width(), mWindowDecoration->Size().height() );
	mBorderBottom->Pos( 0, mSize.height() - mBorderBottom->Size().height() );

	mContainer->Pos( mBorderLeft->Size().width(), mWindowDecoration->Size().height() );
	mContainer->Size( mSize.width() - mBorderLeft->Size().width() - mBorderRight->Size().width(), mSize.height() - mWindowDecoration->Size().height() - mBorderBottom->Size().height() );

	Uint32 yPos;

	if ( NULL != mButtonClose ) {
		yPos = mWindowDecoration->Size().height() / 2 - mButtonClose->Size().height() / 2 + mButtonsPositionFixer.y;

		mButtonClose->Pos( mWindowDecoration->Size().width() - mBorderRight->Size().width() - mButtonClose->Size().width() + mButtonsPositionFixer.x, yPos );
	}

	if ( NULL != mButtonMaximize ) {
		yPos = mWindowDecoration->Size().height() / 2 - mButtonMaximize->Size().height() / 2 + mButtonsPositionFixer.y;

		if ( NULL != mButtonClose ) {
			mButtonMaximize->Pos( mButtonClose->Pos().x - mButtonsSeparation - mButtonMaximize->Size().width(), yPos );
		} else {
			mButtonMaximize->Pos( mWindowDecoration->Size().width() - mBorderRight->Size().width() - mButtonMaximize->Size().width() + mButtonsPositionFixer.x, yPos );
		}
	}

	if ( NULL != mButtonMinimize ) {
		yPos = mWindowDecoration->Size().height() / 2 - mButtonMinimize->Size().height() / 2 + mButtonsPositionFixer.y;

		if ( NULL != mButtonMaximize ) {
			mButtonMinimize->Pos( mButtonMaximize->Pos().x - mButtonsSeparation - mButtonMinimize->Size().width(), yPos );
		} else {
			if ( NULL != mButtonClose ) {
				mButtonMinimize->Pos( mButtonClose->Pos().x - mButtonsSeparation - mButtonMinimize->Size().width(), yPos );
			} else {
				mButtonMinimize->Pos( mWindowDecoration->Size().width() - mBorderRight->Size().width() - mButtonMinimize->Size().width() + mButtonsPositionFixer.x, yPos );
			}
		}
	}

	FixTitleSize();
}

Uint32 UIWindow::OnMessage( const UIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case UIMessage::MsgFocus:
		{
			ToFront();
			break;
		}
		case UIMessage::MsgMouseDown:
		{
			DoResize( Msg );
			break;
		}
		case UIMessage::MsgWindowResize:
		{
			if ( IsModal() && NULL != mModalCtrl ) {
				mModalCtrl->Size( UIManager::instance()->MainControl()->Size() );
			}

			break;
		}
		case UIMessage::MsgMouseExit:
		{
			UIManager::instance()->SetCursor( EE_CURSOR_ARROW );
			break;
		}
		case UIMessage::MsgDragStart:
		{
			UIManager::instance()->SetCursor( EE_CURSOR_HAND );
			break;
		}
		case UIMessage::MsgDragEnd:
		{
			UIManager::instance()->SetCursor( EE_CURSOR_ARROW );
			break;
		}
	}

	return UIComplexControl::OnMessage( Msg );
}

void UIWindow::DoResize ( const UIMessage * Msg ) {
	if ( NULL == mWindowDecoration )
		return;

	if (	!( mWinFlags & UI_WIN_RESIZEABLE ) ||
			!( Msg->Flags() & EE_BUTTON_LMASK ) ||
			RESIZE_NONE != mResizeType ||
			( UIManager::instance()->LastPressTrigger() & EE_BUTTON_LMASK )
	)
		return;

	DecideResizeType( Msg->Sender() );
}

void UIWindow::DecideResizeType( UIControl * Control ) {
	Vector2i Pos = UIManager::instance()->GetMousePos();

	WorldToControl( Pos );

	if ( Control == this ) {
		if ( Pos.x <= mBorderLeft->Size().width() ) {
			TryResize( RESIZE_TOPLEFT );
		} else if ( Pos.x >= ( mSize.width() - mBorderRight->Size().width() ) ) {
			TryResize( RESIZE_TOPRIGHT );
		} else if ( Pos.y <= mBorderBottom->Size().height() ) {
			if ( Pos.x < mMinCornerDistance ) {
				TryResize( RESIZE_TOPLEFT );
			} else if ( Pos.x > mSize.width() - mMinCornerDistance ) {
				TryResize( RESIZE_TOPRIGHT );
			} else {
				TryResize( RESIZE_TOP );
			}
		}
	} else if ( Control == mBorderBottom ) {
		if ( Pos.x < mMinCornerDistance ) {
			TryResize( RESIZE_LEFTBOTTOM );
		} else if ( Pos.x > mSize.width() - mMinCornerDistance ) {
			TryResize( RESIZE_RIGHTBOTTOM );
		} else {
			TryResize( RESIZE_BOTTOM );
		}
	} else if ( Control == mBorderLeft )  {
		if ( Pos.y >= mSize.height() - mMinCornerDistance ) {
			TryResize( RESIZE_LEFTBOTTOM );
		} else {
			TryResize( RESIZE_LEFT );
		}
	} else if ( Control == mBorderRight ) {
		if ( Pos.y >= mSize.height() - mMinCornerDistance ) {
			TryResize( RESIZE_RIGHTBOTTOM );
		} else {
			TryResize( RESIZE_RIGHT );
		}
	}
}

void UIWindow::TryResize( const UI_RESIZE_TYPE& Type ) {
	if ( RESIZE_NONE != mResizeType )
		return;

	DragEnable( false );

	Vector2i Pos = UIManager::instance()->GetMousePos();

	WorldToControl( Pos );
	
	mResizeType = Type;
	
	switch ( mResizeType )
	{
		case RESIZE_RIGHT:
		{
			mResizePos.x = mSize.width() - Pos.x;
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
			mResizePos.y = mSize.height() - Pos.y;
			break;
		}
		case RESIZE_RIGHTBOTTOM:
		{
			mResizePos.x = mSize.width() - Pos.x;
			mResizePos.y = mSize.height() - Pos.y;
			break;
		}
		case RESIZE_LEFTBOTTOM:
		{
			mResizePos.x = Pos.x;
			mResizePos.y = mSize.height() - Pos.y;
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
			mResizePos.x = mSize.width() - Pos.x;
			break;
		}
		case RESIZE_NONE:
		{
		}
	}
}

void UIWindow::EndResize() {
	mResizeType = RESIZE_NONE;
}

void UIWindow::UpdateResize() {
	if ( RESIZE_NONE == mResizeType )
		return;

	if ( !( UIManager::instance()->PressTrigger() & EE_BUTTON_LMASK ) ) {
		EndResize();
		DragEnable( true );
		return;
	}

	Vector2i Pos = UIManager::instance()->GetMousePos();

	WorldToControl( Pos );

	switch ( mResizeType ) {
		case RESIZE_RIGHT:
		{
			InternalSize( Pos.x + mResizePos.x, mSize.height() );
			break;
		}
		case RESIZE_BOTTOM:
		{
			InternalSize( mSize.width(), Pos.y + mResizePos.y );
			break;
		}
		case RESIZE_LEFT:
		{
			Pos.x -= mResizePos.x;
			UIControl::Pos( mPos.x + Pos.x, mPos.y );
			InternalSize( mSize.width() - Pos.x, mSize.height() );
			break;
		}
		case RESIZE_TOP:
		{
			Pos.y -= mResizePos.y;
			UIControl::Pos( mPos.x, mPos.y + Pos.y );
			InternalSize( mSize.width(), mSize.height() - Pos.y );
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
			UIControl::Pos( mPos.x + Pos.x, mPos.y + Pos.y );
			InternalSize( mSize.width() - Pos.x, mSize.height() - Pos.y );
			break;
		}
		case RESIZE_TOPRIGHT:
		{
			Pos.y -= mResizePos.y;
			Pos.x += mResizePos.x;
			UIControl::Pos( mPos.x, mPos.y + Pos.y );
			InternalSize( Pos.x, mSize.height() - Pos.y );
			break;
		}
		case RESIZE_LEFTBOTTOM:
		{
			Pos.x -= mResizePos.x;
			Pos.y += mResizePos.y;
			UIControl::Pos( mPos.x + Pos.x, mPos.y );
			InternalSize( mSize.width() - Pos.x, Pos.y );
			break;
		}
		case RESIZE_NONE:
		{
		}
	}
}

void UIWindow::InternalSize( const Int32& w, const Int32& h ) {
	InternalSize( Sizei( w, h ) );
}

void UIWindow::InternalSize( Sizei Size ) {
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

void UIWindow::Draw() {
	UIComplexControl::Draw();

	if ( mWinFlags & UI_WIN_DRAW_SHADOW ) {
		Primitives P;
		P.ForceDraw( false );

		ColorA BeginC( 0, 0, 0, 25 * ( Alpha() / (Float)255 ) );
		ColorA EndC( 0, 0, 0, 0 );
		Float SSize = 16.f;

		Vector2i ShadowPos = mScreenPos + Vector2i( 0, 16 );

		P.DrawRectangle( Rectf( Vector2f( ShadowPos.x, ShadowPos.y ), Sizef( mSize.width(), mSize.height() ) ), BeginC, BeginC, BeginC, BeginC );

		P.DrawRectangle( Rectf( Vector2f( ShadowPos.x, ShadowPos.y - SSize ), Sizef( mSize.width(), SSize ) ), EndC, BeginC, BeginC, EndC );

		P.DrawRectangle( Rectf( Vector2f( ShadowPos.x - SSize, ShadowPos.y ), Sizef( SSize, mSize.height() ) ), EndC, EndC, BeginC, BeginC );

		P.DrawRectangle( Rectf( Vector2f( ShadowPos.x + mSize.width(), ShadowPos.y ), Sizef( SSize, mSize.height() ) ), BeginC, BeginC, EndC, EndC );

		P.DrawRectangle( Rectf( Vector2f( ShadowPos.x, ShadowPos.y + mSize.height() ), Sizef( mSize.width(), SSize ) ), BeginC, EndC, EndC, BeginC );

		P.DrawTriangle( Triangle2f( Vector2f( ShadowPos.x + mSize.width(), ShadowPos.y ), Vector2f( ShadowPos.x + mSize.width(), ShadowPos.y - SSize ), Vector2f( ShadowPos.x + mSize.width() + SSize, ShadowPos.y ) ), BeginC, EndC, EndC );

		P.DrawTriangle( Triangle2f( Vector2f( ShadowPos.x, ShadowPos.y ), Vector2f( ShadowPos.x, ShadowPos.y - SSize ), Vector2f( ShadowPos.x - SSize, ShadowPos.y ) ), BeginC, EndC, EndC );

		P.DrawTriangle( Triangle2f( Vector2f( ShadowPos.x + mSize.width(), ShadowPos.y + mSize.height() ), Vector2f( ShadowPos.x + mSize.width(), ShadowPos.y + mSize.height() + SSize ), Vector2f( ShadowPos.x + mSize.width() + SSize, ShadowPos.y + mSize.height() ) ), BeginC, EndC, EndC );

		P.DrawTriangle( Triangle2f( Vector2f( ShadowPos.x, ShadowPos.y + mSize.height() ), Vector2f( ShadowPos.x - SSize, ShadowPos.y + mSize.height() ), Vector2f( ShadowPos.x, ShadowPos.y + mSize.height() + SSize ) ), BeginC, EndC, EndC );

		P.ForceDraw( true );
	}
}

void UIWindow::Update() {
	ResizeCursor();

	UIComplexControl::Update();

	UpdateResize();
}

UIControlAnim * UIWindow::Container() const {
	return mContainer;
}

UIComplexControl * UIWindow::ButtonClose() const {
	return mButtonClose;
}

UIComplexControl * UIWindow::ButtonMaximize() const {
	return mButtonMaximize;
}

UIComplexControl * UIWindow::ButtonMinimize() const {
	return mButtonMinimize;
}

bool UIWindow::Show() {
	if ( !Visible() ) {
		Enabled( true );
		Visible( true );

		SetFocus();

		StartAlphaAnim( mBaseAlpha == Alpha() ? 0.f : mAlpha, mBaseAlpha, UIThemeManager::instance()->ControlsFadeInTime() );

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

bool UIWindow::Hide() {
	if ( Visible() ) {
		if ( UIThemeManager::instance()->DefaultEffectsEnabled() ) {
			DisableFadeOut( UIThemeManager::instance()->ControlsFadeOutTime() );
		} else {
			Enabled( false );
			Visible( false );
		}

		UIManager::instance()->MainControl()->SetFocus();

		if ( NULL != mModalCtrl ) {
			mModalCtrl->Enabled( false );
			mModalCtrl->Visible( false );
		}

		return true;
	}

	return false;
}

void UIWindow::OnAlphaChange() {
	if ( mWinFlags & UI_WIN_SHARE_ALPHA_WITH_CHILDS ) {
		UIControlAnim * AnimChild;
		UIControl * CurChild = mChild;

		while ( NULL != CurChild ) {
			if ( CurChild->IsAnimated() ) {
				AnimChild = reinterpret_cast<UIControlAnim*> ( CurChild );
				AnimChild->Alpha( mAlpha );
			}

			CurChild = CurChild->NextGet();
		}
	}

	UIComplexControl::OnAlphaChange();
}

void UIWindow::BaseAlpha( const Uint8& Alpha ) {
	if ( mAlpha == mBaseAlpha ) {
		UIControlAnim::Alpha( Alpha );
	}

	mBaseAlpha = Alpha;
}

const Uint8& UIWindow::BaseAlpha() const {
	return mBaseAlpha;
}

void UIWindow::Title( const String& Text ) {
	if ( NULL == mTitle ) {
		UITextBox::CreateParams Params;
		Params.Parent( this );
		Params.Flags		= UI_CLIP_ENABLE | UI_VALIGN_CENTER;
		Params.FontColor	= mTitleFontColor;

		if ( mFlags & UI_HALIGN_CENTER )
			Params.Flags |= UI_HALIGN_CENTER;

		if ( mFlags & UI_DRAW_SHADOW )
			Params.Flags |= UI_DRAW_SHADOW;

		mTitle = eeNew( UITextBox, ( Params ) );
		mTitle->Enabled( false );
		mTitle->Visible( true );
	}

	mTitle->Text( Text );

	FixTitleSize();
}

void UIWindow::FixTitleSize() {
	if ( NULL != mWindowDecoration && NULL != mTitle ) {
		mTitle->Size( mWindowDecoration->Size().width() - mBorderLeft->Size().width() - mBorderRight->Size().width(), mWindowDecoration->Size().height() );
		mTitle->Pos( mBorderLeft->Size().width(), 0 );
	}
}

String UIWindow::Title() const {
	if ( NULL != mTitle )
		return mTitle->Text();

	return String();
}

UITextBox * UIWindow::TitleTextBox() const {
	return mTitle;
}

void UIWindow::Maximize() {
	UIControl * Ctrl = UIManager::instance()->MainControl();

	if ( Ctrl->Size() == mSize ) {
		Pos( mNonMaxPos );
		InternalSize( mNonMaxSize );
	} else {
		mNonMaxPos	= mPos;
		mNonMaxSize = mSize;

		Pos( 0, 0 );
		InternalSize( UIManager::instance()->MainControl()->Size() );
	}
}

Uint32 UIWindow::OnMouseDoubleClick( const Vector2i &Pos, const Uint32 Flags ) {
	if ( ( mWinFlags & UI_WIN_RESIZEABLE ) && ( NULL != mButtonMaximize ) && ( Flags & EE_BUTTON_LMASK ) ) {
		ButtonMaximizeClick( NULL );
	}

	return 1;
}

Uint32 UIWindow::OnKeyDown( const UIEventKey &Event ) {
	CheckShortcuts( Event.KeyCode(), Event.Mod() );

	return UIComplexControl::OnKeyDown( Event );
}

void UIWindow::CheckShortcuts( const Uint32& KeyCode, const Uint32& Mod ) {
	for ( KeyboardShortcuts::iterator it = mKbShortcuts.begin(); it != mKbShortcuts.end(); it++ ) {
		KeyboardShortcut kb = (*it);

		if ( KeyCode == kb.KeyCode && ( Mod & kb.Mod ) ) {
			UIManager::instance()->SendMouseUp( kb.Button, Vector2i(0,0), EE_BUTTON_LMASK );
			UIManager::instance()->SendMouseClick( kb.Button, Vector2i(0,0), EE_BUTTON_LMASK );
		}
	}
}

UIWindow::KeyboardShortcuts::iterator UIWindow::ExistsShortcut( const Uint32& KeyCode, const Uint32& Mod ) {
	for ( KeyboardShortcuts::iterator it = mKbShortcuts.begin(); it != mKbShortcuts.end(); it++ ) {
		if ( (*it).KeyCode == KeyCode && (*it).Mod == Mod )
			return it;
	}

	return mKbShortcuts.end();
}

bool UIWindow::AddShortcut( const Uint32& KeyCode, const Uint32& Mod, UIPushButton * Button ) {
	if ( InParentTreeOf( Button ) && mKbShortcuts.end() == ExistsShortcut( KeyCode, Mod ) ) {
		mKbShortcuts.push_back( KeyboardShortcut( KeyCode, Mod, Button ) );

		return true;
	}

	return false;
}

bool UIWindow::RemoveShortcut( const Uint32& KeyCode, const Uint32& Mod ) {
	KeyboardShortcuts::iterator it = ExistsShortcut( KeyCode, Mod );

	if ( mKbShortcuts.end() != it ) {
		mKbShortcuts.erase( it );

		return true;
	}

	return false;
}

bool UIWindow::IsMaximixable() {
	return 0 != ( ( mWinFlags & UI_WIN_RESIZEABLE ) && ( mWinFlags & UI_WIN_MAXIMIZE_BUTTON ) );
}

bool UIWindow::IsModal() {
	return 0 != ( mWinFlags & UI_WIN_MODAL );
}

UIControlAnim * UIWindow::GetModalControl() const {
	return mModalCtrl;
}

void UIWindow::ResizeCursor() {
	UIManager * Man = UIManager::instance();

	if ( !IsMouseOverMeOrChilds() || !Man->UseGlobalCursors() || ( mWinFlags & UI_WIN_NO_BORDER ) || !( mWinFlags & UI_WIN_RESIZEABLE ) )
		return;

	Vector2i Pos = Man->GetMousePos();

	WorldToControl( Pos );

	const UIControl * Control = Man->OverControl();

	if ( Control == this ) {
		if ( Pos.x <= mBorderLeft->Size().width() ) {
			Man->SetCursor( EE_CURSOR_SIZENWSE ); // RESIZE_TOPLEFT
		} else if ( Pos.x >= ( mSize.width() - mBorderRight->Size().width() ) ) {
			Man->SetCursor( EE_CURSOR_SIZENESW ); // RESIZE_TOPRIGHT
		} else if ( Pos.y <= mBorderBottom->Size().height() ) {
			if ( Pos.x < mMinCornerDistance ) {
				Man->SetCursor( EE_CURSOR_SIZENWSE ); // RESIZE_TOPLEFT
			} else if ( Pos.x > mSize.width() - mMinCornerDistance ) {
				Man->SetCursor( EE_CURSOR_SIZENESW ); // RESIZE_TOPRIGHT
			} else {
				Man->SetCursor( EE_CURSOR_SIZENS ); // RESIZE_TOP
			}
		} else if ( !( UIManager::instance()->PressTrigger() & EE_BUTTON_LMASK ) ) {
			Man->SetCursor( EE_CURSOR_ARROW );
		}
	} else if ( Control == mBorderBottom ) {
		if ( Pos.x < mMinCornerDistance ) {
			Man->SetCursor( EE_CURSOR_SIZENESW ); // RESIZE_LEFTBOTTOM
		} else if ( Pos.x > mSize.width() - mMinCornerDistance ) {
			Man->SetCursor( EE_CURSOR_SIZENWSE ); // RESIZE_RIGHTBOTTOM
		} else {
			Man->SetCursor( EE_CURSOR_SIZENS ); // RESIZE_BOTTOM
		}
	} else if ( Control == mBorderLeft )  {
		if ( Pos.y >= mSize.height() - mMinCornerDistance ) {
			Man->SetCursor( EE_CURSOR_SIZENESW ); // RESIZE_LEFTBOTTOM
		} else {
			Man->SetCursor( EE_CURSOR_SIZEWE ); // RESIZE_LEFT
		}
	} else if ( Control == mBorderRight ) {
		if ( Pos.y >= mSize.height() - mMinCornerDistance ) {
			Man->SetCursor( EE_CURSOR_SIZENWSE ); // RESIZE_RIGHTBOTTOM
		} else {
			Man->SetCursor( EE_CURSOR_SIZEWE ); // RESIZE_RIGHT
		}
	}
}

}}
