#include <eepp/ui/uimanager.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/cursormanager.hpp>
#include <eepp/graphics/globalbatchrenderer.hpp>
#include <algorithm>

namespace EE { namespace UI {

SINGLETON_DECLARE_IMPLEMENTATION(UIManager)

UIManager::UIManager() :
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
	mControlDragging( false ),
	mUseGlobalCursors( true )
{
}

UIManager::~UIManager() {
	Shutdown();
}

void UIManager::Init( Uint32 Flags, EE::Window::Window * window ) {
	if ( mInit )
		Shutdown();

	mWindow		= window;
	mFlags		= Flags;

	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->GetCurrentWindow();
	}

	mKM				= mWindow->GetInput();

	mInit			= true;

	UIWindow::CreateParams Params;
	Params.Parent( NULL );
	Params.PosSet( 0, 0 );
	Params.SizeSet( Engine::instance()->GetWidth(), Engine::instance()->GetHeight() );
	Params.Flags = UI_CONTROL_DEFAULT_FLAGS | UI_REPORT_SIZE_CHANGE_TO_CHILDS;
	Params.WinFlags = UI_WIN_NO_BORDER | UI_WIN_RESIZEABLE;
	Params.MinWindowSize = Sizei( 0, 0 );
	Params.DecorationSize = Sizei( 0, 0 );
	Params.DecorationAutoSize = false;

	mControl		= eeNew( UIWindow, ( Params ) );
	mControl->Visible( true );
	mControl->Enabled( true );
	mControl->Container()->Enabled( false );
	mControl->Container()->Visible( false );

	mFocusControl	= mControl;
	mOverControl	= mControl;

	mCbId = mKM->PushCallback( cb::Make1( this, &UIManager::InputCallback ) );
	mResizeCb = mWindow->PushResizeCallback( cb::Make1( this, &UIManager::ResizeControl ) );
}

void UIManager::Shutdown() {
	if ( mInit ) {
		if ( -1 != mCbId &&
			NULL != Engine::existsSingleton() &&
			Engine::instance()->ExistsWindow( mWindow )
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

	UIThemeManager::destroySingleton();
}

void UIManager::InputCallback( InputEvent * Event ) {
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

void UIManager::ResizeControl( EE::Window::Window * win ) {
	mControl->Size( mWindow->GetWidth(), mWindow->GetHeight() );
	SendMsg( mControl, UIMessage::MsgWindowResize );

	std::list<UIWindow*>::iterator it;

	for ( it = mWindowsList.begin(); it != mWindowsList.end(); it++ ) {
		SendMsg( *it, UIMessage::MsgWindowResize );
	}
}

void UIManager::SendKeyUp( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod ) {
	UIEventKey	KeyEvent	= UIEventKey( mFocusControl, UIEvent::EventKeyUp, KeyCode, Char, Mod );
	UIControl * CtrlLoop	= mFocusControl;

	while( NULL != CtrlLoop ) {
		if ( CtrlLoop->Enabled() && CtrlLoop->OnKeyUp( KeyEvent ) )
			break;

		CtrlLoop = CtrlLoop->Parent();
	}
}

void UIManager::SendKeyDown( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod ) {
	UIEventKey	KeyEvent	= UIEventKey( mFocusControl, UIEvent::EventKeyDown, KeyCode, Char, Mod );
	UIControl * CtrlLoop	= mFocusControl;

	while( NULL != CtrlLoop ) {
		if ( CtrlLoop->Enabled() && CtrlLoop->OnKeyDown( KeyEvent ) )
			break;

		CtrlLoop = CtrlLoop->Parent();
	}
}

UIControl * UIManager::FocusControl() const {
	return mFocusControl;
}

UIControl * UIManager::LossFocusControl() const {
	return mLossFocusControl;
}

void UIManager::FocusControl( UIControl * Ctrl ) {
	if ( NULL != mFocusControl && NULL != Ctrl && Ctrl != mFocusControl ) {
		mLossFocusControl = mFocusControl;

		mFocusControl = Ctrl;

		mLossFocusControl->OnFocusLoss();
		SendMsg( mLossFocusControl, UIMessage::MsgFocusLoss );

		mFocusControl->OnFocus();
		SendMsg( mFocusControl, UIMessage::MsgFocus );
	}
}

UIControl * UIManager::OverControl() const {
	return mOverControl;
}

void UIManager::OverControl( UIControl * Ctrl ) {
	mOverControl = Ctrl;
}

void UIManager::SendMsg( UIControl * Ctrl, const Uint32& Msg, const Uint32& Flags ) {
	UIMessage tMsg( Ctrl, Msg, Flags );

	Ctrl->MessagePost( &tMsg );
}

void UIManager::Update() {
	mElapsed = mWindow->Elapsed();

	bool wasDraggingControl = IsControlDragging();

	mControl->Update();

	UIControl * pOver = mControl->OverFind( mKM->GetMousePosf() );

	if ( pOver != mOverControl ) {
		if ( NULL != mOverControl ) {
			SendMsg( mOverControl, UIMessage::MsgMouseExit );
			mOverControl->OnMouseExit( mKM->GetMousePos(), 0 );
		}

		mOverControl = pOver;

		if ( NULL != mOverControl ) {
			SendMsg( mOverControl, UIMessage::MsgMouseEnter );
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
			SendMsg( mOverControl, UIMessage::MsgMouseDown, mKM->PressTrigger() );
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
				SendMsg( mFocusControl, UIMessage::MsgMouseUp, mKM->ReleaseTrigger() );

				if ( mKM->ClickTrigger() ) { // mDownControl == mOverControl &&
					SendMsg( mFocusControl, UIMessage::MsgClick, mKM->ClickTrigger() );
					mFocusControl->OnMouseClick( mKM->GetMousePos(), mKM->ClickTrigger() );

					if ( mKM->DoubleClickTrigger() ) {
						SendMsg( mFocusControl, UIMessage::MsgDoubleClick, mKM->DoubleClickTrigger() );
						mFocusControl->OnMouseDoubleClick( mKM->GetMousePos(), mKM->DoubleClickTrigger() );
					}
				}
			}
		}

		mFirstPress = false;
	}

	CheckClose();
}

UIControl * UIManager::DownControl() const {
	return mDownControl;
}

void UIManager::Draw() {
	GlobalBatchRenderer::instance()->Draw();
	mControl->InternalDraw();
	GlobalBatchRenderer::instance()->Draw();
}

UIWindow * UIManager::MainControl() const {
	return mControl;
}

const Time& UIManager::Elapsed() const {
	return mElapsed;
}

Vector2i UIManager::GetMousePos() {
	return mKM->GetMousePos();
}

Input * UIManager::GetInput() const {
	return mKM;
}

const Uint32& UIManager::PressTrigger() const {
	return mKM->PressTrigger();
}

const Uint32& UIManager::LastPressTrigger() const {
	return mKM->LastPressTrigger();
}

void UIManager::ClipEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height ) {
	mWindow->ClipPlaneEnable( x, y, Width, Height );
}

void UIManager::ClipDisable() {
	mWindow->ClipPlaneDisable();
}

void UIManager::HighlightFocus( bool Highlight ) {
	BitOp::setBitFlagValue( &mFlags, UI_MANAGER_HIGHLIGHT_FOCUS, Highlight ? 1 : 0 );
}

bool UIManager::HighlightFocus() const {
	return 0 != ( mFlags & UI_MANAGER_HIGHLIGHT_FOCUS );
}

void UIManager::HighlightFocusColor( const ColorA& Color ) {
	mHighlightFocusColor = Color;
}

const ColorA& UIManager::HighlightFocusColor() const {
	return mHighlightFocusColor;
}

void UIManager::HighlightOver( bool Highlight ) {
	BitOp::setBitFlagValue( &mFlags, UI_MANAGER_HIGHLIGHT_OVER, Highlight ? 1 : 0 );
}

bool UIManager::HighlightOver() const {
	return 0 != ( mFlags & UI_MANAGER_HIGHLIGHT_OVER );
}

void UIManager::HighlightOverColor( const ColorA& Color ) {
	mHighlightOverColor = Color;
}

const ColorA& UIManager::HighlightOverColor() const {
	return mHighlightOverColor;
}

void UIManager::CheckTabPress( const Uint32& KeyCode ) {
	eeASSERT( NULL != mFocusControl );

	if ( KeyCode == KEY_TAB ) {
		UIControl * Ctrl = mFocusControl->NextComplexControl();

		if ( NULL != Ctrl )
			Ctrl->SetFocus();
	}
}

void UIManager::SendMouseClick( UIControl * ToCtrl, const Vector2i& Pos, const Uint32 Flags ) {
	SendMsg( ToCtrl, UIMessage::MsgClick, Flags );
	ToCtrl->OnMouseClick( Pos, Flags );
}

void UIManager::SendMouseUp( UIControl * ToCtrl, const Vector2i& Pos, const Uint32 Flags ) {
	SendMsg( ToCtrl, UIMessage::MsgMouseUp, Flags );
	ToCtrl->OnMouseUp( Pos, Flags );
}

void UIManager::SendMouseDown( UIControl * ToCtrl, const Vector2i& Pos, const Uint32 Flags ) {
	SendMsg( ToCtrl, UIMessage::MsgMouseDown, Flags );
	ToCtrl->OnMouseDown( Pos, Flags );
}

EE::Window::Window * UIManager::GetWindow() const {
	return mWindow;
}

void UIManager::SetFocusLastWindow( UIWindow * window ) {
	if ( !mWindowsList.empty() && window != mWindowsList.front() ) {
		FocusControl( mWindowsList.front() );
	}
}

void UIManager::WindowAdd( UIWindow * win ) {
	if ( !WindowExists( win ) ) {
		mWindowsList.push_front( win );
	} else {
		//! Send to front
		mWindowsList.remove( win );
		mWindowsList.push_front( win );
	}
}

void UIManager::WindowRemove( UIWindow * win ) {
	if ( WindowExists( win ) ) {
		mWindowsList.remove( win );
	}
}

bool UIManager::WindowExists( UIWindow * win ) {
	return mWindowsList.end() != std::find( mWindowsList.begin(), mWindowsList.end(), win );
}

const bool& UIManager::IsShootingDown() const {
	return mShootingDown;
}

const Vector2i &UIManager::GetMouseDownPos() const {
	return mMouseDownPos;
}

void UIManager::AddToCloseQueue( UIControl * Ctrl ) {
	eeASSERT( NULL != Ctrl );

	std::list<UIControl*>::iterator it;
	UIControl * itCtrl = NULL;

	for ( it = mCloseList.begin(); it != mCloseList.end(); it++ ) {
		itCtrl = *it;

		if ( NULL != itCtrl && itCtrl->IsParentOf( Ctrl ) ) {
			// If a parent will be removed, means that the control
			// that we are trying to queue will be removed by the father
			// so we skip it
			return;
		}
	}

	std::list< std::list<UIControl*>::iterator > itEraseList;

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
	std::list< std::list<UIControl*>::iterator >::iterator ite;

	for ( ite = itEraseList.begin(); ite != itEraseList.end(); ite++ ) {
		mCloseList.erase( *ite );
	}

	mCloseList.push_back( Ctrl );
}

void UIManager::CheckClose() {
	if ( !mCloseList.empty() ) {
		for ( std::list<UIControl*>::iterator it = mCloseList.begin(); it != mCloseList.end(); it++ ) {
			eeDelete( *it );
		}

		mCloseList.clear();
	}
}

void UIManager::SetControlDragging( bool dragging ) {
	mControlDragging = dragging;
}

const bool& UIManager::IsControlDragging() const {
	return mControlDragging;
}

void UIManager::UseGlobalCursors( const bool& use ) {
	mUseGlobalCursors = use;
}

const bool& UIManager::UseGlobalCursors() {
	return mUseGlobalCursors;
}

void UIManager::SetCursor( EE_CURSOR_TYPE cursor ) {
	if ( mUseGlobalCursors ) {
		mWindow->GetCursorManager()->Set( cursor );
	}
}

}}
