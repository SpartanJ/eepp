#ifndef EE_SCENENODE_HPP
#define EE_SCENENODE_HPP

#include <eepp/scene/node.hpp>
#include <eepp/system/translator.hpp>
#include <eepp/window/cursor.hpp>
#include <unordered_set>

namespace EE { namespace Graphics {
class FrameBuffer;
}} // namespace EE::Graphics
using namespace EE::Graphics;

namespace EE { namespace Window {
class Window;
}} // namespace EE::Window

namespace EE { namespace Scene {

class EE_API SceneNode : public Node {
  public:
	static SceneNode* New( EE::Window::Window* window = NULL );

	SceneNode( EE::Window::Window* window = NULL );

	~SceneNode();

	void setTranslator( Translator translator );

	Translator& getTranslator();

	String getTranslatorString( const std::string& str );

	void enableFrameBuffer();

	void disableFrameBuffer();

	bool ownsFrameBuffer() const;

	virtual void draw();

	virtual void update( const Time& elapsed );

	void enableDrawInvalidation();

	void disableDrawInvalidation();

	EE::Window::Window* getWindow();

	FrameBuffer* getFrameBuffer() const;

	void setEventDispatcher( EventDispatcher* eventDispatcher );

	EventDispatcher* getEventDispatcher() const;

	void setDrawDebugData( bool debug );

	bool getDrawDebugData() const;

	void setDrawBoxes( bool draw );

	bool getDrawBoxes() const;

	void setHighlightOver( bool Highlight );

	bool getHighlightOver() const;

	void setHighlightFocus( bool Highlight );

	bool getHighlightFocus() const;

	void setHighlightInvalidation( bool Highlight );

	bool getHighlightInvalidation() const;

	void setHighlightOverColor( const Color& Color );

	const Color& getHighlightOverColor() const;

	void setHighlightFocusColor( const Color& Color );

	const Color& getHighlightFocusColor() const;

	void setHighlightInvalidationColor( const Color& Color );

	const Color& getHighlightInvalidationColor() const;

	const Time& getElapsed() const;

	bool usesInvalidation();

	void setUseGlobalCursors( const bool& use );

	const bool& getUseGlobalCursors();

	void setCursor( Cursor::Type cursor );

	virtual bool isDrawInvalidator() const;

	ActionManager* getActionManager() const;

	void subscribeScheduledUpdate( Node* node );

	void unsubscribeScheduledUpdate( Node* node );

	bool isSubscribedForScheduledUpdate( Node* node );

	void addMouseOverNode( Node* node );

	void removeMouseOverNode( Node* node );

	const bool& getUpdateAllChilds() const;

	void setUpdateAllChilds( const bool& updateAllChilds );

	const Float& getDPI() const;

  protected:
	friend class Node;
	typedef std::unordered_set<Node*> CloseList;

	EE::Window::Window* mWindow;
	ActionManager* mActionManager;
	FrameBuffer* mFrameBuffer;
	EventDispatcher* mEventDispatcher;
	CloseList mCloseList;
	bool mFrameBufferBound;
	bool mUseInvalidation;
	bool mUseGlobalCursors;
	bool mUpdateAllChilds;
	Int32 mResizeCb;
	bool mDrawDebugData;
	bool mDrawBoxes;
	bool mHighlightOver;
	bool mHighlightFocus;
	bool mHighlightInvalidation;
	Color mHighlightFocusColor;
	Color mHighlightOverColor;
	Color mHighlightInvalidationColor;
	Time mElapsed;
	std::unordered_set<Node*> mScheduledUpdate;
	std::unordered_set<Node*> mScheduledUpdateRemove;
	std::unordered_set<Node*> mMouseOverNodes;
	Float mDPI;

	virtual void onSizeChange();

	virtual void matrixSet();

	virtual void matrixUnset();

	virtual void preDraw();

	virtual void postDraw();

	void sendMsg( Node* Ctrl, const Uint32& Msg, const Uint32& Flags = 0 );

	virtual void resizeControl( EE::Window::Window* win );

	void addToCloseQueue( Node* node );

	void checkClose();

	void createFrameBuffer();

	void drawFrameBuffer();

	Sizei getFrameBufferSize();
};

}} // namespace EE::Scene

#endif
