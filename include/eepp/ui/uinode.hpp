#ifndef EE_UIUINODE_HPP
#define EE_UIUINODE_HPP

#include <eepp/ui/base.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/uibackground.hpp>
#include <eepp/ui/uiborder.hpp>
#include <eepp/ui/uimessage.hpp>
#include <eepp/ui/uievent.hpp>
#include <eepp/ui/uieventkey.hpp>
#include <eepp/ui/uieventmouse.hpp>
#include <eepp/ui/uiskin.hpp>
#include <eepp/ui/uiskinstate.hpp>
#include <eepp/ui/uiskinsimple.hpp>
#include <eepp/ui/uiskincomplex.hpp>
#include <eepp/ui/uithememanager.hpp>
namespace EE { namespace UI {

class UITheme;
class UIWindow;
class UIManager;
class UIAction;
class UIActionManager;

class EE_API UINode {
	public:
		static UINode * New();

		typedef cb::Callback1<void, const UIEvent*> UIEventCallback;

		UINode();

		virtual ~UINode();

		void screenToNode( Vector2i& position ) const;

		void nodeToScreen( Vector2i& position ) const;

		void worldToNode( Vector2i& pos ) const;

		void nodeToWorld( Vector2i& pos ) const;

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void messagePost( const UIMessage * Msg );

		UINode * setPosition( const Vector2i& position );

		UINode * setPosition(const Vector2f & Pos);

		UINode * setPosition( const Int32& x, const Int32& y );

		void setPixelsPosition( const Vector2i& position );

		void setPixelsPosition( const Int32& x, const Int32& y );

		const Vector2i& getPosition() const;

		const Vector2i& getRealPosition() const;

		virtual UINode * setSize( const Sizei& size );

		UINode * setSize( const Int32& Width, const Int32& Height );

		void setPixelsSize( const Sizei& size );

		void setPixelsSize( const Int32& x, const Int32& y );

		const Sizei& getSize();

		const Sizei& getRealSize();

		Rect getRect() const;

		UINode * setVisible( const bool& visible );

		bool isVisible() const;

		bool isHided() const;

		UINode * setEnabled( const bool& enabled );

		bool isEnabled() const;

		bool isDisabled() const;

		UINode * getParent() const;

		UINode * setParent( UINode * parent );

		void centerHorizontal();

		void centerVertical();

		void center();

		virtual void close();

		virtual void draw();

		virtual void update( const Time& time );

		Uint32 getHorizontalAlign() const;

		UINode * setHorizontalAlign( Uint32 halign );

		Uint32 getVerticalAlign() const;

		UINode * setVerticalAlign( Uint32 valign );

		UINode * setGravity( Uint32 hvalign );

		UIBackground * setBackgroundFillEnabled( bool enabled );

		UIBorder * setBorderEnabled( bool enabled );

		UINode * getNextNode() const;

		UINode * getPrevNode() const;

		UINode * getNextNodeLoop() const;

		UINode * setData( const UintPtr& data );

		const UintPtr& getData() const;

		UINode * childGetAt( Vector2i CtrlPos, unsigned int RecursiveLevel = 0 );

		const Uint32& getFlags() const;

		virtual UINode * setFlags( const Uint32& flags );

		virtual UINode * unsetFlags( const Uint32& flags );

		virtual UINode * resetFlags( Uint32 newFlags = 0 );

		UINode * setBlendMode( const BlendMode& blend );

		BlendMode getBlendMode();

		void toFront();

		void toBack();

		void toPosition( const Uint32& position );

		const Uint32& getNodeFlags() const;

		/** Use it at your own risk */
		void setNodeFlags( const Uint32& flags );

		Uint32 isWidget();

		Uint32 isWindow();

		Uint32 isClipped();

		Uint32 isRotated();

		Uint32 isScaled();

		Uint32 isFrameBuffer();

		bool isMeOrParentTreeRotated();

		bool isMeOrParentTreeScaled();

		bool isMeOrParentTreeScaledOrRotated();

		bool isMeOrParentTreeScaledOrRotatedOrFrameBuffer();

		Uint32 addEventListener( const Uint32& EventType, const UIEventCallback& Callback );

		void removeEventListener( const Uint32& CallbackId );

		UIBackground * getBackground();

		UIBorder * getBorder();

		void setThemeByName( const std::string& Theme );

		virtual void setTheme( UITheme * Theme );

		virtual UINode * setThemeSkin( UITheme * Theme, const std::string& skinName );

		virtual UINode * setThemeSkin( const std::string& skinName );

		void setThemeToChilds( UITheme * Theme );

		UISkin * getSkin();

		virtual UINode * setSkin( const UISkin& Skin );

		UINode * setSkin( UISkin * skin );

		void removeSkin();

		UINode * getFirstChild() const;

		UINode * getLastChild() const;

		bool isMouseOver();

		bool isMouseOverMeOrChilds();

		Polygon2f& getPolygon();

		Vector2f getPolygonCenter();

		void setSkinState( const Uint32& State );

		bool hasFocus() const;

		virtual void setFocus();

		bool isParentOf( UINode * Ctrl );

		void sendEvent( const UIEvent * Event );

		void sendMouseEvent( const Uint32& Event, const Vector2i& position, const Uint32& flags );

		void sendCommonEvent( const Uint32& Event );

		Sizei getSkinSize();

		UINode * getNextWidget();

		void applyDefaultTheme();

		Rect getScreenRect();

		void childsCloseAll();

		std::string getId() const;

		UINode * setId( const std::string & id );

		Uint32 getIdHash() const;

		UINode * getWindowContainer();

		UINode * find( const std::string& id );

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

		UIWindow * getOwnerWindow();

		void invalidateDraw();

		void setClipEnabled();

		void setClipDisabled();

		bool isDragging() const;

		void setDragging( const bool& dragging );

		const Vector2i& getDragPoint() const;

		void setDragPoint( const Vector2i& Point );

		bool isDragEnabled() const;

		void setDragEnabled( const bool& enable );

		void setDragButton( const Uint32& Button );

		const Uint32& getDragButton() const;

		const Float& getRotation() const;

		void setRotation( const Float& angle );

		void setRotation( const Float& angle, const OriginPoint& center );

		const OriginPoint& getRotationOrigin() const;

		void setRotationOrigin( const OriginPoint& center );

		Vector2f getRotationOrigin();

		const Vector2f& getScale() const;

		void setScale( const Vector2f& scale );

		void setScale( const Vector2f& scale, const OriginPoint& center );

		void setScale( const Float& scale , const OriginPoint & center = OriginPoint::OriginCenter );

		const OriginPoint& getScaleOrigin() const;

		void setScaleOrigin( const OriginPoint& center );

		Vector2f getScaleOrigin();

		const Float& getAlpha() const;

		virtual void setAlpha( const Float& alpha );

		virtual void setChildsAlpha( const Float& alpha );

		bool isAnimating();

		Interpolation1d * startAlphaAnim( const Float& From, const Float& To, const Time& TotalTime, const bool& alphaChilds = true, const Ease::Interpolation& type = Ease::Linear, Interpolation1d::OnPathEndCallback PathEndCallback = Interpolation1d::OnPathEndCallback() );

		Interpolation2d * startScaleAnim( const Vector2f& From, const Vector2f& To, const Time& TotalTime, const Ease::Interpolation& type = Ease::Linear, Interpolation2d::OnPathEndCallback PathEndCallback = Interpolation2d::OnPathEndCallback() );

		Interpolation2d * startScaleAnim( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& type = Ease::Linear, Interpolation2d::OnPathEndCallback PathEndCallback = Interpolation2d::OnPathEndCallback() );

		Interpolation2d * startTranslation( const Vector2i& From, const Vector2i& To, const Time& TotalTime, const Ease::Interpolation& type = Ease::Linear, Interpolation2d::OnPathEndCallback PathEndCallback = Interpolation2d::OnPathEndCallback() );

		Interpolation1d * startRotation( const Float& From, const Float& To, const Time& TotalTime, const Ease::Interpolation& type = Ease::Linear, Interpolation1d::OnPathEndCallback PathEndCallback = Interpolation1d::OnPathEndCallback() );

		Interpolation1d * startAlphaAnim( const Float& To, const Time& TotalTime, const bool& alphaChilds = true, const Ease::Interpolation& type = Ease::Linear, Interpolation1d::OnPathEndCallback PathEndCallback = Interpolation1d::OnPathEndCallback() );

		Interpolation2d * startScaleAnim( const Vector2f& To, const Time& TotalTime, const Ease::Interpolation& type = Ease::Linear, Interpolation2d::OnPathEndCallback PathEndCallback = Interpolation2d::OnPathEndCallback() );

		Interpolation2d * startScaleAnim( const Float& To, const Time& TotalTime, const Ease::Interpolation& type = Ease::Linear, Interpolation2d::OnPathEndCallback PathEndCallback = Interpolation2d::OnPathEndCallback() );

		Interpolation2d * startTranslation( const Vector2i& To, const Time& TotalTime, const Ease::Interpolation& type = Ease::Linear, Interpolation2d::OnPathEndCallback PathEndCallback = Interpolation2d::OnPathEndCallback() );

		Interpolation1d * startRotation( const Float& To, const Time& TotalTime, const Ease::Interpolation& type = Ease::Linear, Interpolation1d::OnPathEndCallback PathEndCallback = Interpolation1d::OnPathEndCallback() );

		Interpolation1d * createFadeIn( const Time& Time, const bool& alphaChilds = true, const Ease::Interpolation& type = Ease::Linear );

		Interpolation1d * createFadeOut( const Time& Time, const bool& alphaChilds = true, const Ease::Interpolation& type = Ease::Linear );

		Interpolation1d * closeFadeOut( const Time& Time, const bool& alphaChilds = true, const Ease::Interpolation& type = Ease::Linear );

		Interpolation1d * disableFadeOut( const Time & Time, const bool& alphaChilds = true, const Ease::Interpolation& type = Ease::Linear );

		bool isFadingOut();

		UIActionManager * getActionManager();

		void runAction( UIAction * action );
	protected:
		typedef std::map< Uint32, std::map<Uint32, UIEventCallback> > UIEventsMap;
		friend class UIManager;
		friend class UIWindow;

		std::string		mId;
		Uint32			mIdHash;
		Vector2i		mPos;
		Vector2i		mRealPos;
		Vector2i		mScreenPos;
		Vector2f		mScreenPosf;
		Sizei			mSize;
		Sizei			mRealSize;

		Uint32			mFlags;
		UintPtr			mData;

		UINode *		mParentCtrl;
		UIWindow *		mParentWindowCtrl;
		UINode *		mChild;			//! Pointer to the first child of the node
		UINode *		mChildLast;		//! Pointer to the last child added
		UINode *		mNext;			//! Pointer to the next child of the father
		UINode *		mPrev;			//! Pointer to the prev child of the father
		UISkinState *	mSkinState;

		UIBackground *	mBackground;
		UIBorder *		mBorder;

		Uint32			mNodeFlags;
		BlendMode		mBlend;
		Uint16			mNumCallBacks;

		mutable Polygon2f		mPoly;
		Vector2f 		mCenter;

		UIEventsMap		mEvents;

		bool			mVisible;
		bool			mEnabled;

		Vector2i 	mDragPoint;
		Uint32 		mDragButton;

		Float				mRotation;
		OriginPoint			mRotationOrigin;
		Vector2f 			mScale;
		OriginPoint			mScaleOrigin;
		Float				mAlpha;

		UIActionManager *	mActionManager;

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

		virtual Uint32 onFocus();

		virtual Uint32 onFocusLoss();

		virtual void onClose();

		virtual Uint32 onValueChange();

		virtual void onVisibilityChange();

		virtual void onEnabledChange();

		virtual void onPositionChange();

		virtual void onSizeChange();

		virtual void onParentSizeChange( const Vector2i& SizeChange );

		virtual void onStateChange();

		virtual void onParentChange();

		virtual void onAlignChange();
		
		virtual void onWidgetFocusLoss();

		virtual void drawSkin();

		virtual void drawBackground();

		virtual void drawBorder();

		virtual void updateWorldPolygon();

		virtual void updateCenter();

		virtual void matrixSet();

		virtual void matrixUnset();

		virtual void drawChilds();

		virtual void onThemeLoaded();

		virtual void onChildCountChange();

		virtual void onAngleChange();

		virtual void onScaleChange();

		virtual void onAlphaChange();

		virtual UINode * overFind( const Vector2f& Point );

		virtual void onParentWindowChange();

		virtual void clipMe();

		virtual Uint32 onDrag( const Vector2i& position );

		virtual Uint32 onDragStart( const Vector2i& position );

		virtual Uint32 onDragStop( const Vector2i& position );

		void checkClose();

		virtual void internalDraw();

		void childDeleteAll();

		void childAdd( UINode * ChildCtrl );

		void childAddAt( UINode * ChildCtrl, Uint32 position );

		void childRemove( UINode * ChildCtrl );

		bool isChild( UINode * ChildCtrl ) const;

		bool inParentTreeOf( UINode * Child ) const;

		Uint32 childCount() const;

		UINode * childAt( Uint32 Index ) const;

		UINode * childPrev( UINode * Ctrl, bool Loop = false ) const;

		UINode * childNext( UINode * Ctrl, bool Loop = false ) const;

		virtual void clipDisable();

		void setPrevSkinState();

		virtual void updateScreenPos();

		void writeCtrlFlag( const Uint32& Flag, const Uint32& Val );

		void writeFlag( const Uint32& Flag, const Uint32& Val );

		void sendParentSizeChange( const Vector2i& SizeChange );

		Rect makePadding( bool PadLeft = true, bool PadRight = true, bool PadTop = true, bool PadBottom = true, bool SkipFlags = false );

		Sizei getSkinSize( UISkin * Skin, const Uint32& State = UISkinState::StateNormal );

		Rectf getRectf();

		void drawHighlightFocus();

		void drawOverNode();

		void drawDebugData();

		void drawBox();

		void setInternalPosition( const Vector2i& Pos );

		void setInternalSize( const Sizei& size );

		void setInternalWidth( const Int32& width );

		void setInternalHeight( const Int32& height );

		void setInternalPixelsSize( const Sizei& size );

		void setInternalPixelsWidth( const Int32& width );

		void setInternalPixelsHeight( const Int32& height );

		Color getColor( const Color& Col );

		UINode * findIdHash( const Uint32& idHash );

		UIWindow * getParentWindow();

		void updateRotationOrigin();

		void updateScaleOrigin();

		void setDirty();

		void setChildsDirty();
};

}}

#endif
