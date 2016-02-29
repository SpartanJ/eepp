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

				void PosSet( const Vector2i& pos )						{ Pos.x = pos.x; Pos.y = pos.y; }
				void PosSet( const Int32& X, const Int32& Y )				{ Pos.x = X; Pos.y = Y; }
				void Parent( UIControl * Ctrl )							{ ParentCtrl = Ctrl; }
				void SizeSet( const Sizei& size )							{ Size.x = size.x; Size.y = size.y;	}
				void SizeSet( const Int32& Width, const Int32& Height )		{ Size.x = Width; Size.y = Height;	}

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

		void ScreenToControl( Vector2i& Pos ) const;

		void ControlToScreen( Vector2i& Pos ) const;

		void WorldToControl( Vector2i& pos ) const;

		void ControlToWorld( Vector2i& pos ) const;

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void MessagePost( const UIMessage * Msg );

		bool IsInside( const Vector2i& Pos ) const;

		void Pos( const Vector2i& Pos );

		void Pos( const Int32& x, const Int32& y );

		const Vector2i& Pos() const;

		virtual void Size( const Sizei& Size );

		void Size( const Int32& Width, const Int32& Height );

		const Sizei& Size();

		Recti Rect() const;

		void Visible( const bool& visible );

		bool Visible() const;

		bool Hided() const;

		void Enabled( const bool& enabled );

		bool Enabled() const;

		bool Disabled() const;

		UIControl * Parent() const;

		void Parent( UIControl * parent );

		void CenterHorizontal();

		void CenterVertical();

		void Center();

		virtual void Close();

		virtual void Draw();

		virtual void Update();

		Uint32 HAlign() const;

		void HAlign( Uint32 halign );

		Uint32 VAlign() const;

		void VAlign( Uint32 valign );

		void FillBackground( bool enabled );

		void Border( bool enabled );

		UIControl * NextGet() const;

		UIControl * PrevGet() const;

		UIControl * NextGetLoop() const;

		void Data( const UintPtr& data );

		const UintPtr& Data() const;

		UIControl * ChildGetAt( Vector2i CtrlPos, unsigned int RecursiveLevel = 0 );

		const Uint32& Flags() const;

		void Flags( const Uint32& flags );

		void Blend( const EE_BLEND_MODE& blend );

		EE_BLEND_MODE Blend();

		void ToFront();

		void ToBack();

		void ToPos( const Uint32& Pos );

		const Uint32& ControlFlags() const;

		/** Use it at your own risk */
		void ControlFlags( const Uint32& Flags );

		Uint32 IsAnimated();

		Uint32 IsDragable();

		Uint32 IsComplex();

		Uint32 IsClipped();

		Uint32 AddEventListener( const Uint32& EventType, const UIEventCallback& Callback );

		void RemoveEventListener( const Uint32& CallbackId );

		UIBackground * Background();

		UIBorder * Border();

		void SetThemeByName( const std::string& Theme );

		virtual void SetTheme( UITheme * Theme );

		void SetThemeControl( UITheme * Theme, const std::string& ControlName );

		void SetThemeToChilds( UITheme * Theme );

		UISkin * GetSkin();

		void SetSkinFromTheme( UITheme * Theme, const std::string& ControlName );

		virtual void SetSkin( const UISkin& Skin );

		UIControl * ChildGetFirst() const;

		UIControl * ChildGetLast() const;

		bool IsMouseOver();

		bool IsMouseOverMeOrChilds();

		Polygon2f &GetPolygon();

		const Vector2f& GetPolygonCenter() const;

		void SetSkinState( const Uint32& State );

		bool HasFocus() const;

		virtual void SetFocus();

		bool IsParentOf( UIControl * Ctrl );

		void SendEvent( const UIEvent * Event );

		void SendMouseEvent( const Uint32& Event, const Vector2i& Pos, const Uint32& Flags );

		void SendCommonEvent( const Uint32& Event );

		Sizei GetSkinSize();

		UIControl * NextComplexControl();

		void ApplyDefaultTheme();

		Recti GetScreenRect();

		void ChildsCloseAll();
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

		virtual Uint32 OnMessage( const UIMessage * Msg );

		virtual Uint32 OnKeyDown( const UIEventKey& Event );

		virtual Uint32 OnKeyUp( const UIEventKey& Event );

		virtual Uint32 OnMouseMove( const Vector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseDown( const Vector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseClick( const Vector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseDoubleClick( const Vector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseUp( const Vector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseEnter( const Vector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseExit( const Vector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnFocus();

		virtual Uint32 OnFocusLoss();

		virtual void OnClose();

		virtual Uint32 OnValueChange();

		virtual void OnVisibleChange();

		virtual void OnEnabledChange();

		virtual void OnPosChange();

		virtual void OnSizeChange();

		virtual void OnParentSizeChange( const Vector2i& SizeChange );

		virtual void OnStateChange();
		
		virtual void OnComplexControlFocusLoss();

		virtual void BackgroundDraw();

		virtual void BorderDraw();

		virtual void UpdateQuad();

		virtual void UpdateCenter();

		virtual void MatrixSet();

		virtual void MatrixUnset();

		virtual void DrawChilds();

		virtual void DoAfterSetTheme();

		virtual UIControl * OverFind( const Vector2f& Point );

		void ClipMe();

		void CheckClose();

		void InternalDraw();

		void ChildDeleteAll();

		void ChildAdd( UIControl * ChildCtrl );

		void ChildAddAt( UIControl * ChildCtrl, Uint32 Pos );

		void ChildRemove( UIControl * ChildCtrl );

		bool IsChild( UIControl * ChildCtrl ) const;

		bool InParentTreeOf( UIControl * Child ) const;

		Uint32 ChildCount() const;

		UIControl * ChildAt( Uint32 Index ) const;

		UIControl * ChildPrev( UIControl * Ctrl, bool Loop = false ) const;

		UIControl * ChildNext( UIControl * Ctrl, bool Loop = false ) const;

		void ClipDisable();

		void ClipTo();

		void SetPrevSkinState();

		virtual void UpdateScreenPos();

		void UpdateChildsScreenPos();

		void WriteCtrlFlag( const Uint32& Flag, const Uint32& Val );

		void WriteFlag( const Uint32& Flag, const Uint32& Val );

		void SendParentSizeChange( const Vector2i& SizeChange );

		Time Elapsed();

		Recti MakePadding( bool PadLeft = true, bool PadRight = true, bool PadTop = true, bool PadBottom = true, bool SkipFlags = false );

		void SafeDeleteSkinState();

		Sizei GetSkinSize( UISkin * Skin, const Uint32& State = UISkinState::StateNormal );

		Rectf GetRectf();
};

}}

#endif
