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
		typedef cb::Callback1<void, const UIEvent*> UIEventCallback;

		class CreateParams {
			public:
				CreateParams(
					UIControl * parentCtrl,
					const Vector2i& pos = Vector2i( 0, 0 ),
					const Sizei& size = Sizei( -1, -1 ),
					const Uint32& flags = UI_CONTROL_DEFAULT_FLAGS,
					const EE_BLEND_MODE& blend = ALPHA_NORMAL,
					const UIBackground& Back = UIBackground(),
					const UIBorder& Bord = UIBorder()
				) :
					ParentCtrl( parentCtrl ),
					Pos( pos ),
					Size( size ),
					Flags( flags ),
					Blend( blend )
				{
					Background = Back;
					Border = Bord;
				}

				CreateParams() {
					ParentCtrl		= NULL;
					Pos				= Vector2i( 0, 0 );
					Size			= Sizei( -1, -1 );
					Flags			= UI_CONTROL_DEFAULT_FLAGS;
					Blend			= ALPHA_NORMAL;
				}

				~CreateParams() {}

				void setPos( const Vector2i& pos )						{ Pos.x = pos.x; Pos.y = pos.y; }
				void setPos( const Int32& X, const Int32& Y )				{ Pos.x = X; Pos.y = Y; }
				void setParent( UIControl * Ctrl )							{ ParentCtrl = Ctrl; }
				void setSize( const Sizei& size )							{ Size.x = size.x; Size.y = size.y;	}
				void setSize( const Int32& Width, const Int32& Height )		{ Size.x = Width; Size.y = Height;	}

				UIControl *		ParentCtrl;
				Vector2i			Pos;
				Sizei				Size;
				Uint32				Flags;
				UIBackground		Background;
				UIBorder			Border;
				EE_BLEND_MODE	Blend;
		};

		UIControl( const CreateParams& Params );

		virtual ~UIControl();

		void screenToControl( Vector2i& position ) const;

		void controlToScreen( Vector2i& position ) const;

		void worldToControl( Vector2i& pos ) const;

		void controlToWorld( Vector2i& pos ) const;

		virtual Uint32 getType() const;

		virtual bool isType( const Uint32& type ) const;

		virtual void messagePost( const UIMessage * Msg );

		bool isInside( const Vector2i& position ) const;

		void position( const Vector2i& position );

		void position( const Int32& x, const Int32& y );

		const Vector2i& position() const;

		virtual void size( const Sizei& size );

		void size( const Int32& Width, const Int32& Height );

		const Sizei& size();

		Recti getRect() const;

		void visible( const bool& visible );

		bool visible() const;

		bool isHided() const;

		void enabled( const bool& enabled );

		bool enabled() const;

		bool isDisabled() const;

		UIControl * parent() const;

		void parent( UIControl * parent );

		void centerHorizontal();

		void centerVertical();

		void center();

		virtual void close();

		virtual void draw();

		virtual void update();

		Uint32 hAlign() const;

		void hAlign( Uint32 halign );

		Uint32 vAlign() const;

		void vAlign( Uint32 valign );

		void fillBackground( bool enabled );

		void border( bool enabled );

		UIControl * nextGet() const;

		UIControl * prevGet() const;

		UIControl * nextGetLoop() const;

		void data( const UintPtr& data );

		const UintPtr& data() const;

		UIControl * childGetAt( Vector2i CtrlPos, unsigned int RecursiveLevel = 0 );

		const Uint32& flags() const;

		void flags( const Uint32& flags );

		void blend( const EE_BLEND_MODE& blend );

		EE_BLEND_MODE blend();

		void toFront();

		void toBack();

		void toPos( const Uint32& position );

		const Uint32& controlFlags() const;

		/** Use it at your own risk */
		void controlFlags( const Uint32& flags );

		Uint32 isAnimated();

		Uint32 isDragable();

		Uint32 isComplex();

		Uint32 isClipped();

		Uint32 addEventListener( const Uint32& EventType, const UIEventCallback& Callback );

		void removeEventListener( const Uint32& CallbackId );

		UIBackground * background();

		UIBorder * border();

		void setThemeByName( const std::string& Theme );

		virtual void setTheme( UITheme * Theme );

		void setThemeControl( UITheme * Theme, const std::string& ControlName );

		void setThemeToChilds( UITheme * Theme );

		UISkin * getSkin();

		void setSkinFromTheme( UITheme * Theme, const std::string& ControlName );

		virtual void setSkin( const UISkin& Skin );

		UIControl * childGetFirst() const;

		UIControl * childGetLast() const;

		bool isMouseOver();

		bool isMouseOverMeOrChilds();

		Polygon2f &getPolygon();

		const Vector2f& getPolygonCenter() const;

		void setSkinState( const Uint32& State );

		bool hasFocus() const;

		virtual void setFocus();

		bool isParentOf( UIControl * Ctrl );

		void sendEvent( const UIEvent * Event );

		void sendMouseEvent( const Uint32& Event, const Vector2i& position, const Uint32& flags );

		void sendCommonEvent( const Uint32& Event );

		Sizei getSkinSize();

		UIControl * nextComplexControl();

		void applyDefaultTheme();

		Recti getScreenRect();

		void childsCloseAll();
	protected:
		typedef std::map< Uint32, std::map<Uint32, UIEventCallback> > UIEventsMap;
		friend class UIManager;
		friend class UIWindow;

		Vector2i		mPos;
		Vector2i		mScreenPos;
		Vector2f		mScreenPosf;
		Sizei			mSize;

		Uint32			mFlags;
		UintPtr			mData;

		UIControl *	mParentCtrl;
		UIControl *	mChild;			//! Pointer to the first child of the control
		UIControl * 	mChildLast;		//! Pointer to the last child added
		UIControl *	mNext;			//! Pointer to the next child of the father
		UIControl * 	mPrev;			//! Pointer to the prev child of the father
		UISkinState *	mSkinState;

		UIBackground *	mBackground;
		UIBorder *		mBorder;

		Uint32			mControlFlags;
		Uint16			mBlend;
		Uint16			mNumCallBacks;

		Polygon2f 	mPoly;
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

		virtual void onVisibleChange();

		virtual void onEnabledChange();

		virtual void onPositionChange();

		virtual void onSizeChange();

		virtual void onParentSizeChange( const Vector2i& SizeChange );

		virtual void onStateChange();
		
		virtual void onComplexControlFocusLoss();

		virtual void backgroundDraw();

		virtual void borderDraw();

		virtual void updateQuad();

		virtual void updateCenter();

		virtual void matrixSet();

		virtual void matrixUnset();

		virtual void drawChilds();

		virtual void doAftersetTheme();

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

		Time elapsed();

		Recti makePadding( bool PadLeft = true, bool PadRight = true, bool PadTop = true, bool PadBottom = true, bool SkipFlags = false );

		void safeDeleteSkinState();

		Sizei getSkinSize( UISkin * Skin, const Uint32& State = UISkinState::StateNormal );

		Rectf getRectf();
};

}}

#endif
