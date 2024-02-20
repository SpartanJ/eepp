#ifndef EE_SCENE_NODE_HPP
#define EE_SCENE_NODE_HPP

#include <eepp/scene/actions/runnable.hpp>
#include <eepp/scene/event.hpp>
#include <eepp/scene/eventdispatcher.hpp>
#include <eepp/scene/keyevent.hpp>
#include <eepp/scene/mouseevent.hpp>
#include <eepp/scene/nodemessage.hpp>

#include <eepp/graphics/blendmode.hpp>
using namespace EE::Graphics;

#include <eepp/math/math.hpp>
#include <eepp/math/originpoint.hpp>
#include <eepp/math/polygon2.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/math/transformable.hpp>
#include <eepp/math/vector2.hpp>
using namespace EE::Math;

#include <eepp/system/color.hpp>
using namespace EE::System;

namespace EE { namespace Scene {
class Action;
class ActionManager;
class SceneNode;
}} // namespace EE::Scene
using namespace EE::Scene;

namespace EE { namespace Scene {

enum NodeFlags {
	NODE_FLAG_SCHEDULED_UPDATE = ( 1 << 0 ),
	NODE_FLAG_VIEW_DIRTY = ( 1 << 1 ),
	NODE_FLAG_POSITION_DIRTY = ( 1 << 2 ),
	NODE_FLAG_POLYGON_DIRTY = ( 1 << 3 ),
	NODE_FLAG_ROTATED = ( 1 << 4 ),
	NODE_FLAG_SCALED = ( 1 << 5 ),
	NODE_FLAG_CLOSE = ( 1 << 6 ),
	NODE_FLAG_MOUSEOVER = ( 1 << 7 ),
	NODE_FLAG_HAS_FOCUS = ( 1 << 8 ),
	NODE_FLAG_SELECTED = ( 1 << 9 ),
	NODE_FLAG_MOUSEOVER_ME_OR_CHILD = ( 1 << 10 ),
	NODE_FLAG_DRAGGING = ( 1 << 11 ),
	NODE_FLAG_SKIN_OWNER = ( 1 << 12 ),
	NODE_FLAG_TOUCH_DRAGGING = ( 1 << 13 ),
	NODE_FLAG_DROPPABLE_HOVERING = ( 1 << 14 ),
	NODE_FLAG_OWNED_BY_NODE = ( 1 << 15 ),
	NODE_FLAG_REVERSE_DRAW = ( 1 << 16 ),
	NODE_FLAG_FRAME_BUFFER = ( 1 << 17 ),
	NODE_FLAG_CLIP_ENABLE = ( 1 << 18 ),
	NODE_FLAG_REPORT_SIZE_CHANGE_TO_CHILDS = ( 1 << 19 ),
	NODE_FLAG_OVER_FIND_ALLOWED = ( 1 << 20 ),

	NODE_FLAG_SCENENODE = ( 1 << 21 ),
	NODE_FLAG_UISCENENODE = ( 1 << 22 ),
	NODE_FLAG_UINODE = ( 1 << 23 ),
	NODE_FLAG_WIDGET = ( 1 << 24 ),
	NODE_FLAG_WINDOW = ( 1 << 25 ),
	NODE_FLAG_LAYOUT = ( 1 << 26 ),

	NODE_FLAG_LOADING = ( 1 << 27 ),
	NODE_FLAG_CLOSING_CHILDREN = ( 1 << 28 ),
	NODE_FLAG_FREE_USE = ( 1 << 29 )
};

class EE_API Node : public Transformable {
  public:
	static Node* New();

	typedef std::function<void( const Event* )> EventCallback;

	Node();

	virtual ~Node();

	virtual void worldToNodeTranslation( Vector2f& position ) const;

	virtual void nodeToWorldTranslation( Vector2f& position ) const;

	virtual void worldToNode( Vector2i& pos ) const;

	virtual void nodeToWorld( Vector2i& pos ) const;

	virtual void worldToNode( Vector2f& pos ) const;

	virtual void nodeToWorld( Vector2f& pos ) const;

	virtual Uint32 getType() const;

	virtual bool isType( const Uint32& type ) const;

	void messagePost( const NodeMessage* Msg );

	virtual void setPosition( const Vector2f& Pos );

	virtual Node* setPosition( const Float& x, const Float& y );

	virtual Node* setSize( const Sizef& size );

	Node* setSize( const Float& Width, const Float& Height );

	virtual const Sizef& getSize() const;

	const Sizef& getPixelsSize() const;

	Node* setVisible( const bool& visible, bool emitEventNotification = true );

	Node* setChildsVisibility( bool visible, bool emitEventNotification = true );

	bool isVisible() const;

	bool isHided() const;

	Node* setEnabled( const bool& enabled );

	bool isEnabled() const;

	bool isDisabled() const;

	Node* getParent() const;

	Node* setParent( Node* parent );

	virtual void close();

	virtual void draw();

	virtual void update( const Time& time );

	virtual void scheduledUpdate( const Time& time );

	Node* getNextNode() const;

	Node* getPrevNode() const;

	Node* getNextNodeLoop() const;

	Node* setData( const UintPtr& data );

	const UintPtr& getData() const;

	Node* setBlendMode( const BlendMode& blend );

	const BlendMode& getBlendMode() const;

	Node* toFront();

	Node* toBack();

	void toPosition( const Uint32& position );

	const Uint32& getNodeFlags() const;

	/** Use it at your own risk */
	void setNodeFlags( const Uint32& flags );

	bool isSceneNode() const;

	bool isUISceneNode() const;

	bool isUINode() const;

	bool isWidget() const;

	bool isWindow() const;

	bool isLayout() const;

	bool isClipped() const;

	bool isRotated() const;

	bool isScaled() const;

	bool isFrameBuffer() const;

	bool isMouseOver() const;

	bool isMouseOverMeOrChilds() const;

	bool isMeOrParentTreeVisible() const;

	bool isMeOrParentTreeRotated() const;

	bool isMeOrParentTreeScaled() const;

	bool isMeOrParentTreeScaledOrRotated() const;

	bool isMeOrParentTreeScaledOrRotatedOrFrameBuffer() const;

	Uint32 addEventListener( const Uint32& eventType, const EventCallback& callback );

	Uint32 on( const Uint32& eventType, const EventCallback& callback );

	Uint32 onClick( const std::function<void( const MouseEvent* )>& callback,
					const MouseButton& button = MouseButton::EE_BUTTON_LEFT );

	void removeEventsOfType( const Uint32& eventType );

	void removeEventListener( const Uint32& callbackId );

	void removeEventListener( const std::vector<Uint32>& callbacksIds );

	void clearEventListener();

	Node* getFirstChild() const;

	Node* getLastChild() const;

	const Polygon2f& getWorldPolygon();

	const Rectf& getWorldBounds();

	bool isParentOf( const Node* node ) const;

	void sendEvent( const Event* Event );

	void sendMouseEvent( const Uint32& Event, const Vector2i& position, const Uint32& flags );

	void sendCommonEvent( const Uint32& Event );

	void sendTextEvent( const Uint32& event, const std::string& text );

	void childsCloseAll();

	const std::string& getId() const;

	virtual Node* setId( const std::string& id );

	const String::HashType& getIdHash() const;

	Node* find( const std::string& id ) const;

	Node* hasChild( const std::string& id ) const;

	template <typename T> T* find( const std::string& id ) const {
		return reinterpret_cast<T*>( find( id ) );
	}

	template <typename T> T* bind( const std::string& id, T*& node ) {
		node = find<T>( id );
		return node;
	}

	template <typename T> T* asType() { return reinterpret_cast<T*>( this ); }

	template <typename T> const T* asConstType() const {
		return reinterpret_cast<const T*>( this );
	}

	Node* findByType( const Uint32& type ) const;

	template <typename T> T* findByType( const Uint32& type ) const {
		return reinterpret_cast<T*>( findByType( type ) );
	}

	template <typename T> T* bindByType( const Uint32& type, T*& node ) {
		node = findByType<T>( type );
		return node;
	}

	std::vector<Node*> findAllByType( const Uint32& type ) const;

	template <typename T> std::vector<T*> findAllByType( const Uint32& type ) const {
		std::vector<T*> casted;
		auto all( findAllByType( type ) );
		casted.reserve( all.size() );
		for ( auto* node : all )
			casted.push_back( reinterpret_cast<T*>( node ) );
		return casted;
	}

	bool inNodeTree( Node* node ) const;

	bool isReverseDraw() const;

	void setReverseDraw( bool reverseDraw );

	void invalidateDraw();

	void setRotation( float angle );

	void setRotation( const Float& angle, const OriginPoint& center );

	const OriginPoint& getRotationOriginPoint() const;

	void setRotationOriginPoint( const OriginPoint& center );

	void setRotationOriginPointX( const std::string& xEq );

	void setRotationOriginPointY( const std::string& yEq );

	Vector2f getRotationCenter() const;

	void setScale( const Vector2f& scale );

	void setScale( const Vector2f& scale, const OriginPoint& center );

	void setScale( const Float& scale, const OriginPoint& center = OriginPoint::OriginCenter );

	const OriginPoint& getScaleOriginPoint() const;

	void setScaleOriginPoint( const OriginPoint& center );

	void setScaleOriginPointX( const std::string& xEq );

	void setScaleOriginPointY( const std::string& yEq );

	Vector2f getScaleCenter() const;

	virtual void setScale( float factorX, float factorY );

	virtual void setScaleOrigin( float x, float y );

	virtual void setRotationOrigin( float x, float y );

	const Float& getAlpha() const;

	virtual void setAlpha( const Float& alpha );

	virtual void setChildsAlpha( const Float& alpha );

	ActionManager* getActionManager() const;

	Node* runAction( Action* action );

	bool removeAction( Action* action );

	bool removeActions( const std::vector<Action*>& actions );

	bool removeActionsByTag( const String::HashType& tag );

	std::vector<Action*> getActions();

	std::vector<Action*> getActionsByTag( const Uint32& tag );

	void clearActions();

	Transform getLocalTransform() const;

	Transform getGlobalTransform() const;

	Transform getNodeToWorldTransform() const;

	Transform getWorldToNodeTransform() const;

	Vector2f convertToNodeSpace( const Vector2f& worldPoint ) const;

	Vector2f convertToWorldSpace( const Vector2f& nodePoint ) const;

	Rectf getLocalBounds() const;

	bool hasFocus() const;

	bool hasFocusWithin() const;

	virtual Node* setFocus();

	Node* getFirstWidget() const;

	Node* getParentWidget() const;

	void enableReportSizeChangeToChilds();

	void disableReportSizeChangeToChilds();

	bool reportSizeChangeToChilds() const;

	Node* centerHorizontal();

	Node* centerVertical();

	Node* center();

	Node* clipEnable();

	Node* clipDisable();

	void writeNodeFlag( const Uint32& Flag, const Uint32& Val );

	SceneNode* getSceneNode() const;

	EventDispatcher* getEventDispatcher() const;

	virtual bool isDrawInvalidator() const;

	bool invalidated() const;

	virtual void invalidate( Node* invalidator );

	Uint32 getChildCount() const;

	Uint32 getChildOfTypeCount( const Uint32& type ) const;

	Node* getChildAt( Uint32 Index ) const;

	Uint32 getNodeIndex() const;

	Uint32 getNodeOfTypeIndex() const;

	void runOnMainThread( Actions::Runnable::RunnableFunc runnable,
						  const Time& delay = Seconds( 0 ), const Uint32& uniqueIdentifier = 0 );

	void setTimeout( Actions::Runnable::RunnableFunc runnable, const Time& delay = Seconds( 0 ),
					 const Uint32& uniqueIdentifier = 0 );

	void debounce( Actions::Runnable::RunnableFunc runnable, const Time& delay,
				   const Uint32& uniqueIdentifier );

	void setInterval( Actions::Runnable::RunnableFunc runnable, const Time& interval,
					  const Uint32& uniqueIdentifier = 0 );

	bool isChild( Node* child ) const;

	bool inParentTreeOf( Node* child ) const;

	void setLoadingState( bool loading );

	bool isLoadingState() const;

	virtual void onIdChange();

	bool isClosing() const;

	bool isClosingChildren() const;

	virtual Node* overFind( const Vector2f& Point );

	/** This removes the node from its parent. Never use this unless you know what you are doing. */
	void detach();

	void forEachNode( std::function<void( Node* )> func );

	void forEachChild( std::function<void( Node* )> func );

	virtual void nodeDraw();

	Uint32 forceKeyDown( const KeyEvent& event );

	Uint32 foceKeyUp( const KeyEvent& event );

	Uint32 forceTextInput( const TextInputEvent& Event );

	const Vector2f& getScreenPos() const;

	Rectf getScreenRect() const;

	bool hasEventsOfType( const Uint32& eventType ) const;

  protected:
	typedef UnorderedMap<Uint32, std::map<Uint32, EventCallback>> EventsMap;
	friend class EventDispatcher;

	std::string mId;
	String::HashType mIdHash;
	Vector2f mScreenPos;
	Vector2i mScreenPosi;
	Sizef mSize;
	UintPtr mData;
	Node* mParentNode;
	SceneNode* mSceneNode;
	Node* mNodeDrawInvalidator;
	Node* mChild;	  //! Pointer to the first child of the node
	Node* mChildLast; //! Pointer to the last child added
	Node* mNext;	  //! Pointer to the next child of the father
	Node* mPrev;	  //! Pointer to the prev child of the father
	Uint32 mNodeFlags;
	BlendMode mBlend;
	Uint32 mNumCallBacks;

	mutable Polygon2f mPoly;
	mutable Rectf mWorldBounds;
	Vector2f mCenter;

	EventsMap mEvents;

	bool mVisible;
	bool mEnabled;

	OriginPoint mRotationOriginPoint;
	OriginPoint mScaleOriginPoint;
	Float mAlpha;

	virtual Uint32 onMessage( const NodeMessage* msg );

	virtual Uint32 onTextInput( const TextInputEvent& event );

	virtual Uint32 onTextEditing( const TextEditingEvent& event );

	virtual Uint32 onKeyDown( const KeyEvent& event );

	virtual Uint32 onKeyUp( const KeyEvent& event );

	virtual Uint32 onMouseMove( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseDown( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseClick( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseDoubleClick( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseUp( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseOver( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseLeave( const Vector2i& position, const Uint32& flags );

	virtual Uint32 onMouseWheel( const Vector2f& offset, bool flipped );

	virtual Uint32 onCalculateDrag( const Vector2f& position, const Uint32& flags );

	void onClose();

	virtual void onVisibilityChange();

	virtual void onEnabledChange();

	virtual void onPositionChange();

	virtual void onSizeChange();

	virtual void onParentSizeChange( const Vector2f& SizeChange );

	virtual void onParentChange();

	void updateWorldPolygon();

	void updateCenter();

	virtual void matrixSet();

	virtual void matrixUnset();

	virtual void drawChilds();

	virtual void onChildCountChange( Node* child, const bool& removed );

	virtual void onAngleChange();

	virtual void onScaleChange();

	virtual void onAlphaChange();

	virtual void onSceneChange();

	virtual Uint32 onFocus();

	virtual Uint32 onFocusLoss();

	void clipStart( bool needsClipPlanes );

	void clipEnd( bool needsClipPlanes );

	void updateScreenPos();

	virtual void setInternalSize( const Sizef& size );

	void checkClose();

	void sendParentSizeChange( const Vector2f& sizeChange );

	void childDeleteAll();

	void childAdd( Node* node );

	void childAddAt( Node* node, Uint32 index );

	void childRemove( Node* node );

	Rectf getScreenBounds();

	void setInternalPosition( const Vector2f& Pos );

	void setInternalWidth( const Float& width );

	void setInternalHeight( const Float& height );

	Color getColor( const Color& Col );

	Node* findIdHash( const String::HashType& idHash ) const;

	Node* hasChildHash( const String::HashType& idHash ) const;

	virtual void updateOriginPoint();

	void setDirty();

	void setChildsDirty();

	void clipSmartEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height,
						  bool needsClipPlanes );

	void clipSmartDisable( bool needsClipPlanes );

	void clipSmartEnable( const Int32& x, const Int32& y, const Uint32& Width,
						  const Uint32& Height );

	void clipSmartDisable();

	Node* getDrawInvalidator();

	SceneNode* findSceneNode();

	void updateDrawInvalidator( bool force = false );

	void subscribeScheduledUpdate();

	void unsubscribeScheduledUpdate();

	bool isSubscribedForScheduledUpdate();
};

}} // namespace EE::Scene

#endif
