#include <algorithm>
#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/scene/scenenode.hpp>
#include <eepp/window/engine.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/inputevent.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace Scene {

EventDispatcher* EventDispatcher::New( SceneNode* sceneNode ) {
	return eeNew( EventDispatcher, ( sceneNode ) );
}

EventDispatcher::EventDispatcher( SceneNode* sceneNode ) :
	mWindow( sceneNode->getWindow() ),
	mInput( mWindow->getInput() ),
	mSceneNode( sceneNode ),
	mFocusControl( sceneNode ),
	mOverControl( NULL ),
	mDownControl( NULL ),
	mLossFocusControl( NULL ),
	mCbId( 0 ),
	mFirstPress( false ),
	mNodeDragging( NULL ) {
	mCbId = mInput->pushCallback( cb::Make1( this, &EventDispatcher::inputCallback ) );
}

EventDispatcher::~EventDispatcher() {
	if ( -1 != mCbId && NULL != Engine::existsSingleton() &&
		 Engine::instance()->existsWindow( mWindow ) ) {
		mInput->popCallback( mCbId );
	}
}

void EventDispatcher::inputCallback( InputEvent* Event ) {
	switch ( Event->Type ) {
		case InputEvent::Window: {
			if ( Event->window.type == InputEvent::WindowKeyboardFocusGain ) {
				mMousePosi = mInput->queryMousePos();
				mMousePos = mMousePosi.asFloat();
			}
			break;
		}
		case InputEvent::KeyUp:
			sendKeyUp( Event->key.keysym.sym, Event->key.keysym.unicode, Event->key.keysym.mod );
			break;
		case InputEvent::KeyDown:
			sendKeyDown( Event->key.keysym.sym, Event->key.keysym.unicode, Event->key.keysym.mod );
			break;
		case InputEvent::SysWM:
		case InputEvent::VideoResize:
		case InputEvent::VideoExpose: {
			if ( NULL != mSceneNode )
				mSceneNode->invalidate( NULL );
		}
	}
}

void EventDispatcher::update( const Time& time ) {
	bool wasDraggingControl = mNodeDragging;

	mElapsed = time;
	mMousePos = mInput->getMousePosFromView( mWindow->getDefaultView() );
	mMousePosi = mMousePos.asInt();

	Node* pOver = mSceneNode->overFind( mMousePos );

	if ( pOver != mOverControl ) {
		if ( NULL != mOverControl ) {
			mOverControl->onMouseLeave( mMousePosi, 0 );
			sendMsg( mOverControl, NodeMessage::MouseLeave );
		}

		mOverControl = pOver;

		if ( NULL != mOverControl ) {
			mOverControl->onMouseOver( mMousePosi, 0 );
			sendMsg( mOverControl, NodeMessage::MouseOver );
		}
	} else {
		if ( NULL != mOverControl && mLastMousePos != mMousePos ) {
			mOverControl->onMouseMove( mMousePosi, mInput->getPressTrigger() );
			sendMsg( mOverControl, NodeMessage::MouseMove, mInput->getPressTrigger() );
		}
	}

	if ( NULL != mNodeDragging )
		mNodeDragging->onCalculateDrag( mMousePos, mInput->getPressTrigger() );

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
				if ( mOverControl != mFocusControl &&
					 mInput->getReleaseTrigger() & ( EE_BUTTON_LMASK | EE_BUTTON_RMASK ) )
					setFocusControl( mOverControl );

				// The focused control can change after the MouseUp ( since the control can call
				// "setFocus()" on other control And the Click would be received by the new focused
				// control instead of the real one
				Node* lastFocusControl = mFocusControl;

				if ( NULL != mOverControl ) {
					getOverControl()->onMouseUp( mMousePosi, mInput->getReleaseTrigger() );

					if ( NULL != getOverControl() )
						sendMsg( getOverControl(), NodeMessage::MouseUp,
								 mInput->getReleaseTrigger() );
				}

				if ( mInput->getClickTrigger() ) {
					lastFocusControl->onMouseClick( mMousePosi, mInput->getClickTrigger() );
					sendMsg( lastFocusControl, NodeMessage::Click, mInput->getClickTrigger() );

					if ( mInput->getDoubleClickTrigger() ) {
						lastFocusControl->onMouseDoubleClick( mMousePosi,
															  mInput->getDoubleClickTrigger() );
						sendMsg( lastFocusControl, NodeMessage::DoubleClick,
								 mInput->getDoubleClickTrigger() );
					}
				}
			}
		}

		mFirstPress = false;
	}

	mLastMousePos = mMousePos;
}

Input* EventDispatcher::getInput() const {
	return mInput;
}

void EventDispatcher::sendKeyUp( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod ) {
	KeyEvent keyEvent = KeyEvent( mFocusControl, Event::KeyUp, KeyCode, Char, Mod );
	Node* CtrlLoop = mFocusControl;

	while ( NULL != CtrlLoop ) {
		if ( CtrlLoop->isEnabled() && CtrlLoop->onKeyUp( keyEvent ) )
			break;

		CtrlLoop = CtrlLoop->getParent();
	}
}

void EventDispatcher::sendKeyDown( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod ) {
	KeyEvent keyEvent = KeyEvent( mFocusControl, Event::KeyDown, KeyCode, Char, Mod );
	Node* CtrlLoop = mFocusControl;

	while ( NULL != CtrlLoop ) {
		if ( CtrlLoop->isEnabled() && CtrlLoop->onKeyDown( keyEvent ) )
			break;

		CtrlLoop = CtrlLoop->getParent();
	}
}

void EventDispatcher::sendMsg( Node* Ctrl, const Uint32& Msg, const Uint32& Flags ) {
	NodeMessage tMsg( Ctrl, Msg, Flags );

	Ctrl->messagePost( &tMsg );
}

void EventDispatcher::sendMouseClick( Node* ToCtrl, const Vector2i& Pos, const Uint32 Flags ) {
	sendMsg( ToCtrl, NodeMessage::Click, Flags );
	ToCtrl->onMouseClick( Pos, Flags );
}

void EventDispatcher::sendMouseUp( Node* ToCtrl, const Vector2i& Pos, const Uint32 Flags ) {
	sendMsg( ToCtrl, NodeMessage::MouseUp, Flags );
	ToCtrl->onMouseUp( Pos, Flags );
}

void EventDispatcher::sendMouseDown( Node* ToCtrl, const Vector2i& Pos, const Uint32 Flags ) {
	sendMsg( ToCtrl, NodeMessage::MouseDown, Flags );
	ToCtrl->onMouseDown( Pos, Flags );
}

void EventDispatcher::setFocusControl( Node* Ctrl ) {
	if ( NULL != mFocusControl && NULL != Ctrl && Ctrl != mFocusControl ) {
		mLossFocusControl = mFocusControl;

		mFocusControl = Ctrl;

		mLossFocusControl->onFocusLoss();
		sendMsg( mLossFocusControl, NodeMessage::FocusLoss );

		mFocusControl->onFocus();
		sendMsg( mFocusControl, NodeMessage::Focus );
	}
}

Node* EventDispatcher::getDownControl() const {
	return mDownControl;
}

Node* EventDispatcher::getOverControl() const {
	return mOverControl;
}

void EventDispatcher::setOverControl( Node* Ctrl ) {
	mOverControl = Ctrl;
}

Node* EventDispatcher::getFocusControl() const {
	return mFocusControl;
}

Node* EventDispatcher::getLossFocusControl() const {
	return mLossFocusControl;
}

const Uint32& EventDispatcher::getPressTrigger() const {
	return mInput->getPressTrigger();
}

const Uint32& EventDispatcher::getLastPressTrigger() const {
	return mInput->getLastPressTrigger();
}

const Uint32& EventDispatcher::getClickTrigger() const {
	return mInput->getClickTrigger();
}

const Uint32& EventDispatcher::getDoubleClickTrigger() const {
	return mInput->getDoubleClickTrigger();
}

void EventDispatcher::setNodeDragging( Node* dragging ) {
	mNodeDragging = dragging;
}

bool EventDispatcher::isNodeDragging() const {
	return NULL != mNodeDragging;
}

Vector2i EventDispatcher::getMousePos() {
	return mMousePosi;
}

Vector2f EventDispatcher::getMousePosf() {
	return mMousePos;
}

Vector2i EventDispatcher::getMouseDownPos() {
	return mMouseDownPos;
}

Vector2f EventDispatcher::getLastMousePos() {
	return mLastMousePos;
}

SceneNode* EventDispatcher::getSceneNode() const {
	return mSceneNode;
}

const Time& EventDispatcher::getLastFrameTime() const {
	return mElapsed;
}

}} // namespace EE::Scene
