#ifndef EE_UICUIMANAGER_H
#define EE_UICUIMANAGER_H

#include "cuicontrol.hpp"
#include "cuiwindow.hpp"
#include "../window/cinput.hpp"
#include "../window/cwindow.hpp"

namespace EE { namespace UI {

class EE_API cUIManager : public tSingleton<cUIManager> {
	friend class tSingleton<cUIManager>;
	public:
		cUIManager();

		~cUIManager();

		cUIWindow * MainControl() const;

		cUIControl * FocusControl() const;

		void FocusControl( cUIControl * Ctrl );

		cUIControl * OverControl() const;

		void OverControl( cUIControl * Ctrl );

		void Init( Uint32 Flags = 0, cWindow * window = NULL );

		void Shutdown();

		void Update();

		void Draw();

		const eeFloat& Elapsed() const;

		void ResizeControl();

		void SendMsg( cUIControl * Ctrl, const Uint32& Msg, const Uint32& Flags = 0 );

		eeVector2i GetMousePos();

		cInput * GetInput() const;

		const Uint32& PressTrigger() const;

		const Uint32& LastPressTrigger() const;

		void ClipEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height );

		void ClipDisable();

		void SendKeyUp( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod );

		void SendKeyDown( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod );

		void HighlightFocus( bool Highlight );

		bool HighlightFocus() const;

		void HighlightColor( const eeColorA& Color );

		const eeColorA& HighlightColor() const;

		void SendMouseClick( cUIControl * ToCtrl, const eeVector2i& Pos, const Uint32 Flags );

		void SendMouseUp( cUIControl * ToCtrl, const eeVector2i& Pos, const Uint32 Flags );

		void SendMouseDown( cUIControl * ToCtrl, const eeVector2i& Pos, const Uint32 Flags );

		cWindow * GetWindow() const;
	protected:
		cWindow *			mWindow;
		cInput *			mKM;
		cUIWindow *			mControl;
		cUIControl *		mFocusControl;
		cUIControl *		mOverControl;
		cUIControl * 		mDownControl;

		eeFloat 			mElapsed;
		Int32 				mCbId;
		Uint32				mResizeCb;

		Uint32				mFlags;
		eeColorA			mHighlightColor;

		bool				mInit;
		bool 				mFirstPress;

		void				InputCallback( InputEvent * Event );

		void				CheckTabPress( const Uint32& KeyCode );
};

}}

#endif
