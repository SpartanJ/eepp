#ifndef EE_UICUICONTROL_H
#define EE_UICUICONTROL_H

#include <eepp/ui/base.hpp>
#include <eepp/ui/uihelper.hpp>
#include <eepp/ui/cuibackground.hpp>
#include <eepp/ui/cuiborder.hpp>
#include <eepp/ui/cuimessage.hpp>
#include <eepp/ui/cuievent.hpp>
#include <eepp/ui/cuieventkey.hpp>
#include <eepp/ui/cuieventmouse.hpp>
#include <eepp/ui/cuiskin.hpp>
#include <eepp/ui/cuiskinstate.hpp>
#include <eepp/ui/cuiskinsimple.hpp>
#include <eepp/ui/cuiskincomplex.hpp>
#include <eepp/ui/cuitheme.hpp>
#include <eepp/ui/cuithememanager.hpp>

namespace EE { namespace UI {

class cUIManager;

class EE_API cUIControl {
	public:
		typedef cb::Callback1<void, const cUIEvent*> UIEventCallback;

		class CreateParams {
			public:
				CreateParams(
					cUIControl * parentCtrl,
					const eeVector2i& pos = eeVector2i( 0, 0 ),
					const eeSize& size = eeSize( -1, -1 ),
					const Uint32& flags = UI_CONTROL_DEFAULT_FLAGS,
					const EE_BLEND_MODE& blend = ALPHA_NORMAL,
					const cUIBackground& Back = cUIBackground(),
					const cUIBorder& Bord = cUIBorder()
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
					Pos				= eeVector2i( 0, 0 );
					Size			= eeSize( -1, -1 );
					Flags			= UI_CONTROL_DEFAULT_FLAGS;
					Blend			= ALPHA_NORMAL;
				}

				~CreateParams() {}

				void PosSet( const eeVector2i& pos )						{ Pos.x = pos.x; Pos.y = pos.y; }
				void PosSet( const Int32& X, const Int32& Y )				{ Pos.x = X; Pos.y = Y; }
				void Parent( cUIControl * Ctrl )							{ ParentCtrl = Ctrl; }
				void SizeSet( const eeSize& size )							{ Size.x = size.x; Size.y = size.y;	}
				void SizeSet( const Int32& Width, const Int32& Height )		{ Size.x = Width; Size.y = Height;	}

				cUIControl *		ParentCtrl;
				eeVector2i			Pos;
				eeSize				Size;
				Uint32				Flags;
				cUIBackground		Background;
				cUIBorder			Border;
				EE_BLEND_MODE	Blend;
		};

		cUIControl( const CreateParams& Params );

		virtual ~cUIControl();

		void ScreenToControl( eeVector2i& Pos ) const;

		void ControlToScreen( eeVector2i& Pos ) const;

		virtual Uint32 Type() const;

		virtual bool IsType( const Uint32& type ) const;

		virtual void MessagePost( const cUIMessage * Msg );

		bool IsInside( const eeVector2i& Pos ) const;

		void Pos( const eeVector2i& Pos );

		void Pos( const Int32& x, const Int32& y );

		const eeVector2i& Pos() const;

		virtual void Size( const eeSize& Size );

		void Size( const Int32& Width, const Int32& Height );

		const eeSize& Size();

		eeRecti Rect() const;

		void Visible( const bool& visible );

		bool Visible() const;

		bool Hided() const;

		void Enabled( const bool& enabled );

		bool Enabled() const;

		bool Disabled() const;

		cUIControl * Parent() const;

		void Parent( cUIControl * parent );

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

		cUIControl * NextGet() const;

		cUIControl * PrevGet() const;

		cUIControl * NextGetLoop() const;

		void Data( const Uint32& data );

		const Uint32& Data() const;

		cUIControl * ChildGetAt( eeVector2i CtrlPos, eeUint RecursiveLevel = 0 );

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

		cUIBackground * Background();

		cUIBorder * Border();

		void SetThemeByName( const std::string& Theme );

		virtual void SetTheme( cUITheme * Theme );

		void SetThemeControl( cUITheme * Theme, const std::string& ControlName );

		void SetThemeToChilds( cUITheme * Theme );

		cUISkin * GetSkin();

		void SetSkinFromTheme( cUITheme * Theme, const std::string& ControlName );

		virtual void SetSkin( cUISkin * Skin );

		cUIControl * ChildGetFirst() const;

		cUIControl * ChildGetLast() const;

		bool IsMouseOver();

		bool IsMouseOverMeOrChilds();

		const eePolygon2f& GetPolygon() const;

		const eeVector2f& GetPolygonCenter() const;

		void SetSkinState( const Uint32& State );

		bool HasFocus() const;

		virtual void SetFocus();

		bool IsParentOf( cUIControl * Ctrl );

		void SendEvent( const cUIEvent * Event );

		void SendMouseEvent( const Uint32& Event, const eeVector2i& Pos, const Uint32& Flags );

		void SendCommonEvent( const Uint32& Event );

		eeSize GetSkinSize();

		cUIControl * NextComplexControl();

		void ApplyDefaultTheme();
	protected:
		typedef std::map< Uint32, std::map<Uint32, UIEventCallback> > UIEventsMap;
		friend class cUIManager;

		eeVector2i		mPos;
		eeVector2i		mScreenPos;
		eeSize			mSize;

		Uint32			mFlags;
		Uint32 			mData;

		cUIControl *	mParentCtrl;
		cUIControl *	mChild;			//! Pointer to the first child of the control
		cUIControl * 	mChildLast;		//! Pointer to the last child added
		cUIControl *	mNext;			//! Pointer to the next child of the father
		cUIControl * 	mPrev;			//! Pointer to the prev child of the father
		cUISkinState *	mSkinState;

		cUIBackground *	mBackground;
		cUIBorder *		mBorder;

		Uint32			mControlFlags;
		Uint16			mBlend;
		Uint16			mNumCallBacks;

		eePolygon2f 	mPoly;
		eeVector2f 		mCenter;

		UIEventsMap		mEvents;

		bool			mVisible;
		bool			mEnabled;

		virtual Uint32 OnMessage( const cUIMessage * Msg );

		virtual Uint32 OnKeyDown( const cUIEventKey& Event );

		virtual Uint32 OnKeyUp( const cUIEventKey& Event );

		virtual Uint32 OnMouseMove( const eeVector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseDown( const eeVector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseClick( const eeVector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseDoubleClick( const eeVector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseUp( const eeVector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseEnter( const eeVector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnMouseExit( const eeVector2i& Pos, const Uint32 Flags );

		virtual Uint32 OnFocus();

		virtual Uint32 OnFocusLoss();

		virtual void OnClose();

		virtual Uint32 OnValueChange();

		virtual void OnVisibleChange();

		virtual void OnEnabledChange();

		virtual void OnPosChange();

		virtual void OnSizeChange();

		virtual void OnParentSizeChange( const eeVector2i& SizeChange );

		virtual void OnStateChange();
		
		virtual void OnComplexControlFocusLoss();

		virtual void BackgroundDraw();

		virtual void BorderDraw();

		virtual void UpdateQuad();

		virtual void MatrixSet();

		virtual void MatrixUnset();

		virtual void DrawChilds();

		virtual cUIControl * OverFind( const eeVector2f& Point );

		void ClipMe();

		void CheckClose();

		void InternalDraw();

		void ChildDeleteAll();

		void ChildAdd( cUIControl * ChildCtrl );

		void ChildAddAt( cUIControl * ChildCtrl, Uint32 Pos );

		void ChildRemove( cUIControl * ChildCtrl );

		bool IsChild( cUIControl * ChildCtrl ) const;

		bool InParentTreeOf( cUIControl * Child ) const;

		Uint32 ChildCount() const;

		cUIControl * ChildAt( Uint32 Index ) const;

		cUIControl * ChildPrev( cUIControl * Ctrl, bool Loop = false ) const;

		cUIControl * ChildNext( cUIControl * Ctrl, bool Loop = false ) const;

		void ClipDisable();

		void ClipTo();

		void SetPrevSkinState();

		virtual void UpdateScreenPos();

		void UpdateChildsScreenPos();

		void WriteCtrlFlag( const Uint32& Flag, const Uint32& Val );

		void WriteFlag( const Uint32& Flag, const Uint32& Val );

		void SendParentSizeChange( const eeVector2i& SizeChange );

		eeFloat Elapsed();

		eeRecti MakePadding( bool PadLeft = true, bool PadRight = true, bool PadTop = true, bool PadBottom = true, bool SkipFlags = false );

		void SafeDeleteSkinState();

		eeSize GetSkinSize( cUISkin * Skin, const Uint32& State = cUISkinState::StateNormal );
};

}}

#endif
