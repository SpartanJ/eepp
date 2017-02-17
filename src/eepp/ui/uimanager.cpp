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
	shutdown();
}

void UIManager::init( Uint32 Flags, EE::Window::Window * window ) {
	if ( mInit )
		shutdown();

	mWindow		= window;
	mFlags		= Flags;

	if ( NULL == mWindow ) {
		mWindow = Engine::instance()->getCurrentWindow();
	}

	mKM				= mWindow->getInput();

	mInit			= true;

	UIWindow::CreateParams Params;
	Params.setParent( NULL );
	Params.setPos( 0, 0 );
	Params.setSize( Engine::instance()->getWidth(), Engine::instance()->getHeight() );
	Params.Flags = UI_CONTROL_DEFAULT_FLAGS | UI_REPORT_SIZE_CHANGE_TO_CHILDS;
	Params.WinFlags = UI_WIN_NO_BORDER | UI_WIN_RESIZEABLE;
	Params.MinWindowSize = Sizei( 0, 0 );
	Params.DecorationSize = Sizei( 0, 0 );
	Params.DecorationAutoSize = false;

	mControl		= eeNew( UIWindow, ( Params ) );
	mControl->visible( true );
	mControl->enabled( true );
	mControl->getContainer()->enabled( false );
	mControl->getContainer()->visible( false );

	mFocusControl	= mControl;
	mOverControl	= mControl;

	mCbId = mKM->pushCallback( cb::Make1( this, &UIManager::inputCallback ) );
	mResizeCb = mWindow->pushResizeCallback( cb::Make1( this, &UIManager::resizeControl ) );
}

void UIManager::shutdown() {
	if ( mInit ) {
		if ( -1 != mCbId &&
			NULL != Engine::existsSingleton() &&
			Engine::instance()->existsWindow( mWindow )
		)
		{
			mKM->popCallback( mCbId );
			mWindow->popResizeCallback( mResizeCb );
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

void UIManager::inputCallback( InputEvent * Event ) {
	switch( Event->Type ) {
		case InputEvent::KeyUp:
			sendKeyUp( Event->key.keysym.sym, Event->key.keysym.unicode, Event->key.keysym.mod );
			break;
		case InputEvent::KeyDown:
			sendKeyDown( Event->key.keysym.sym, Event->key.keysym.unicode, Event->key.keysym.mod );

			checkTabPress( Event->key.keysym.sym );
			break;
	}
}

void UIManager::resizeControl( EE::Window::Window * win ) {
	mControl->size( mWindow->getWidth(), mWindow->getHeight() );
	sendMsg( mControl, UIMessage::MsgWindowResize );

	std::list<UIWindow*>::iterator it;

	for ( it = mWindowsList.begin(); it != mWindowsList.end(); it++ ) {
		sendMsg( *it, UIMessage::MsgWindowResize );
	}
}

void UIManager::sendKeyUp( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod ) {
	UIEventKey	KeyEvent	= UIEventKey( mFocusControl, UIEvent::EventKeyUp, KeyCode, Char, Mod );
	UIControl * CtrlLoop	= mFocusControl;

	while( NULL != CtrlLoop ) {
		if ( CtrlLoop->enabled() && CtrlLoop->onKeyUp( KeyEvent ) )
			break;

		CtrlLoop = CtrlLoop->parent();
	}
}

void UIManager::sendKeyDown( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod ) {
	UIEventKey	KeyEvent	= UIEventKey( mFocusControl, UIEvent::EventKeyDown, KeyCode, Char, Mod );
	UIControl * CtrlLoop	= mFocusControl;

	while( NULL != CtrlLoop ) {
		if ( CtrlLoop->enabled() && CtrlLoop->onKeyDown( KeyEvent ) )
			break;

		CtrlLoop = CtrlLoop->parent();
	}
}

UIControl * UIManager::focusControl() const {
	return mFocusControl;
}

UIControl * UIManager::lossFocusControl() const {
	return mLossFocusControl;
}

void UIManager::focusControl( UIControl * Ctrl ) {
	if ( NULL != mFocusControl && NULL != Ctrl && Ctrl != mFocusControl ) {
		mLossFocusControl = mFocusControl;

		mFocusControl = Ctrl;

		mLossFocusControl->onFocusLoss();
		sendMsg( mLossFocusControl, UIMessage::MsgFocusLoss );

		mFocusControl->onFocus();
		sendMsg( mFocusControl, UIMessage::MsgFocus );
	}
}

UIControl * UIManager::overControl() const {
	return mOverControl;
}

void UIManager::overControl( UIControl * Ctrl ) {
	mOverControl = Ctrl;
}

void UIManager::sendMsg( UIControl * Ctrl, const Uint32& Msg, const Uint32& Flags ) {
	UIMessage tMsg( Ctrl, Msg, Flags );

	Ctrl->messagePost( &tMsg );
}

void UIManager::update() {
	mElapsed = mWindow->elapsed();

	bool wasDraggingControl = isControlDragging();

	mControl->update();

	UIControl * pOver = mControl->overFind( mKM->getMousePosf() );

	if ( pOver != mOverControl ) {
		if ( NULL != mOverControl ) {
			sendMsg( mOverControl, UIMessage::MsgMouseExit );
			mOverControl->onMouseExit( mKM->getMousePos(), 0 );
		}

		mOverControl = pOver;

		if ( NULL != mOverControl ) {
			sendMsg( mOverControl, UIMessage::MsgMouseEnter );
			mOverControl->onMouseEnter( mKM->getMousePos(), 0 );
		}
	} else {
		if ( NULL != mOverControl )
			mOverControl->onMouseMove( mKM->getMousePos(), mKM->pressTrigger() );
	}

	if ( mKM->pressTrigger() ) {
		/*if ( !wasDraggingControl && mOverControl != mFocusControl )
			FocusControl( mOverControl );*/

		if ( NULL != mOverControl ) {
			mOverControl->onMouseDown( mKM->getMousePos(), mKM->pressTrigger() );
			sendMsg( mOverControl, UIMessage::MsgMouseDown, mKM->pressTrigger() );
		}

		if ( !mFirstPress ) {
			mDownControl = mOverControl;
			mMouseDownPos = mKM->getMousePos();

			mFirstPress = true;
		}
	}

	if ( mKM->releaseTrigger() ) {
		if ( NULL != mFocusControl ) {
			if ( !wasDraggingControl ) {
				if ( mOverControl != mFocusControl )
					focusControl( mOverControl );

				mFocusControl->onMouseUp( mKM->getMousePos(), mKM->releaseTrigger() );
				sendMsg( mFocusControl, UIMessage::MsgMouseUp, mKM->releaseTrigger() );

				if ( mKM->clickTrigger() ) { // mDownControl == mOverControl &&
					sendMsg( mFocusControl, UIMessage::MsgClick, mKM->clickTrigger() );
					mFocusControl->onMouseClick( mKM->getMousePos(), mKM->clickTrigger() );

					if ( mKM->doubleClickTrigger() ) {
						sendMsg( mFocusControl, UIMessage::MsgDoubleClick, mKM->doubleClickTrigger() );
						mFocusControl->onMouseDoubleClick( mKM->getMousePos(), mKM->doubleClickTrigger() );
					}
				}
			}
		}

		mFirstPress = false;
	}

	checkClose();
}

UIControl * UIManager::downControl() const {
	return mDownControl;
}

void UIManager::draw() {
	GlobalBatchRenderer::instance()->draw();
	mControl->internalDraw();
	GlobalBatchRenderer::instance()->draw();
}

UIWindow * UIManager::mainControl() const {
	return mControl;
}

const Time& UIManager::elapsed() const {
	return mElapsed;
}

Vector2i UIManager::getMousePos() {
	return mKM->getMousePos();
}

Input * UIManager::getInput() const {
	return mKM;
}

const Uint32& UIManager::pressTrigger() const {
	return mKM->pressTrigger();
}

const Uint32& UIManager::lastPressTrigger() const {
	return mKM->lastPressTrigger();
}

void UIManager::clipEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height ) {
	mWindow->clipPlaneEnable( x, y, Width, Height );
}

void UIManager::clipDisable() {
	mWindow->clipPlaneDisable();
}

void UIManager::highlightFocus( bool Highlight ) {
	BitOp::setBitFlagValue( &mFlags, UI_MANAGER_HIGHLIGHT_FOCUS, Highlight ? 1 : 0 );
}

bool UIManager::highlightFocus() const {
	return 0 != ( mFlags & UI_MANAGER_HIGHLIGHT_FOCUS );
}

void UIManager::highlightFocusColor( const ColorA& Color ) {
	mHighlightFocusColor = Color;
}

const ColorA& UIManager::highlightFocusColor() const {
	return mHighlightFocusColor;
}

void UIManager::highlightOver( bool Highlight ) {
	BitOp::setBitFlagValue( &mFlags, UI_MANAGER_HIGHLIGHT_OVER, Highlight ? 1 : 0 );
}

bool UIManager::highlightOver() const {
	return 0 != ( mFlags & UI_MANAGER_HIGHLIGHT_OVER );
}

void UIManager::highlightOverColor( const ColorA& Color ) {
	mHighlightOverColor = Color;
}

const ColorA& UIManager::highlightOverColor() const {
	return mHighlightOverColor;
}

void UIManager::checkTabPress( const Uint32& KeyCode ) {
	eeASSERT( NULL != mFocusControl );

	if ( KeyCode == KEY_TAB ) {
		UIControl * Ctrl = mFocusControl->nextComplexControl();

		if ( NULL != Ctrl )
			Ctrl->setFocus();
	}
}

void UIManager::sendMouseClick( UIControl * ToCtrl, const Vector2i& Pos, const Uint32 Flags ) {
	sendMsg( ToCtrl, UIMessage::MsgClick, Flags );
	ToCtrl->onMouseClick( Pos, Flags );
}

void UIManager::sendMouseUp( UIControl * ToCtrl, const Vector2i& Pos, const Uint32 Flags ) {
	sendMsg( ToCtrl, UIMessage::MsgMouseUp, Flags );
	ToCtrl->onMouseUp( Pos, Flags );
}

void UIManager::sendMouseDown( UIControl * ToCtrl, const Vector2i& Pos, const Uint32 Flags ) {
	sendMsg( ToCtrl, UIMessage::MsgMouseDown, Flags );
	ToCtrl->onMouseDown( Pos, Flags );
}

EE::Window::Window * UIManager::getWindow() const {
	return mWindow;
}

void UIManager::setFocusLastWindow( UIWindow * window ) {
	if ( !mWindowsList.empty() && window != mWindowsList.front() ) {
		focusControl( mWindowsList.front() );
	}
}

void UIManager::windowAdd( UIWindow * win ) {
	if ( !windowExists( win ) ) {
		mWindowsList.push_front( win );
	} else {
		//! Send to front
		mWindowsList.remove( win );
		mWindowsList.push_front( win );
	}
}

void UIManager::windowRemove( UIWindow * win ) {
	if ( windowExists( win ) ) {
		mWindowsList.remove( win );
	}
}

bool UIManager::windowExists( UIWindow * win ) {
	return mWindowsList.end() != std::find( mWindowsList.begin(), mWindowsList.end(), win );
}

const bool& UIManager::isShootingDown() const {
	return mShootingDown;
}

const Vector2i &UIManager::getMouseDownPos() const {
	return mMouseDownPos;
}

void UIManager::addToCloseQueue( UIControl * Ctrl ) {
	eeASSERT( NULL != Ctrl );

	std::list<UIControl*>::iterator it;
	UIControl * itCtrl = NULL;

	for ( it = mCloseList.begin(); it != mCloseList.end(); it++ ) {
		itCtrl = *it;

		if ( NULL != itCtrl && itCtrl->isParentOf( Ctrl ) ) {
			// If a parent will be removed, means that the control
			// that we are trying to queue will be removed by the father
			// so we skip it
			return;
		}
	}

	std::list< std::list<UIControl*>::iterator > itEraseList;

	for ( it = mCloseList.begin(); it != mCloseList.end(); it++ ) {
		itCtrl = *it;

		if ( NULL != itCtrl && Ctrl->isParentOf( itCtrl ) ) {
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

void UIManager::checkClose() {
	if ( !mCloseList.empty() ) {
		for ( std::list<UIControl*>::iterator it = mCloseList.begin(); it != mCloseList.end(); it++ ) {
			eeDelete( *it );
		}

		mCloseList.clear();
	}
}

void UIManager::setControlDragging( bool dragging ) {
	mControlDragging = dragging;
}

const bool& UIManager::isControlDragging() const {
	return mControlDragging;
}

void UIManager::useGlobalCursors( const bool& use ) {
	mUseGlobalCursors = use;
}

const bool& UIManager::useGlobalCursors() {
	return mUseGlobalCursors;
}

void UIManager::setCursor( EE_CURSOR_TYPE cursor ) {
	if ( mUseGlobalCursors ) {
		mWindow->getCursorManager()->set( cursor );
	}
}

}}
