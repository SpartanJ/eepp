#ifndef EE_EVENTDISPATCHER_HPP
#define EE_EVENTDISPATCHER_HPP

#include <eepp/core.hpp>
#include <eepp/system/time.hpp>
using namespace EE::System;
#include <eepp/math/vector2.hpp>
using namespace EE::Math;
#include <vector>

namespace EE { namespace Window {
class Input;
class InputEvent;
class Window;
}} // namespace EE::Window
using namespace EE::Window;

namespace EE { namespace Scene {

class Node;
class SceneNode;

class EE_API EventDispatcher {
  public:
	static EventDispatcher* New( SceneNode* sceneNode );

	EventDispatcher( SceneNode* sceneNode );

	virtual ~EventDispatcher();

	virtual void update( const Time& elapsed );

	Input* getInput() const;

	Node* getFocusNode() const;

	void setFocusNode( Node* Ctrl );

	Node* getMouseOverNode() const;

	void setMouseOverNode( Node* Ctrl );

	Node* getMouseDownNode() const;

	Node* getLossFocusNode() const;

	void sendMsg( Node* Ctrl, const Uint32& Msg, const Uint32& Flags = 0 );

	void sendTextInput( const Uint32& textChar, const Uint32& timestamp );

	void sendKeyUp( const Uint32& KeyCode, const Uint32& Char, const Uint32& Mod );

	void sendKeyDown( const Uint32& KeyCode, const Uint32& Char, const Uint32& Mod );

	void sendMouseClick( Node* ToCtrl, const Vector2i& Pos, const Uint32 Flags );

	void sendMouseUp( Node* ToCtrl, const Vector2i& Pos, const Uint32 Flags );

	void sendMouseDown( Node* ToCtrl, const Vector2i& Pos, const Uint32 Flags );

	const Uint32& getPressTrigger() const;

	const Uint32& getLastPressTrigger() const;

	const Uint32& getClickTrigger() const;

	const Uint32& getDoubleClickTrigger() const;

	void setNodeDragging( Node* dragging );

	bool isNodeDragging() const;

	bool wasNodeDragging() const;

	bool isOrWasNodeDragging() const;

	Vector2i getMousePos();

	Vector2f getMousePosf();

	Vector2i getMouseDownPos();

	Vector2f getLastMousePos();

	SceneNode* getSceneNode() const;

	const Time& getLastFrameTime() const;

	Node* getNodeDragging() const;

	Node* getNodeWasDragging() const;

  protected:
	EE::Window::Window* mWindow;
	Input* mInput;
	SceneNode* mSceneNode;
	Node* mFocusNode;
	Node* mOverNode;
	Node* mDownNode;
	Node* mLossFocusNode;
	Vector2f mMousePos;
	Vector2i mMousePosi;
	Vector2f mLastMousePos;
	Vector2i mMouseDownPos;
	Vector2i mClickPos;
	Int32 mCbId;
	bool mFirstPress;
	Node* mNodeWasDragging;
	Node* mNodeDragging;
	Time mElapsed;

	virtual void inputCallback( InputEvent* event );
};

}} // namespace EE::Scene

#endif
