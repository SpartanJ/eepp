#include "cuimanager.hpp"
#include "../window/cengine.hpp"

namespace EE { namespace UI {

cUIManager::cUIManager() :
	mWindow( NULL ),
	mKM( NULL ),
	mControl( NULL ),
	mFocusControl( NULL ),
	mOverControl( NULL ),
	mDownControl( NULL ),
	mCbId(-1),
	mResizeCb(0),
	mFlags( 0 ),
	mHighlightColor( 234, 195, 123, 255 ),
	mInit( false ),
	mFirstPress( false )
{
}

cUIManager::~cUIManager() {
	Shutdown();
}

void cUIManager::Init( Uint32 Flags, cWindow * window ) {
	if ( mInit )
		Shutdown();

	mWindow		= window;
	mFlags		= Flags;

	if ( NULL == mWindow ) {
		mWindow = cEngine::instance()->GetCurrentWindow();
	}

	mKM				= mWindow->GetInput();

	mInit			= true;

	cUIWindow::CreateParams Params;
	Params.Parent( NULL );
	Params.PosSet( 0, 0 );
	Params.SizeSet( cEngine::instance()->GetWidth(), cEngine::instance()->GetHeight() );
	Params.Flags = UI_CONTROL_DEFAULT_FLAGS | UI_REPORT_SIZE_CHANGE_TO_CHILDS;
	Params.WinFlags = UI_WIN_NO_BORDER | UI_WIN_RESIZEABLE;
	Params.MinWindowSize = eeSize( 0, 0 );
	Params.DecorationSize = eeSize( 0, 0 );
	Params.DecorationAutoSize = false;

	mControl		= eeNew( cUIWindow, ( Params ) );
	mControl->Visible( true );
	mControl->Enabled( true );

	mFocusControl	= mControl;
	mOverControl	= mControl;

	mCbId = mKM->PushCallback( cb::Make1( this, &cUIManager::InputCallback ) );
	mResizeCb = mWindow->PushResizeCallback( cb::Make0( this, &cUIManager::ResizeControl ) );
}

void cUIManager::Shutdown() {
	if ( mInit ) {
		if ( -1 != mCbId &&
			NULL != cEngine::ExistsSingleton() &&
			cEngine::instance()->ExistsWindow( mWindow )
		)
		{
			mKM->PopCallback( mCbId );
			mWindow->PopResizeCallback( mResizeCb );
		}

		mOverControl = NULL;
		mFocusControl = NULL;

		eeSAFE_DELETE( mControl );

		mInit = false;
	}

	cUIThemeManager::DestroySingleton();
}

void cUIManager::InputCallback( InputEvent * Event ) {
	switch( Event->Type ) {
		case InputEvent::KeyUp:
			SendKeyUp( Event->key.keysym.sym, Event->key.keysym.unicode, Event->key.keysym.mod );
			break;
		case InputEvent::KeyDown:
			SendKeyDown( Event->key.keysym.sym, Event->key.keysym.unicode, Event->key.keysym.mod );

			CheckTabPress( Event->key.keysym.sym );
			break;
	}
}

void cUIManager::ResizeControl() {
	mControl->Size( mWindow->GetWidth(), mWindow->GetHeight() );
	SendMsg( mControl, cUIMessage::MsgWindowResize );
}

void cUIManager::SendKeyUp( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod ) {
	cUIEventKey	KeyEvent	= cUIEventKey( mFocusControl, cUIEvent::EventKeyUp, KeyCode, Char, Mod );
	cUIControl * CtrlLoop	= mFocusControl;

	while( NULL != CtrlLoop ) {
		if ( CtrlLoop->Enabled() && CtrlLoop->OnKeyUp( KeyEvent ) )
			break;

		CtrlLoop = CtrlLoop->Parent();
	}
}

void cUIManager::SendKeyDown( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod ) {
	cUIEventKey	KeyEvent	= cUIEventKey( mFocusControl, cUIEvent::EventKeyDown, KeyCode, Char, Mod );
	cUIControl * CtrlLoop	= mFocusControl;

	while( NULL != CtrlLoop ) {
		if ( CtrlLoop->Enabled() && CtrlLoop->OnKeyDown( KeyEvent ) )
			break;

		CtrlLoop = CtrlLoop->Parent();
	}
}

cUIControl * cUIManager::FocusControl() const {
	return mFocusControl;
}

void cUIManager::FocusControl( cUIControl * Ctrl ) {
	if ( NULL != Ctrl && Ctrl != mFocusControl ) {
		cUIControl * CtrlFocusLoss = mFocusControl;

		mFocusControl = Ctrl;

		CtrlFocusLoss->OnFocusLoss();
		SendMsg( CtrlFocusLoss, cUIMessage::MsgFocusLoss );

		mFocusControl->OnFocus();
		SendMsg( mFocusControl, cUIMessage::MsgFocus );
	}
}

cUIControl * cUIManager::OverControl() const {
	return mOverControl;
}

void cUIManager::OverControl( cUIControl * Ctrl ) {
	mOverControl = Ctrl;
}

void cUIManager::SendMsg( cUIControl * Ctrl, const Uint32& Msg, const Uint32& Flags ) {
	cUIMessage tMsg( Ctrl, Msg, Flags );

	Ctrl->MessagePost( &tMsg );
}

void cUIManager::Update() {
	mElapsed = mWindow->Elapsed();

	mControl->Update();

	if ( mKM->PressTrigger() ) {
		if ( NULL != mOverControl ) {
			if ( mOverControl != mFocusControl )
				FocusControl( mOverControl );

			mOverControl->OnMouseDown( mKM->GetMousePos(), mKM->PressTrigger() );
			SendMsg( mOverControl, cUIMessage::MsgMouseDown, mKM->PressTrigger() );
		}

		if ( !mFirstPress ) {
			mDownControl = mOverControl;

			mFirstPress = true;
		}
	}

	if ( mKM->ReleaseTrigger() ) {
		if ( NULL != mFocusControl ) {
			mFocusControl->OnMouseUp( mKM->GetMousePos(), mKM->ReleaseTrigger() );
			SendMsg( mFocusControl, cUIMessage::MsgMouseUp, mKM->ClickTrigger() );

			if ( mDownControl == mOverControl && mKM->ClickTrigger() ) {
				SendMsg( mFocusControl, cUIMessage::MsgClick, mKM->ClickTrigger() );
				mFocusControl->OnMouseClick( mKM->GetMousePos(), mKM->ClickTrigger() );

				if ( mKM->DoubleClickTrigger() ) {
					SendMsg( mFocusControl, cUIMessage::MsgDoubleClick, mKM->DoubleClickTrigger() );
					mFocusControl->OnMouseDoubleClick( mKM->GetMousePos(), mKM->DoubleClickTrigger() );
				}
			}
		}

		mFirstPress = false;
	}

	cUIControl * pOver = mControl->OverFind( mKM->GetMousePosf() );

	if ( pOver != mOverControl ) {
		if ( NULL != mOverControl ) {
			SendMsg( mOverControl, cUIMessage::MsgMouseExit );
			mOverControl->OnMouseExit( mKM->GetMousePos(), 0 );
		}

		mOverControl = pOver;

		if ( NULL != mOverControl ) {
			SendMsg( mOverControl, cUIMessage::MsgMouseEnter );
			mOverControl->OnMouseEnter( mKM->GetMousePos(), 0 );
		}
	} else {
		if ( NULL != mOverControl )
			mOverControl->OnMouseMove( mKM->GetMousePos(), mKM->PressTrigger() );
	}

	mControl->CheckClose();
}

void cUIManager::Draw() {
	mControl->InternalDraw();
	cGlobalBatchRenderer::instance()->Draw();
}

cUIWindow * cUIManager::MainControl() const {
	return mControl;
}

const eeFloat& cUIManager::Elapsed() const {
	return mElapsed;
}

eeVector2i cUIManager::GetMousePos() {
	return mKM->GetMousePos();
}

cInput * cUIManager::GetInput() const {
	return mKM;
}

const Uint32& cUIManager::PressTrigger() const {
	return mKM->PressTrigger();
}

const Uint32& cUIManager::LastPressTrigger() const {
	return mKM->LastPressTrigger();
}

void cUIManager::ClipEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height ) {
	mWindow->ClipPlaneEnable( x, y, Width, Height );
}

void cUIManager::ClipDisable() {
	mWindow->ClipPlaneDisable();
}

void cUIManager::HighlightFocus( bool Highlight ) {
	SetFlagValue( &mFlags, UI_MANAGER_HIGHLIGHT_FOCUS, Highlight ? 1 : 0 );
}

bool cUIManager::HighlightFocus() const {
	return 0 != ( mFlags & UI_MANAGER_HIGHLIGHT_FOCUS );
}

void cUIManager::HighlightColor( const eeColorA& Color ) {
	mHighlightColor = Color;
}

const eeColorA& cUIManager::HighlightColor() const {
	return mHighlightColor;
}

void cUIManager::CheckTabPress( const Uint32& KeyCode ) {
	if ( KeyCode == KEY_TAB ) {
		cUIControl * Ctrl = mFocusControl->NextComplexControl();

		if ( NULL != Ctrl )
			Ctrl->SetFocus();
	}
}

void cUIManager::SendMouseClick( cUIControl * ToCtrl, const eeVector2i& Pos, const Uint32 Flags ) {
	SendMsg( ToCtrl, cUIMessage::MsgClick, Flags );
	ToCtrl->OnMouseClick( Pos, Flags );
}

void cUIManager::SendMouseUp( cUIControl * ToCtrl, const eeVector2i& Pos, const Uint32 Flags ) {
	SendMsg( ToCtrl, cUIMessage::MsgMouseUp, Flags );
	ToCtrl->OnMouseUp( Pos, Flags );
}

void cUIManager::SendMouseDown( cUIControl * ToCtrl, const eeVector2i& Pos, const Uint32 Flags ) {
	SendMsg( ToCtrl, cUIMessage::MsgMouseDown, Flags );
	ToCtrl->OnMouseDown( Pos, Flags );
}

}}
