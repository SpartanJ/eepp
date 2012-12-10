#ifndef EE_WINDOWCPLATFORMIMPL_HPP
#define EE_WINDOWCPLATFORMIMPL_HPP

#include <eepp/window/base.hpp>
#include <eepp/window/cursorhelper.hpp>

namespace EE {

namespace Graphics {
class cTexture;
class cImage;
}

namespace Window {
class cWindow;
class cCursor;
}

}

namespace EE { namespace Window { namespace Platform {

using namespace EE::Window;

/** This is an abstraction of some platform specific implementations
* It's not garanteed that everything is implemented in every platform.
* X11 and Win32 implementation are complete, OS X implementation still lacks of most features.
*/
class cPlatformImpl {
	public:
		cPlatformImpl( Window::cWindow * window );
		
		virtual ~cPlatformImpl();
		
		/** Minimize the window */
		virtual void MinimizeWindow() = 0;

		/** Maximize the window */
		virtual void MaximizeWindow() = 0;

		/** @return true if the window is maximized */
		virtual bool IsWindowMaximized() = 0;

		/** Hide the window */
		virtual void HideWindow() = 0;

		/** Raise the window */
		virtual void RaiseWindow() = 0;

		/** Show the window */
		virtual void ShowWindow() = 0;

		/** Move the window to the desired position
		* @param left Move to the x-axis position
		* @param top Move to the y-axis position
		*/
		virtual void MoveWindow( int left, int top ) = 0;
		
		/** Set the GL context as the current context */
		virtual void SetContext( eeWindowContex Context ) = 0;

		/** @return The current window position */
		virtual eeVector2i Position() = 0;

		/** Force to show the mouse cursor */
		virtual void ShowMouseCursor() = 0;

		/** Hide the mouse cursor */
		virtual void HideMouseCursor() = 0;

		/** Creates a cursor from a texture
		* @param tex The texture pointer to use as cursor
		* @param hotspot The hotspot where the mouse click is taken
		* @param name The name of the cursor
		*/
		virtual cCursor * CreateMouseCursor( cTexture * tex, const eeVector2i& hotspot, const std::string& name ) = 0;

		/** Creates a cursor from a image
		* @param img The image path
		* @param hotspot The hotspot where the mouse click is taken
		* @param name The name of the cursor
		*/
		virtual cCursor * CreateMouseCursor( cImage * img, const eeVector2i& hotspot, const std::string& name ) = 0;

		/** Creates a cursor from a image path
		* @param path The image pointer to use as cursor
		* @param hotspot The hotspot where the mouse click is taken
		* @param name The name of the cursor
		*/
		virtual cCursor * CreateMouseCursor( const std::string& path, const eeVector2i& hotspot, const std::string& name ) = 0;

		/** Set the the current cursor by its cursor pointer */
		virtual void SetMouseCursor( cCursor * cursor ) = 0;

		/** Set the cursor using a system cursor */
		virtual void SetSystemMouseCursor( Cursor::EE_SYSTEM_CURSOR syscursor ) = 0;

		/** Force to reset the state of the current seted cursor */
		virtual void RestoreCursor() = 0;
	protected:
		cWindow *	mWindow;
};

}}}

#endif
