#ifndef EE_UICUIMANAGER_H
#define EE_UICUIMANAGER_H

#include <eepp/ui/uicontrol.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>
#include <eepp/window/cursorhelper.hpp>

namespace EE { namespace UI {

class EE_API UIManager {
	SINGLETON_DECLARE_HEADERS(UIManager)

	public:
		~UIManager();

		UIWindow * MainControl() const;

		UIControl * FocusControl() const;

		void FocusControl( UIControl * Ctrl );

		UIControl * OverControl() const;

		void OverControl( UIControl * Ctrl );

		void Init( Uint32 Flags = 0, EE::Window::Window * window = NULL );

		void Shutdown();

		void Update();

		void Draw();

		const Time& Elapsed() const;

		void ResizeControl( EE::Window::Window * win );

		void SendMsg( UIControl * Ctrl, const Uint32& Msg, const Uint32& Flags = 0 );

		Vector2i GetMousePos();

		Input * GetInput() const;

		const Uint32& PressTrigger() const;

		const Uint32& LastPressTrigger() const;

		void ClipEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height );

		void ClipDisable();

		void SendKeyUp( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod );

		void SendKeyDown( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod );

		void HighlightFocus( bool Highlight );

		bool HighlightFocus() const;

		void HighlightFocusColor( const ColorA& Color );

		const ColorA& HighlightFocusColor() const;

		void HighlightOver( bool Highlight );

		bool HighlightOver() const;

		void HighlightOverColor( const ColorA& Color );

		const ColorA& HighlightOverColor() const;

		void SendMouseClick( UIControl * ToCtrl, const Vector2i& Pos, const Uint32 Flags );

		void SendMouseUp( UIControl * ToCtrl, const Vector2i& Pos, const Uint32 Flags );

		void SendMouseDown( UIControl * ToCtrl, const Vector2i& Pos, const Uint32 Flags );

		EE::Window::Window * GetWindow() const;

		/** Control where the mouse click started to be down */
		UIControl * DownControl() const;

		UIControl * LossFocusControl() const;

		const bool& IsShootingDown() const;

		/** @return The position of the mouse when the event MouseDown was fired last time.
		**	Useful to compare the mouse position of the MouseClick event */
		const Vector2i& GetMouseDownPos() const;

		void SetControlDragging( bool dragging );

		const bool& IsControlDragging() const;

		void UseGlobalCursors( const bool& use );

		const bool& UseGlobalCursors();

		void SetCursor( EE_CURSOR_TYPE cursor );
	protected:
		friend class UIControl;
		friend class UIWindow;

		EE::Window::Window *			mWindow;
		Input *			mKM;
		UIWindow *			mControl;
		UIControl *		mFocusControl;
		UIControl *		mOverControl;
		UIControl * 		mDownControl;
		UIControl *		mLossFocusControl;
		std::list<UIWindow*> mWindowsList;
		std::list<UIControl*> mCloseList;

		Time	 			mElapsed;
		Int32 				mCbId;
		Uint32				mResizeCb;

		Uint32				mFlags;
		ColorA			mHighlightFocusColor;
		ColorA			mHighlightOverColor;
		Vector2i			mMouseDownPos;

		bool				mInit;
		bool 				mFirstPress;
		bool				mShootingDown;
		bool				mControlDragging;
		bool				mUseGlobalCursors;

		UIManager();

		void				InputCallback( InputEvent * Event );

		void				CheckTabPress( const Uint32& KeyCode );

		void				SetActiveWindow( UIWindow * window );

		void				SetFocusLastWindow( UIWindow * window  );

		void				WindowAdd( UIWindow * win );

		void				WindowRemove( UIWindow * win );

		bool				WindowExists( UIWindow * win );

		void				AddToCloseQueue( UIControl * Ctrl );

		void				CheckClose();
};

}}

#endif
