#ifndef EE_EVENTDISPATCHER_HPP
#define EE_EVENTDISPATCHER_HPP

#include <eepp/core.hpp>
#include <eepp/math/vector2.hpp>
#include <eepp/system/time.hpp>
#include <eepp/window/keycodes.hpp>
#include <vector>

using namespace EE::System;
using namespace EE::Math;

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
	typedef std::function<void( const Uint32& cbId, Node* focus, Node* focusLoss )> FocusCallback;

	static EventDispatcher* New( SceneNode* sceneNode );

	EventDispatcher( SceneNode* sceneNode );

	virtual ~EventDispatcher();

	virtual void update( const Time& elapsed );

	Input* getInput() const;

	Node* getFocusNode() const;

	void setFocusNode( Node* node );

	Node* getMouseOverNode() const;

	void setMouseOverNode( Node* node );

	Node* getMouseDownNode() const;

	void resetMouseDownNode();

	Node* getLossFocusNode() const;

	void sendMsg( Node* node, const Uint32& Msg, const Uint32& Flags = 0 );

	void sendTextInput( const Uint32& textChar, const Uint32& timestamp );

	void sendKeyUp( const Keycode& keyCode, const Scancode& scancode, const Uint32& chr,
					const Uint32& mod );

	void sendKeyDown( const Keycode& keyCode, const Scancode& scancode, const Uint32& chr,
					  const Uint32& mod );

	void sendMouseClick( Node* toNode, const Vector2i& pos, const Uint32 flags );

	void sendMouseUp( Node* toNode, const Vector2i& pos, const Uint32 flags );

	void sendMouseDown( Node* toNode, const Vector2i& pos, const Uint32 flags );

	const Uint32& getPressTrigger() const;

	const Uint32& getLastPressTrigger() const;

	const Uint32& getClickTrigger() const;

	const Uint32& getDoubleClickTrigger() const;

	const Uint32& getReleaseTrigger() const;

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

	bool getDisableMousePress() const;

	void setDisableMousePress( bool disableMousePress );

	Uint32 addFocusEventCallback( const FocusCallback& cb );

	bool removeFocusEventCallback( const Uint32& cbId );

	Node* getLastFocusNode() const;

	void setLastFocusNode( Node* lastFocusNode );

  protected:
	EE::Window::Window* mWindow;
	Input* mInput;
	SceneNode* mSceneNode;
	Node* mFocusNode;
	Node* mLastFocusNode;
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
	bool mDisableMousePress{ false };
	bool mJustDisabledMousePress{ false };
	Node* mNodeWasDragging;
	Node* mNodeDragging;
	Time mElapsed;
	Uint32 mCurFocusId{ 0 };
	std::map<Uint32, FocusCallback> mFocusCbs;

	virtual void inputCallback( InputEvent* event );
};

}} // namespace EE::Scene

#endif
