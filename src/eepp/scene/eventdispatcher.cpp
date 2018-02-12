#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/inputevent.hpp>
#include <eepp/window/window.hpp>
#include <eepp/window/engine.hpp>

namespace EE { namespace Scene {

EventDispatcher * EventDispatcher::New( SceneNode * sceneNode ) {
	return eeNew( EventDispatcher, ( sceneNode ) );
}

EventDispatcher::EventDispatcher( SceneNode * sceneNode ) :
	mWindow( sceneNode->getWindow() ),
	mInput( mWindow->getInput() ),
	mSceneNode( sceneNode ),
	mFocusControl( sceneNode ),
	mOverControl( NULL ),
	mDownControl( NULL ),
	mLossFocusControl( NULL ),
	mCbId( 0 ),
	mFirstPress( false ),
	mControlDragging( false )
{
	mCbId = mInput->pushCallback( cb::Make1( this, &EventDispatcher::inputCallback ) );
}

EventDispatcher::~EventDispatcher() {
	if ( -1 != mCbId && NULL != Engine::existsSingleton() && Engine::instance()->existsWindow( mWindow ) ) {
		mInput->popCallback( mCbId );
	}
}

void EventDispatcher::inputCallback( InputEvent * Event ) {
	switch( Event->Type ) {
		case InputEvent::KeyUp:
			sendKeyUp( Event->key.keysym.sym, Event->key.keysym.unicode, Event->key.keysym.mod );
			break;
		case InputEvent::KeyDown:
			sendKeyDown( Event->key.keysym.sym, Event->key.keysym.unicode, Event->key.keysym.mod );

			//checkTabPress( Event->key.keysym.sym );
			break;
		case InputEvent::SysWM:
		case InputEvent::VideoResize:
		case InputEvent::VideoExpose:
		{
			if ( NULL != mSceneNode )
				mSceneNode->invalidate();
		}
	}
}

void EventDispatcher::update( const Time& elapsed ) {
	bool wasDraggingControl = mControlDragging;

	mMousePos = mInput->getMousePosFromView( mWindow->getDefaultView() );
	mMousePosi = mMousePos.asInt();

	Node * pOver = mSceneNode->overFind( mMousePos );

	if ( pOver != mOverControl ) {
		if ( NULL != mOverControl ) {
			sendMsg( mOverControl, NodeMessage::MouseExit );
			mOverControl->onMouseExit( mMousePosi, 0 );
		}

		mOverControl = pOver;

		if ( NULL != mOverControl ) {
			sendMsg( mOverControl, NodeMessage::MouseEnter );
			mOverControl->onMouseEnter( mMousePosi, 0 );
		}
	} else {
		if ( NULL != mOverControl )
			mOverControl->onMouseMove( mMousePosi, mInput->getPressTrigger() );
	}

	if ( mInput->getPressTrigger() ) {
		if ( NULL != mOverControl ) {
			mOverControl->onMouseDown( mMousePosi, mInput->getPressTrigger() );
			sendMsg( mOverControl, NodeMessage::MouseDown, mInput->getPressTrigger() );
		}

		if ( !mFirstPress ) {
			mDownControl = mOverControl;
			mMouseDownPos = mMousePosi;

			mFirstPress = true;
		}
	}

	if ( mInput->getReleaseTrigger() ) {
		if ( NULL != mFocusControl ) {
			if ( !wasDraggingControl || mMousePos == mLastMousePos ) {
				if ( mOverControl != mFocusControl )
					setFocusControl( mOverControl );

				// The focused control can change after the MouseUp ( since the control can call "setFocus()" on other control
				// And the Click would be received by the new focused control instead of the real one
				Node * lastFocusControl = mFocusControl;

				lastFocusControl->onMouseUp( mMousePosi, mInput->getReleaseTrigger() );
				sendMsg( lastFocusControl, NodeMessage::MouseUp, mInput->getReleaseTrigger() );

				if ( mInput->getClickTrigger() ) {
					sendMsg( lastFocusControl, NodeMessage::Click, mInput->getClickTrigger() );
					lastFocusControl->onMouseClick( mMousePosi, mInput->getClickTrigger() );

					if ( mInput->getDoubleClickTrigger() ) {
						sendMsg( lastFocusControl, NodeMessage::DoubleClick, mInput->getDoubleClickTrigger() );
						lastFocusControl->onMouseDoubleClick( mMousePosi, mInput->getDoubleClickTrigger() );
					}
				}
			}
		}

		mFirstPress = false;
	}

	mLastMousePos = mMousePos;
}

Input * EventDispatcher::getInput() const {
	return mInput;
}

void EventDispatcher::sendKeyUp( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod ) {
	KeyEvent	keyEvent	= KeyEvent( mFocusControl, Event::KeyUp, KeyCode, Char, Mod );
	Node * CtrlLoop	= mFocusControl;

	while( NULL != CtrlLoop ) {
		if ( CtrlLoop->isEnabled() && CtrlLoop->onKeyUp( keyEvent ) )
			break;

		CtrlLoop = CtrlLoop->getParent();
	}
}

void EventDispatcher::sendKeyDown( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod ) {
	KeyEvent	keyEvent	= KeyEvent( mFocusControl, Event::KeyDown, KeyCode, Char, Mod );
	Node * CtrlLoop	= mFocusControl;

	while( NULL != CtrlLoop ) {
		if ( CtrlLoop->isEnabled() && CtrlLoop->onKeyDown( keyEvent ) )
			break;

		CtrlLoop = CtrlLoop->getParent();
	}
}

void EventDispatcher::sendMsg( Node * Ctrl, const Uint32& Msg, const Uint32& Flags ) {
	NodeMessage tMsg( Ctrl, Msg, Flags );

	Ctrl->messagePost( &tMsg );
}

void EventDispatcher::sendMouseClick( Node * ToCtrl, const Vector2i& Pos, const Uint32 Flags ) {
	sendMsg( ToCtrl, NodeMessage::Click, Flags );
	ToCtrl->onMouseClick( Pos, Flags );
}

void EventDispatcher::sendMouseUp( Node * ToCtrl, const Vector2i& Pos, const Uint32 Flags ) {
	sendMsg( ToCtrl, NodeMessage::MouseUp, Flags );
	ToCtrl->onMouseUp( Pos, Flags );
}

void EventDispatcher::sendMouseDown( Node * ToCtrl, const Vector2i& Pos, const Uint32 Flags ) {
	sendMsg( ToCtrl, NodeMessage::MouseDown, Flags );
	ToCtrl->onMouseDown( Pos, Flags );
}

void EventDispatcher::setFocusControl( Node * Ctrl ) {
	if ( NULL != mFocusControl && NULL != Ctrl && Ctrl != mFocusControl ) {
		mLossFocusControl = mFocusControl;

		mFocusControl = Ctrl;

		mLossFocusControl->onFocusLoss();
		sendMsg( mLossFocusControl, NodeMessage::FocusLoss );

		mFocusControl->onFocus();
		sendMsg( mFocusControl, NodeMessage::Focus );
	}
}

Node * EventDispatcher::getDownControl() const {
	return mDownControl;
}

Node * EventDispatcher::getOverControl() const {
	return mOverControl;
}

void EventDispatcher::setOverControl( Node * Ctrl ) {
	mOverControl = Ctrl;
}

Node * EventDispatcher::getFocusControl() const {
	return mFocusControl;
}

Node * EventDispatcher::getLossFocusControl() const {
	return mLossFocusControl;
}

const Uint32& EventDispatcher::getPressTrigger() const {
	return mInput->getPressTrigger();
}

const Uint32& EventDispatcher::getLastPressTrigger() const {
	return mInput->getLastPressTrigger();
}

const Uint32 &EventDispatcher::getClickTrigger() const {
	return mInput->getClickTrigger();
}

const Uint32 &EventDispatcher::getDoubleClickTrigger() const {
	return mInput->getDoubleClickTrigger();
}

void EventDispatcher::setControlDragging( bool dragging ) {
	mControlDragging = dragging;
}

const bool& EventDispatcher::isControlDragging() const {
	return mControlDragging;
}

Vector2i EventDispatcher::getMousePos() {
	return mMousePosi;
}

Vector2f EventDispatcher::getMousePosf() {
	return mMousePos;
}

SceneNode *EventDispatcher::getSceneNode() const {
	return mSceneNode;
}

}}
