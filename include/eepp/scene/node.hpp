#ifndef EE_SCENE_NODE_HPP
#define EE_SCENE_NODE_HPP

#include <eepp/scene/nodemessage.hpp>
#include <eepp/scene/event.hpp>
#include <eepp/scene/keyevent.hpp>
#include <eepp/scene/mouseevent.hpp>
#include <eepp/scene/eventdispatcher.hpp>

#include <eepp/graphics/blendmode.hpp>
using namespace EE::Graphics;

#include <eepp/math/transformable.hpp>
#include <eepp/math/rect.hpp>
#include <eepp/math/vector2.hpp>
#include <eepp/math/polygon2.hpp>
#include <eepp/math/math.hpp>
#include <eepp/math/originpoint.hpp>
using namespace EE::Math;

namespace EE { namespace Scene {
class Action;
class ActionManager;
class SceneNode;
class NodeDrawInvalidator;
}}
using namespace EE::Scene;

namespace EE { namespace Scene {

enum NODE_FLAGS_VALUES {
	NODE_FLAG_VIEW_DIRTY						= (1<<1),
	NODE_FLAG_POSITION_DIRTY					= (1<<2),
	NODE_FLAG_POLYGON_DIRTY						= (1<<3),
	NODE_FLAG_ROTATED							= (1<<4),
	NODE_FLAG_SCALED							= (1<<5),
	NODE_FLAG_CLOSE								= (1<<6),
	NODE_FLAG_CLOSE_DELAYED						= (1<<7),
	NODE_FLAG_MOUSEOVER							= (1<<8),
	NODE_FLAG_HAS_FOCUS							= (1<<9),
	NODE_FLAG_SELECTED							= (1<<10),
	NODE_FLAG_DISABLE_DELAYED					= (1<<11),
	NODE_FLAG_MOUSEOVER_ME_OR_CHILD				= (1<<12),
	NODE_FLAG_DRAGGING							= (1<<13),
	NODE_FLAG_SKIN_OWNER						= (1<<14),
	NODE_FLAG_TOUCH_DRAGGING					= (1<<15),
	NODE_FLAG_DISABLED_BY_NODE					= (1<<16),
	NODE_FLAG_OWNED_BY_NODE						= (1<<17),
	NODE_FLAG_REVERSE_DRAW						= (1<<18),
	NODE_FLAG_FRAME_BUFFER						= (1<<19),
	NODE_FLAG_CLIP_ENABLE						= (1<<20),
	NODE_FLAG_REPORT_SIZE_CHANGE_TO_CHILDS		= (1<<21),
	NODE_FLAG_OVER_FIND_ALLOWED					= (1<<22),

	NODE_FLAG_SCENENODE							= (1<<23),
	NODE_FLAG_UISCENENODE						= (1<<24),
	NODE_FLAG_UINODE							= (1<<25),
	NODE_FLAG_WIDGET							= (1<<26),
	NODE_FLAG_WINDOW							= (1<<27),

	NODE_FLAG_FREE_USE							= (1<<28)
};

class EE_API Node : public Transformable {
	public:
		static Node * New();

		typedef cb::Callback1<void, const Event*> EventCallback;

		Node();

		virtual ~Node();

		virtual void worldToNodeTranslation( Vector2f& position ) const;

		virtual void nodeToWorldTranslation( Vector2f& position ) const;

		virtual void worldToNode( Vector2i& pos );

		virtual void nodeToWorld( Vector2i& pos );

		virtual void worldToNode( Vector2f& pos );

		virtual void nodeToWorld( Vector2f& pos );

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		void messagePost( const NodeMessage * Msg );

		virtual void setPosition( const Vector2f& Pos );

		virtual Node * setPosition( const Float& x, const Float& y );

		virtual Node * setSize( const Sizef& size );

		Node * setSize( const Float& Width, const Float& Height );

		virtual const Sizef& getSize();

		virtual const Sizef& getRealSize();

		Node * setVisible( const bool& visible );

		bool isVisible() const;

		bool isHided() const;

		Node * setEnabled( const bool& enabled );

		bool isEnabled() const;

		bool isDisabled() const;

		Node * getParent() const;

		Node * setParent( Node * parent );

		virtual void close();

		virtual void draw();

		virtual void update( const Time& time );

		Node * getNextNode() const;

		Node * getPrevNode() const;

		Node * getNextNodeLoop() const;

		Node * setData( const UintPtr& data );

		const UintPtr& getData() const;

		Node * setBlendMode( const BlendMode& blend );

		BlendMode getBlendMode();

		void toFront();

		void toBack();

		void toPosition( const Uint32& position );

		const Uint32& getNodeFlags() const;

		/** Use it at your own risk */
		void setNodeFlags( const Uint32& flags );

		Uint32 isSceneNode();

		Uint32 isUISceneNode();

		Uint32 isUINode();

		Uint32 isWidget();

		Uint32 isWindow();

		Uint32 isClipped();

		Uint32 isRotated();

		Uint32 isScaled();

		Uint32 isFrameBuffer();

		bool isMouseOver();

		bool isMouseOverMeOrChilds();

		bool isMeOrParentTreeRotated();

		bool isMeOrParentTreeScaled();

		bool isMeOrParentTreeScaledOrRotated();

		bool isMeOrParentTreeScaledOrRotatedOrFrameBuffer();

		Uint32 addEventListener( const Uint32& EventType, const EventCallback& Callback );

		void removeEventListener( const Uint32& CallbackId );

		Node * getFirstChild() const;

		Node * getLastChild() const;

		const Polygon2f& getWorldPolygon();

		const Rectf& getWorldBounds();

		bool isParentOf( Node * Ctrl );

		void sendEvent( const Event * Event );

		void sendMouseEvent( const Uint32& Event, const Vector2i& position, const Uint32& flags );

		void sendCommonEvent( const Uint32& Event );

		void childsCloseAll();

		std::string getId() const;

		Node * setId( const std::string & id );

		Uint32 getIdHash() const;

		Node * find( const std::string& id );

		template<typename T>
		T * find( const std::string& id )
		{
			return reinterpret_cast<T*>( find( id ) );
		}

		template<typename T>
		T * bind( const std::string& id, T*& ctrl )
		{
			ctrl = find<T>( id );
			return ctrl;
		}

		bool isReverseDraw() const;

		void setReverseDraw( bool reverseDraw );

		void invalidateDraw();

		void setRotation( float angle );

		void setRotation( const Float& angle, const OriginPoint& center );

		const OriginPoint& getRotationOriginPoint() const;

		void setRotationOriginPoint( const OriginPoint& center );

		Vector2f getRotationCenter();

		void setScale( const Vector2f& scale );

		void setScale( const Vector2f& scale, const OriginPoint& center );

		void setScale( const Float& scale , const OriginPoint & center = OriginPoint::OriginCenter );

		const OriginPoint& getScaleOriginPoint() const;

		void setScaleOriginPoint( const OriginPoint& center );

		Vector2f getScaleCenter();

		virtual void setScale(float factorX, float factorY);

		virtual void setScaleOrigin(float x, float y);

		virtual void setRotationOrigin(float x, float y);

		const Float& getAlpha() const;

		virtual void setAlpha( const Float& alpha );

		virtual void setChildsAlpha( const Float& alpha );

		ActionManager * getActionManager();

		void runAction( Action * action );

		Transform getLocalTransform();

		Transform getGlobalTransform();

		Transform getNodeToWorldTransform();

		Transform getWorldToNodeTransform();

		Vector2f convertToNodeSpace(const Vector2f& worldPoint);

		Vector2f convertToWorldSpace(const Vector2f& nodePoint);

		Rectf getLocalBounds();

		bool hasFocus() const;

		virtual void setFocus();

		Node * getNextWidget();

		void enableReportSizeChangeToChilds();

		void disableReportSizeChangeToChilds();

		bool reportSizeChangeToChilds();

		void centerHorizontal();

		void centerVertical();

		void center();

		Node * clipEnable();

		Node * clipDisable();

		void writeCtrlFlag( const Uint32& Flag, const Uint32& Val );

		SceneNode * getSceneNode();

		EventDispatcher * getEventDispatcher();

		virtual bool isDrawInvalidator();

		virtual bool invalidated();

		virtual void invalidate();
	protected:
		typedef std::map< Uint32, std::map<Uint32, EventCallback> > EventsMap;
		friend class EE::Scene::EventDispatcher;

		std::string		mId;
		Uint32			mIdHash;
		Vector2f		mScreenPos;
		Vector2i		mScreenPosi;
		Sizef			mSize;
		UintPtr			mData;
		Node *			mParentCtrl;
		SceneNode *		mSceneNode;
		Node *			mNodeDrawInvalidator;
		Node *			mChild;			//! Pointer to the first child of the node
		Node *			mChildLast;		//! Pointer to the last child added
		Node *			mNext;			//! Pointer to the next child of the father
		Node *			mPrev;			//! Pointer to the prev child of the father
		Uint32			mNodeFlags;
		BlendMode		mBlend;
		Uint16			mNumCallBacks;

		mutable Polygon2f	mPoly;
		mutable Rectf	mWorldBounds;
		Vector2f 		mCenter;

		EventsMap		mEvents;

		bool			mVisible;
		bool			mEnabled;

		OriginPoint			mRotationOriginPoint;
		OriginPoint			mScaleOriginPoint;
		Float				mAlpha;

		ActionManager *		mActionManager;

		virtual Uint32 onMessage( const NodeMessage * Msg );

		virtual Uint32 onKeyDown( const KeyEvent& Event );

		virtual Uint32 onKeyUp( const KeyEvent& Event );

		virtual Uint32 onMouseMove( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseDown( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseClick( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseDoubleClick( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseUp( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseEnter( const Vector2i& position, const Uint32 flags );

		virtual Uint32 onMouseExit( const Vector2i& position, const Uint32 flags );

		virtual void onClose();

		virtual void onVisibilityChange();

		virtual void onEnabledChange();

		virtual void onPositionChange();

		virtual void onSizeChange();

		virtual void onParentSizeChange( const Vector2f& SizeChange );

		virtual void onParentChange();

		virtual void updateWorldPolygon();

		virtual void updateCenter();

		virtual void matrixSet();

		virtual void matrixUnset();

		virtual void drawChilds();

		virtual void onChildCountChange();

		virtual void onAngleChange();

		virtual void onScaleChange();

		virtual void onAlphaChange();

		virtual Node * overFind( const Vector2f& Point );

		virtual void onSceneChange();

		virtual void clipStart();

		virtual Uint32 onFocus();

		virtual Uint32 onFocusLoss();

		virtual void internalDraw();

		virtual void clipEnd();

		virtual void updateScreenPos();

		virtual void setInternalSize(const Sizef& size );

		void checkClose();

		void sendParentSizeChange( const Vector2f& SizeChange );

		void childDeleteAll();

		void childAdd( Node * ChildCtrl );

		void childAddAt( Node * ChildCtrl, Uint32 position );

		void childRemove( Node * ChildCtrl );

		bool isChild( Node * ChildCtrl ) const;

		bool inParentTreeOf( Node * Child ) const;

		Uint32 childCount() const;

		Node * childAt( Uint32 Index ) const;

		Node * childPrev( Node * Ctrl, bool Loop = false ) const;

		Node * childNext( Node * Ctrl, bool Loop = false ) const;

		Rectf getScreenBounds();

		void setInternalPosition( const Vector2f& Pos );

		void setInternalWidth(const Float& width );

		void setInternalHeight( const Float& height );

		Color getColor( const Color& Col );

		Node * findIdHash( const Uint32& idHash );

		void updateOriginPoint();

		void setDirty();

		void setChildsDirty();

		void clipSmartEnable( const Int32 & x, const Int32 & y, const Uint32 & Width, const Uint32 & Height );

		void clipSmartDisable();

		Node * getDrawInvalidator();

		SceneNode * findSceneNode();
};

}}

#endif
