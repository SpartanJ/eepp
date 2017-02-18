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

		UIWindow * getMainControl() const;

		UIControl * getFocusControl() const;

		void setFocusControl( UIControl * Ctrl );

		UIControl * getOverControl() const;

		void setOverControl( UIControl * Ctrl );

		void init( Uint32 Flags = 0, EE::Window::Window * window = NULL );

		void shutdown();

		void update();

		void draw();

		const Time& getElapsed() const;

		void resizeControl( EE::Window::Window * win );

		void sendMsg( UIControl * Ctrl, const Uint32& Msg, const Uint32& Flags = 0 );

		Vector2i getMousePos();

		Input * getInput() const;

		const Uint32& getPressTrigger() const;

		const Uint32& getLastPressTrigger() const;

		void clipEnable( const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height );

		void clipDisable();

		void sendKeyUp( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod );

		void sendKeyDown( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod );

		void setHighlightFocus( bool Highlight );

		bool getHighlightFocus() const;

		void setHighlightFocusColor( const ColorA& Color );

		const ColorA& getHighlightFocusColor() const;

		void setHighlightOver( bool Highlight );

		bool getHighlightOver() const;

		void setHighlightOverColor( const ColorA& Color );

		const ColorA& getHighlightOverColor() const;

		void sendMouseClick( UIControl * ToCtrl, const Vector2i& Pos, const Uint32 Flags );

		void sendMouseUp( UIControl * ToCtrl, const Vector2i& Pos, const Uint32 Flags );

		void sendMouseDown( UIControl * ToCtrl, const Vector2i& Pos, const Uint32 Flags );

		EE::Window::Window * getWindow() const;

		/** Control where the mouse click started to be down */
		UIControl * getDownControl() const;

		UIControl * getLossFocusControl() const;

		const bool& isShootingDown() const;

		/** @return The position of the mouse when the event MouseDown was fired last time.
		**	Useful to compare the mouse position of the MouseClick event */
		const Vector2i& getMouseDownPos() const;

		void setControlDragging( bool dragging );

		const bool& isControlDragging() const;

		void setUseGlobalCursors( const bool& use );

		const bool& getUseGlobalCursors();

		void setCursor( EE_CURSOR_TYPE cursor );
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

		void				inputCallback( InputEvent * Event );

		void				checkTabPress( const Uint32& KeyCode );

		void				setActiveWindow( UIWindow * window );

		void				setFocusLastWindow( UIWindow * window  );

		void				windowAdd( UIWindow * win );

		void				windowRemove( UIWindow * win );

		bool				windowExists( UIWindow * win );

		void				addToCloseQueue( UIControl * Ctrl );

		void				checkClose();
};

}}

#endif
