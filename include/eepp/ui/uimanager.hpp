#ifndef EE_UICUIMANAGER_H
#define EE_UICUIMANAGER_H

#include <eepp/ui/uinode.hpp>
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

		UINode * getFocusControl() const;

		void setFocusControl( UINode * Ctrl );

		UINode * getOverControl() const;

		void setOverControl( UINode * Ctrl );

		void init( Uint32 Flags = 0, EE::Window::Window * window = NULL );

		void shutdown();

		void update();

		void update( const Time& elapsed );

		void draw();

		const Time& getElapsed() const;

		void resizeControl( EE::Window::Window * win );

		void sendMsg( UINode * Ctrl, const Uint32& Msg, const Uint32& Flags = 0 );

		Vector2i getMousePos();

		Vector2f getMousePosf();

		Input * getInput() const;

		const Uint32& getPressTrigger() const;

		const Uint32& getLastPressTrigger() const;

		void clipSmartEnable( UINode * ctrl, const Int32& x, const Int32& y, const Uint32& Width, const Uint32& Height );

		void clipSmartDisable( UINode * ctrl );

		void sendKeyUp( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod );

		void sendKeyDown( const Uint32& KeyCode, const Uint16& Char, const Uint32& Mod );

		void setHighlightFocus( bool Highlight );

		bool getHighlightFocus() const;

		void setHighlightInvalidation( bool Invalidation );

		bool getHighlightInvalidation() const;

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

		const Color& getHighlightInvalidationColor() const;

		void setHighlightInvalidationColor( const Color& highlightInvalidationColor );

		void setMainControlInFrameBuffer( const bool& set );

		bool isMainControlInFrameBuffer() const;

		void setMainControlInColorBuffer( const bool& set );

		bool isMainControlInColorBuffer() const;

		const Color& getHighlightOverColor() const;

		void sendMouseClick( UINode * ToCtrl, const Vector2i& Pos, const Uint32 Flags );

		void sendMouseUp( UINode * ToCtrl, const Vector2i& Pos, const Uint32 Flags );

		void sendMouseDown( UINode * ToCtrl, const Vector2i& Pos, const Uint32 Flags );

		EE::Window::Window * getWindow() const;

		/** Control where the mouse click started to be down */
		UINode * getDownControl() const;

		UINode * getLossFocusControl() const;

		const bool& isShootingDown() const;

		/** @return The position of the mouse when the event MouseDown was fired last time.
		**	Useful to compare the mouse position of the MouseClick event */
		const Vector2i& getMouseDownPos() const;

		void setControlDragging( bool dragging );

		const bool& isControlDragging() const;

		void setUseGlobalCursors( const bool& use );

		const bool& getUseGlobalCursors();

		void setCursor( EE_CURSOR_TYPE cursor );

		UIWidget * loadLayoutFromFile( const std::string& layoutPath, UINode * parent = NULL );

		UIWidget * loadLayoutFromString( const std::string& layoutString, UINode * parent = NULL );

		UIWidget * loadLayoutFromMemory( const void * buffer, Int32 bufferSize, UINode * parent = NULL );

		UIWidget * loadLayoutFromStream( IOStream& stream, UINode * parent = NULL );

		UIWidget * loadLayoutFromPack( Pack * pack, const std::string& FilePackPath, UINode * parent = NULL );

		UIWidget * loadLayoutNodes( pugi::xml_node node, UINode * parent );

		void setTranslator( Translator translator );

		Translator& getTranslator();

		String getTranslatorString( const std::string& str );

	protected:
		friend class UINode;
		friend class UIWindow;

		EE::Window::Window *mWindow;
		Input *				mKM;
		UIWindow *			mControl;
		UINode *			mFocusControl;
		UINode *			mOverControl;
		UINode * 		mDownControl;
		UINode *			mLossFocusControl;
		std::list<UIWindow*> mWindowsList;
		std::list<UINode*> mCloseList;
		Clock				mClock;

		Time	 			mElapsed;
		Int32 				mCbId;
		Uint32				mResizeCb;

		Uint32				mFlags;
		Color				mHighlightFocusColor;
		Color				mHighlightOverColor;
		Color				mHighlightInvalidationColor;
		Vector2i			mMousePos;
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

		void				addToCloseQueue( UINode * Ctrl );

		void				checkClose();
};

}}

#endif
