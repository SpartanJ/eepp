#ifndef EE_SCENE_NODE_HPP
#define EE_SCENE_NODE_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/uimessage.hpp>
#include <eepp/ui/uievent.hpp>
#include <eepp/ui/uieventkey.hpp>
#include <eepp/ui/uieventmouse.hpp>
#include <eepp/math/transformable.hpp>
#include <eepp/graphics/blendmode.hpp>

using namespace EE::UI;

namespace EE { namespace Scene {
class Action;
class ActionManager;
}}
using namespace EE::Scene;

namespace  EE { namespace UI {
class UIWindow;
class UIManager;
}}

namespace EE { namespace Scene {

class EE_API Node : public Transformable {
	public:
		static Node * New();

		typedef cb::Callback1<void, const UIEvent*> UIEventCallback;

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

		void messagePost( const UIMessage * Msg );

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

		Uint32 addEventListener( const Uint32& EventType, const UIEventCallback& Callback );

		void removeEventListener( const Uint32& CallbackId );

		Node * getFirstChild() const;

		Node * getLastChild() const;

		const Polygon2f& getWorldPolygon();

		const Rectf& getWorldBounds();

		bool isParentOf( Node * Ctrl );

		void sendEvent( const UIEvent * Event );

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

		virtual void setClipEnabled();

		virtual void setClipDisabled();

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

		UIWindow * getOwnerWindow();

		bool hasFocus() const;

		virtual void setFocus();

		Node * getNextWidget();

		void enableReportSizeChangeToChilds();

		void disableReportSizeChangeToChilds();

		bool reportSizeChangeToChilds();

		void centerHorizontal();

		void centerVertical();

		void center();
	protected:
		typedef std::map< Uint32, std::map<Uint32, UIEventCallback> > UIEventsMap;
		friend class EE::UI::UIManager;
		friend class EE::UI::UIWindow;

		std::string		mId;
		Uint32			mIdHash;
		Vector2f		mScreenPos;
		Vector2i		mScreenPosi;
		Sizef			mSize;
		UintPtr			mData;
		Node *			mParentCtrl;
		UIWindow *		mParentWindowCtrl;
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

		UIEventsMap		mEvents;

		bool			mVisible;
		bool			mEnabled;

		OriginPoint			mRotationOriginPoint;
		OriginPoint			mScaleOriginPoint;
		Float				mAlpha;

		ActionManager *		mActionManager;

		virtual Uint32 onMessage( const UIMessage * Msg );

		virtual Uint32 onKeyDown( const UIEventKey& Event );

		virtual Uint32 onKeyUp( const UIEventKey& Event );

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

		virtual void onParentWindowChange();

		virtual void clipMe();

		virtual Uint32 onFocus();

		virtual Uint32 onFocusLoss();

		virtual void internalDraw();

		virtual void clipDisable();

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

		void writeCtrlFlag( const Uint32& Flag, const Uint32& Val );

		Rectf getScreenBounds();

		void setInternalPosition( const Vector2f& Pos );

		void setInternalWidth(const Float& width );

		void setInternalHeight( const Float& height );

		Color getColor( const Color& Col );

		Node * findIdHash( const Uint32& idHash );

		UIWindow * getParentWindow();

		void updateOriginPoint();

		void setDirty();

		void setChildsDirty();
};

}}

#endif
