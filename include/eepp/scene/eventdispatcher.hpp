#ifndef EE_EVENTDISPATCHER_HPP
#define EE_EVENTDISPATCHER_HPP

#include <eepp/core.hpp>
#include <eepp/system/time.hpp>
using namespace EE::System;
#include <eepp/math/vector2.hpp>
using namespace EE::Math;

namespace EE { namespace Window {
class Input;
class InputEvent;
class Window;
}}
using namespace EE::Window;

namespace EE { namespace Scene {

class Node;
class SceneNode;

class EventDispatcher {
	public:
		static EventDispatcher * New( SceneNode * sceneNode );

		EventDispatcher( SceneNode * sceneNode );

		virtual ~EventDispatcher();

		virtual void update( const Time& elapsed );

		Input * getInput() const;

		Node * getFocusControl() const;

		void setFocusControl( Node * Ctrl );

		Node * getOverControl() const;

		void setOverControl( Node * Ctrl );

		Node * getDownControl() const;

		Node * getLossFocusControl() const;

		void sendMsg( Node * Ctrl, const Uint32& Msg, const Uint32& Flags = 0 );

		void sendKeyUp( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod );

		void sendKeyDown( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod );

		void sendMouseClick( Node * ToCtrl, const Vector2i& Pos, const Uint32 Flags );

		void sendMouseUp( Node * ToCtrl, const Vector2i& Pos, const Uint32 Flags );

		void sendMouseDown( Node * ToCtrl, const Vector2i& Pos, const Uint32 Flags );

		const Uint32& getPressTrigger() const;

		const Uint32& getLastPressTrigger() const;

		const Uint32& getClickTrigger() const;

		const Uint32 & getDoubleClickTrigger() const;

		void setControlDragging( bool dragging );

		const bool& isControlDragging() const;

		Vector2i getMousePos();

		Vector2f getMousePosf();

		SceneNode * getSceneNode() const;
	protected:
		EE::Window::Window *mWindow;
		Input *				mInput;
		SceneNode *			mControl;
		Node *				mFocusControl;
		Node *				mOverControl;
		Node *				mDownControl;
		Node *				mLossFocusControl;
		Vector2f			mMousePos;
		Vector2i			mMousePosi;
		Vector2f			mLastMousePos;
		Vector2i			mMouseDownPos;
		Int32 				mCbId;
		bool 				mFirstPress;
		bool				mControlDragging;

		virtual void inputCallback( InputEvent * Event );
};

}}

#endif

