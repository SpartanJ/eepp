#include <eepp/ui/cuimanager.hpp>
#include <eepp/window/cengine.hpp>
#include <eepp/window/ccursormanager.hpp>
#include <eepp/graphics/cglobalbatchrenderer.hpp>
#include <algorithm>

namespace EE { namespace UI {

SINGLETON_DECLARE_IMPLEMENTATION(cUIManager)

cUIManager::cUIManager() :
	mWindow( NULL ),
	mKM( NULL ),
	mControl( NULL ),
	mFocusControl( NULL ),
	mOverControl( NULL ),
	mDownControl( NULL ),
	mLossFocusControl( NULL ),
	mCbId(-1),
	mResizeCb(0),
	mFlags( 0 ),
	mHighlightFocusColor( 234, 195, 123, 255 ),
	mHighlightOverColor( 195, 123, 234, 255 ),
	mInit( false ),
	mFirstPress( false ),
	mShootingDown( false ),
	mUseGlobalCursors( true )
{
}

cUIManager::~cUIManager() {
	Shutdown();
}

void cUIManager::Init( Uint32 Flags, Window::cWindow * window ) {
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
	mControl->Container()->Enabled( false );
	mControl->Container()->Visible( false );

	mFocusControl	= mControl;
	mOverControl	= mControl;

	mCbId = mKM->PushCallback( cb::Make1( this, &cUIManager::InputCallback ) );
	mResizeCb = mWindow->PushResizeCallback( cb::Make1( this, &cUIManager::ResizeControl ) );
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

		mShootingDown = true;

		eeSAFE_DELETE( mControl );

		mShootingDown = false;

		mOverControl = NULL;
		mFocusControl = NULL;

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

void cUIManager::ResizeControl( cWindow * win ) {
	mControl->Size( mWindow->GetWidth(), mWindow->GetHeight() );
	SendMsg( mControl, cUIMessage::MsgWindowResize );

	std::list<cUIWindow*>::iterator it;

	for ( it = mWindowsList.begin(); it != mWindowsList.end(); it++ ) {
		SendMsg( *it, cUIMessage::MsgWindowResize );
	}
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

cUIControl * cUIManager::LossFocusControl() const {
	return mLossFocusControl;
}

void cUIManager::FocusControl( cUIControl * Ctrl ) {
	if ( NULL != mFocusControl && NULL != Ctrl && Ctrl != mFocusControl ) {
		mLossFocusControl = mFocusControl;

		mFocusControl = Ctrl;

		mLossFocusControl->OnFocusLoss();
		SendMsg( mLossFocusControl, cUIMessage::MsgFocusLoss );

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

	bool wasDraggingControl = IsControlDragging();

	mControl->Update();

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

	if ( mKM->PressTrigger() ) {
		/*if ( !wasDraggingControl && mOverControl != mFocusControl )
			FocusControl( mOverControl );*/

		if ( NULL != mOverControl ) {
			mOverControl->OnMouseDown( mKM->GetMousePos(), mKM->PressTrigger() );
			SendMsg( mOverControl, cUIMessage::MsgMouseDown, mKM->PressTrigger() );
		}

		if ( !mFirstPress ) {
			mDownControl = mOverControl;
			mMouseDownPos = mKM->GetMousePos();

			mFirstPress = true;
		}
	}

	if ( mKM->ReleaseTrigger() ) {
		if ( NULL != mFocusControl ) {
			if ( !wasDraggingControl ) {
				if ( mOverControl != mFocusControl )
					FocusControl( mOverControl );

				mFocusControl->OnMouseUp( mKM->GetMousePos(), mKM->ReleaseTrigger() );
				SendMsg( mFocusControl, cUIMessage::MsgMouseUp, mKM->ReleaseTrigger() );

				if ( mKM->ClickTrigger() ) { // mDownControl == mOverControl &&
					SendMsg( mFocusControl, cUIMessage::MsgClick, mKM->ClickTrigger() );
					mFocusControl->OnMouseClick( mKM->GetMousePos(), mKM->ClickTrigger() );

					if ( mKM->DoubleClickTrigger() ) {
						SendMsg( mFocusControl, cUIMessage::MsgDoubleClick, mKM->DoubleClickTrigger() );
						mFocusControl->OnMouseDoubleClick( mKM->GetMousePos(), mKM->DoubleClickTrigger() );
					}
				}
			}
		}

		mFirstPress = false;
	}

	CheckClose();
}

cUIControl * cUIManager::DownControl() const {
	return mDownControl;
}

void cUIManager::Draw() {
	cGlobalBatchRenderer::instance()->Draw();
	mControl->InternalDraw();
	cGlobalBatchRenderer::instance()->Draw();
}

cUIWindow * cUIManager::MainControl() const {
	return mControl;
}

const cTime& cUIManager::Elapsed() const {
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
	BitOp::SetBitFlagValue( &mFlags, UI_MANAGER_HIGHLIGHT_FOCUS, Highlight ? 1 : 0 );
}

bool cUIManager::HighlightFocus() const {
	return 0 != ( mFlags & UI_MANAGER_HIGHLIGHT_FOCUS );
}

void cUIManager::HighlightFocusColor( const eeColorA& Color ) {
	mHighlightFocusColor = Color;
}

const eeColorA& cUIManager::HighlightFocusColor() const {
	return mHighlightFocusColor;
}

void cUIManager::HighlightOver( bool Highlight ) {
	BitOp::SetBitFlagValue( &mFlags, UI_MANAGER_HIGHLIGHT_OVER, Highlight ? 1 : 0 );
}

bool cUIManager::HighlightOver() const {
	return 0 != ( mFlags & UI_MANAGER_HIGHLIGHT_OVER );
}

void cUIManager::HighlightOverColor( const eeColorA& Color ) {
	mHighlightOverColor = Color;
}

const eeColorA& cUIManager::HighlightOverColor() const {
	return mHighlightOverColor;
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

Window::cWindow * cUIManager::GetWindow() const {
	return mWindow;
}

void cUIManager::SetFocusLastWindow( cUIWindow * window ) {
	if ( !mWindowsList.empty() && window != mWindowsList.front() ) {
		FocusControl( mWindowsList.front() );
	}
}

void cUIManager::WindowAdd( cUIWindow * win ) {
	if ( !WindowExists( win ) ) {
		mWindowsList.push_front( win );
	} else {
		//! Send to front
		mWindowsList.remove( win );
		mWindowsList.push_front( win );
	}
}

void cUIManager::WindowRemove( cUIWindow * win ) {
	if ( WindowExists( win ) ) {
		mWindowsList.remove( win );
	}
}

bool cUIManager::WindowExists( cUIWindow * win ) {
	return mWindowsList.end() != std::find( mWindowsList.begin(), mWindowsList.end(), win );
}

const bool& cUIManager::IsShootingDown() const {
	return mShootingDown;
}

const eeVector2i &cUIManager::GetMouseDownPos() const {
	return mMouseDownPos;
}

void cUIManager::AddToCloseQueue( cUIControl * Ctrl ) {
	eeASSERT( NULL != Ctrl );

	std::list<cUIControl*>::iterator it;
	cUIControl * itCtrl = NULL;

	for ( it = mCloseList.begin(); it != mCloseList.end(); it++ ) {
		itCtrl = *it;

		if ( NULL != itCtrl && itCtrl->IsParentOf( Ctrl ) ) {
			// If a parent will be removed, means that the control
			// that we are trying to queue will be removed by the father
			// so we skip it
			return;
		}
	}

	std::list< std::list<cUIControl*>::iterator > itEraseList;

	for ( it = mCloseList.begin(); it != mCloseList.end(); it++ ) {
		itCtrl = *it;

		if ( NULL != itCtrl && Ctrl->IsParentOf( itCtrl ) ) {
			// if the control added is parent of another control already added,
			// we remove the already added control because it will be deleted
			// by its parent
			itEraseList.push_back( it );
		} else if ( NULL == itCtrl ) {
			itEraseList.push_back( it );
		}
	}

	// We delete all the controls that don't need to be deleted
	// because of the new added control to the queue
	std::list< std::list<cUIControl*>::iterator >::iterator ite;

	for ( ite = itEraseList.begin(); ite != itEraseList.end(); ite++ ) {
		mCloseList.erase( *ite );
	}

	mCloseList.push_back( Ctrl );
}

void cUIManager::CheckClose() {
	if ( !mCloseList.empty() ) {
		for ( std::list<cUIControl*>::iterator it = mCloseList.begin(); it != mCloseList.end(); it++ ) {
			eeDelete( *it );
		}

		mCloseList.clear();
	}
}

void cUIManager::SetControlDragging( bool dragging ) {
	mControlDragging = dragging;
}

const bool& cUIManager::IsControlDragging() const {
	return mControlDragging;
}

void cUIManager::UseGlobalCursors( const bool& use ) {
	mUseGlobalCursors = use;
}

const bool& cUIManager::UseGlobalCursors() {
	return mUseGlobalCursors;
}

void cUIManager::SetCursor( EE_CURSOR_TYPE cursor ) {
	if ( mUseGlobalCursors ) {
		mWindow->GetCursorManager()->Set( cursor );
	}
}

}}
