#ifndef EE_UICUIMANAGER_H
#define EE_UICUIMANAGER_H

#include <eepp/ui/uicontrol.hpp>
#include <eepp/ui/uiwindow.hpp>
#include <eepp/window/input.hpp>
#include <eepp/window/window.hpp>
#include <eepp/window/cursorhelper.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/translator.hpp>

namespace pugi {
class xml_node;
}

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

		void clipSmartEnable( UIControl * ctrl, const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height );

		void clipSmartDisable( UIControl * ctrl );

		void sendKeyUp( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod );

		void sendKeyDown( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod );

		void setHighlightFocus( bool Highlight );

		bool getHighlightFocus() const;

		void setDrawDebugData( bool debug );

		bool getDrawDebugData() const;

		void setDrawBoxes( bool draw );

		bool getDrawBoxes() const;

		void setHighlightFocusColor( const Color& Color );

		bool usesInvalidation();

		void setUseInvalidation( const bool& use );

		const Color& getHighlightFocusColor() const;

		void setHighlightOver( bool Highlight );

		bool getHighlightOver() const;

		void setHighlightOverColor( const Color& Color );

		void setMainControlInFrameBuffer( const bool& set );

		bool isMainControlInFrameBuffer() const;

		const Color& getHighlightOverColor() const;

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

		UIWidget * loadLayoutFromFile( const std::string& layoutPath, UIControl * parent = NULL );

		UIWidget * loadLayoutFromString( const std::string& layoutString, UIControl * parent = NULL );

		UIWidget * loadLayoutFromMemory( const void * buffer, Int32 bufferSize, UIControl * parent = NULL );

		UIWidget * loadLayoutFromStream( IOStream& stream, UIControl * parent = NULL );

		UIWidget * loadLayoutFromPack( Pack * pack, const std::string& FilePackPath, UIControl * parent = NULL );

		UIWidget * loadLayoutNodes( pugi::xml_node node, UIControl * parent );

		void setTranslator( Translator translator );

		Translator& getTranslator();

		String getTranslatorString( const std::string& str );
	protected:
		friend class UIControl;
		friend class UIWindow;

		EE::Window::Window *mWindow;
		Input *				mKM;
		UIWindow *			mControl;
		UIControl *			mFocusControl;
		UIControl *			mOverControl;
		UIControl * 		mDownControl;
		UIControl *			mLossFocusControl;
		std::list<UIWindow*> mWindowsList;
		std::list<UIControl*> mCloseList;

		Time	 			mElapsed;
		Int32 				mCbId;
		Uint32				mResizeCb;

		Uint32				mFlags;
		Color				mHighlightFocusColor;
		Color				mHighlightOverColor;
		Vector2i			mMouseDownPos;

		bool				mInit;
		bool 				mFirstPress;
		bool				mShootingDown;
		bool				mControlDragging;
		bool				mUseGlobalCursors;

		Translator			mTranslator;

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
