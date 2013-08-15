#ifndef EE_UICUIMANAGER_H
#define EE_UICUIMANAGER_H

#include <eepp/ui/cuicontrol.hpp>
#include <eepp/ui/cuiwindow.hpp>
#include <eepp/window/cinput.hpp>
#include <eepp/window/cwindow.hpp>

namespace EE { namespace UI {

class EE_API cUIManager {
	SINGLETON_DECLARE_HEADERS(cUIManager)

	public:
		~cUIManager();

		cUIWindow * MainControl() const;

		cUIControl * FocusControl() const;

		void FocusControl( cUIControl * Ctrl );

		cUIControl * OverControl() const;

		void OverControl( cUIControl * Ctrl );

		void Init( Uint32 Flags = 0, Window::cWindow * window = NULL );

		void Shutdown();

		void Update();

		void Draw();

		const cTime& Elapsed() const;

		void ResizeControl( cWindow * win );

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

		void HighlightFocusColor( const eeColorA& Color );

		const eeColorA& HighlightFocusColor() const;

		void HighlightOver( bool Highlight );

		bool HighlightOver() const;

		void HighlightOverColor( const eeColorA& Color );

		const eeColorA& HighlightOverColor() const;

		void SendMouseClick( cUIControl * ToCtrl, const eeVector2i& Pos, const Uint32 Flags );

		void SendMouseUp( cUIControl * ToCtrl, const eeVector2i& Pos, const Uint32 Flags );

		void SendMouseDown( cUIControl * ToCtrl, const eeVector2i& Pos, const Uint32 Flags );

		Window::cWindow * GetWindow() const;

		/** Control where the mouse click started to be down */
		cUIControl * DownControl() const;

		cUIControl * LossFocusControl() const;

		const bool& IsShootingDown() const;
	protected:
		friend class cUIControl;
		friend class cUIWindow;

		Window::cWindow *			mWindow;
		cInput *			mKM;
		cUIWindow *			mControl;
		cUIControl *		mFocusControl;
		cUIControl *		mOverControl;
		cUIControl * 		mDownControl;
		cUIControl *		mLossFocusControl;
		std::list<cUIWindow*> mWindowsList;
		std::list<cUIControl*> mCloseList;

		cTime	 			mElapsed;
		Int32 				mCbId;
		Uint32				mResizeCb;

		Uint32				mFlags;
		eeColorA			mHighlightFocusColor;
		eeColorA			mHighlightOverColor;

		bool				mInit;
		bool 				mFirstPress;
		bool				mShootingDown;

		cUIManager();

		void				InputCallback( InputEvent * Event );

		void				CheckTabPress( const Uint32& KeyCode );

		void				SetActiveWindow( cUIWindow * window );

		void				SetFocusLastWindow( cUIWindow * window  );

		void				WindowAdd( cUIWindow * win );

		void				WindowRemove( cUIWindow * win );

		bool				WindowExists( cUIWindow * win );

		void				AddToCloseQueue( cUIControl * Ctrl );

		void				CheckClose();
};

}}

#endif
