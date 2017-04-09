#ifndef EE_UICUICONTROL_H
#define EE_UICUICONTROL_H

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
class UIManager;

class EE_API UIControl {
	public:
		static UIControl * New();

		typedef cb::Callback1<void, const UIEvent*> UIEventCallback;

		UIControl();

		virtual ~UIControl();

		void screenToControl( Vector2i& position ) const;

		void controlToScreen( Vector2i& position ) const;

		void worldToControl( Vector2i& pos ) const;

		void controlToWorld( Vector2i& pos ) const;

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void messagePost( const UIMessage * Msg );

		UIControl * setPosition( const Vector2i& position );

		UIControl * setPosition( const Int32& x, const Int32& y );

		void setPixelsPosition( const Vector2i& position );

		void setPixelsPosition( const Int32& x, const Int32& y );

		const Vector2i& getPosition() const;

		const Vector2i& getRealPosition() const;

		virtual UIControl * setSize( const Sizei& size );

		UIControl * setSize( const Int32& Width, const Int32& Height );

		void setPixelsSize( const Sizei& size );

		void setPixelsSize( const Int32& x, const Int32& y );

		const Sizei& getSize();

		const Sizei& getRealSize();

		Recti getRect() const;

		UIControl * setVisible( const bool& visible );

		bool isVisible() const;

		bool isHided() const;

		UIControl * setEnabled( const bool& enabled );

		bool isEnabled() const;

		bool isDisabled() const;

		UIControl * getParent() const;

		UIControl * setParent( UIControl * parent );

		void centerHorizontal();

		void centerVertical();

		void center();

		virtual void close();

		virtual void draw();

		virtual void update();

		Uint32 getHorizontalAlign() const;

		UIControl * setHorizontalAlign( Uint32 halign );

		Uint32 getVerticalAlign() const;

		UIControl * setVerticalAlign( Uint32 valign );

		UIBackground * setBackgroundFillEnabled( bool enabled );

		UIBorder * setBorderEnabled( bool enabled );

		UIControl * getNextControl() const;

		UIControl * getPrevControl() const;

		UIControl * getNextControlLoop() const;

		UIControl * setData( const UintPtr& data );

		const UintPtr& getData() const;

		UIControl * childGetAt( Vector2i CtrlPos, unsigned int RecursiveLevel = 0 );

		const Uint32& getFlags() const;

		virtual UIControl * setFlags( const Uint32& flags );

		virtual UIControl * unsetFlags( const Uint32& flags );

		virtual UIControl * resetFlags( Uint32 newFlags = 0 );

		UIControl * setBlendMode( const EE_BLEND_MODE& blend );

		EE_BLEND_MODE getBlendMode();

		void toFront();

		void toBack();

		void toPosition( const Uint32& position );

		const Uint32& getControlFlags() const;

		/** Use it at your own risk */
		void setControlFlags( const Uint32& flags );

		Uint32 isAnimated();

		Uint32 isDragable();

		Uint32 isWidget();

		Uint32 isClipped();

		Uint32 isRotated();

		Uint32 isScaled();

		bool isMeOrParentTreeRotated();

		bool isMeOrParentTreeScaled();

		bool isMeOrParentTreeScaledOrRotated();

		Uint32 addEventListener( const Uint32& EventType, const UIEventCallback& Callback );

		void removeEventListener( const Uint32& CallbackId );

		UIBackground * getBackground();

		UIBorder * getBorder();

		void setThemeByName( const std::string& Theme );

		virtual void setTheme( UITheme * Theme );

		UIControl * setThemeSkin( UITheme * Theme, const std::string& skinName );

		UIControl * setThemeSkin( const std::string& skinName );

		void setThemeToChilds( UITheme * Theme );

		UISkin * getSkin();

		virtual UIControl * setSkin( const UISkin& Skin );

		UIControl * setSkin( UISkin * skin );

		void removeSkin();

		UIControl * getFirstChild() const;

		UIControl * getLastChild() const;

		bool isMouseOver();

		bool isMouseOverMeOrChilds();

		Polygon2f& getPolygon();

		const Vector2f& getPolygonCenter() const;

		void setSkinState( const Uint32& State );

		bool hasFocus() const;

		virtual void setFocus();

		bool isParentOf( UIControl * Ctrl );

		void sendEvent( const UIEvent * Event );

		void sendMouseEvent( const Uint32& Event, const Vector2i& position, const Uint32& flags );

		void sendCommonEvent( const Uint32& Event );

		Sizei getSkinSize();

		UIControl * getNextComplexControl();

		void applyDefaultTheme();

		Recti getScreenRect();

		void childsCloseAll();

		std::string getId() const;

		UIControl * setId( const std::string & id );

		Uint32 getIdHash() const;

		UIControl * find( const std::string& id );

		UIControl * getWindowContainer();
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

		UIControl *		mParentCtrl;
		UIControl *		mChild;			//! Pointer to the first child of the control
		UIControl * 	mChildLast;		//! Pointer to the last child added
		UIControl *		mNext;			//! Pointer to the next child of the father
		UIControl * 	mPrev;			//! Pointer to the prev child of the father
		UISkinState *	mSkinState;

		UIBackground *	mBackground;
		UIBorder *		mBorder;

		Uint32			mControlFlags;
		Uint16			mBlend;
		Uint16			mNumCallBacks;

		Polygon2f		mPoly;
		Vector2f 		mCenter;

		UIEventsMap		mEvents;

		bool			mVisible;
		bool			mEnabled;

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
		
		virtual void onComplexControlFocusLoss();

		virtual void drawSkin();

		virtual void drawBackground();

		virtual void drawBorder();

		virtual void updateQuad();

		virtual void updateCenter();

		virtual void matrixSet();

		virtual void matrixUnset();

		virtual void drawChilds();

		virtual void onThemeLoaded();

		virtual void onChildCountChange();

		virtual UIControl * overFind( const Vector2f& Point );

		void clipMe();

		void checkClose();

		void internalDraw();

		void childDeleteAll();

		void childAdd( UIControl * ChildCtrl );

		void childAddAt( UIControl * ChildCtrl, Uint32 position );

		void childRemove( UIControl * ChildCtrl );

		bool isChild( UIControl * ChildCtrl ) const;

		bool inParentTreeOf( UIControl * Child ) const;

		Uint32 childCount() const;

		UIControl * childAt( Uint32 Index ) const;

		UIControl * childPrev( UIControl * Ctrl, bool Loop = false ) const;

		UIControl * childNext( UIControl * Ctrl, bool Loop = false ) const;

		void clipDisable();

		void clipTo();

		void setPrevSkinState();

		virtual void updateScreenPos();

		void updateChildsScreenPos();

		void writeCtrlFlag( const Uint32& Flag, const Uint32& Val );

		void writeFlag( const Uint32& Flag, const Uint32& Val );

		void sendParentSizeChange( const Vector2i& SizeChange );

		Time getElapsed();

		Recti makePadding( bool PadLeft = true, bool PadRight = true, bool PadTop = true, bool PadBottom = true, bool SkipFlags = false );

		Sizei getSkinSize( UISkin * Skin, const Uint32& State = UISkinState::StateNormal );

		Rectf getRectf();

		void drawHighlightFocus();

		void drawOverControl();

		void drawDebugData();

		void drawBox();

		void setInternalPosition( const Vector2i& Pos );

		void setInternalSize( const Sizei& size );

		void setInternalWidth( const Int32& width );

		void setInternalHeight( const Int32& height );

		void setInternalPixelsSize( const Sizei& size );

		void setInternalPixelsWidth( const Int32& width );

		void setInternalPixelsHeight( const Int32& height );

		UIControl * findIdHash( const Uint32& idHash );
};

}}

#endif
