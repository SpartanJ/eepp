#include "cuiwindow.hpp"
#include "cuimanager.hpp"

namespace EE { namespace UI {

cUIWindow::cUIWindow( const cUIWindow::CreateParams& Params ) :
	cUIComplexControl( Params ),
	mWinFlags( Params.WinFlags ),
	mButtonClose( NULL ),
	mButtonMinimize( NULL ),
	mButtonMaximize( NULL ),
	mDecoSize( Params.DecorationSize ),
	mBorderSize( Params.BorderSize ),
	mMinWindowSize( Params.MinWindowSize ),
	mButtonsPositionFixer( Params.ButtonsPositionFixer ),
	mButtonsSeparation( Params.ButtonsSeparation ),
	mDecoAutoSize( Params.DecorationAutoSize ),
	mBorderAutoSize( Params.BorderAutoSize )
{
	mType |= UI_TYPE_GET( UI_TYPE_WINDOW );

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

	mContainer		= eeNew( cUIControlAnim, ( tParams ) );
	mContainer->Enabled( true );
	mContainer->Visible( true );
	mContainer->AddEventListener( cUIEvent::EventOnPosChange, cb::Make1( this, &cUIWindow::ContainerPosChange ) );

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

	ApplyDefaultTheme();
}

cUIWindow::~cUIWindow() {
}

void cUIWindow::ContainerPosChange( const cUIEvent * Event ) {
	eeVector2i PosDiff = mContainer->Pos() - eeVector2i( mBorderLeft->Size().Width(), mWindowDecoration->Size().Height() );

	if ( PosDiff.x != 0 || PosDiff.y != 0 ) {
		mContainer->Pos( mBorderLeft->Size().Width(), mWindowDecoration->Size().Height() );

		Pos( mPos + PosDiff );
	}
}

void cUIWindow::ButtonCloseClick( const cUIEvent * Event ) {
	CloseFadeOut( cUIThemeManager::instance()->ControlsFadeOutTime() );
}

void cUIWindow::ButtonMaximizeClick( const cUIEvent * Event ) {
	cUIControl * Ctrl = cUIManager::instance()->MainControl();

	if ( Ctrl->Size() == cUIControl::Size() ) {
		Pos( mNonMaxPos );
		Size( mNonMaxSize );
	} else {
		mNonMaxPos	= mPos;
		mNonMaxSize = mSize;

		Pos( 0, 0 );
		Size( cUIManager::instance()->MainControl()->Size() );
	}
}

void cUIWindow::ButtonMinimizeClick( const cUIEvent * Event ) {
	Hide();
}

void cUIWindow::SetTheme( cUITheme *Theme ) {
	cUIComplexControl::SetTheme( Theme );

	mContainer->ForceThemeSkin			( Theme, "winback"			);
	mWindowDecoration->ForceThemeSkin	( Theme, "windeco"			);
	mBorderLeft->ForceThemeSkin			( Theme, "winborderleft"	);
	mBorderRight->ForceThemeSkin		( Theme, "winborderright"	);
	mBorderBottom->ForceThemeSkin		( Theme, "winborderbottom"	);

	if ( NULL != mButtonClose ) {
		mButtonClose->ForceThemeSkin( Theme, "winclose" );
		mButtonClose->Size( mButtonClose->GetSkinShapeSize() );
	}

	if ( NULL != mButtonMaximize ) {
		mButtonMaximize->ForceThemeSkin( Theme, "winmax" );
		mButtonMaximize->Size( mButtonMaximize->GetSkinShapeSize() );
	}

	if ( NULL != mButtonMinimize ) {
		mButtonMinimize->ForceThemeSkin( Theme, "winmin" );
		mButtonMinimize->Size( mButtonMinimize->GetSkinShapeSize() );
	}

	FixChildsSize();
}

void cUIWindow::OnSizeChange() {
	if ( ( 0 != mMinWindowSize.x && 0 != mMinWindowSize.y ) && ( mSize.x < mMinWindowSize.x || mSize.y < mMinWindowSize.y ) ) {
		Size( mMinWindowSize );
	} else {
		FixChildsSize();

		cUIComplexControl::OnSizeChange();
	}
}

void cUIWindow::Size( const eeSize& Size ) {
	eeSize size = Size;

	size.x += mBorderLeft->Size().Width() + mBorderRight->Size().Width();
	size.y += mWindowDecoration->Size().Height() + mBorderBottom->Size().Height();

	cUIControl::Size( size );
}

void cUIWindow::FixChildsSize() {
	if ( mDecoAutoSize ) {
		mDecoSize = eeSize( mSize.Width(), mWindowDecoration->GetSkinShapeSize().Height() );
	}

	mWindowDecoration->Size( mDecoSize );

	if ( mBorderAutoSize ) {
		mBorderBottom->Size( mSize.Width(), mBorderBottom->GetSkinShapeSize().Height() );
	} else {
		mBorderBottom->Size( mSize.Width(), mBorderSize.Height() );
	}

	Uint32 BorderHeight = mSize.Height() - mDecoSize.Height() - mBorderBottom->Size().Height();

	if ( mBorderAutoSize ) {
		mBorderLeft->Size( mBorderLeft->GetSkinShapeSize().Width()	, BorderHeight );
		mBorderRight->Size( mBorderRight->GetSkinShapeSize().Width(), BorderHeight );
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
}

Uint32 cUIWindow::OnMessage( const cUIMessage * Msg ) {
	switch ( Msg->Msg() ) {
		case cUIMessage::MsgFocus:
		{
			ToFront();
			break;
		}
	}

	return cUIComplexControl::OnMessage( Msg );
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

		if ( 255.f == Alpha() ) {
			StartAlphaAnim( 0.f, 255.f, cUIThemeManager::instance()->ControlsFadeInTime() );
		} else {
			CreateFadeIn( cUIThemeManager::instance()->ControlsFadeInTime() );
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

		return true;
	}

	return false;
}

}}
