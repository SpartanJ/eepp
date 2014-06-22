#ifndef EE_WINDOWCPLATFORMIMPL_HPP
#define EE_WINDOWCPLATFORMIMPL_HPP

#include <eepp/core.hpp>

#include <eepp/math/vector2.hpp>
using namespace EE::Math;

#include <eepp/window/windowhandle.hpp>
#include <eepp/window/windowcontext.hpp>
#include <eepp/window/cursorhelper.hpp>

namespace EE {

namespace Graphics {
class Texture;
class Image;
}

namespace Window {
class Window;
class Cursor;
}

}

namespace EE { namespace Window { namespace Platform {

using namespace EE::Window;
using namespace EE::Graphics;

/** This is an abstraction of some platform specific implementations
* It's not garanteed that everything is implemented in every platform.
* X11 and Win32 implementation are complete, OS X implementation still lacks of most features.
*/
class PlatformImpl {
	public:
		PlatformImpl( EE::Window::Window * window );
		
		virtual ~PlatformImpl();
		
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
		virtual Vector2i Position() = 0;

		/** Force to show the mouse cursor */
		virtual void ShowMouseCursor() = 0;

		/** Hide the mouse cursor */
		virtual void HideMouseCursor() = 0;

		/** Creates a cursor from a texture
		* @param tex The texture pointer to use as cursor
		* @param hotspot The hotspot where the mouse click is taken
		* @param name The name of the cursor
		*/
		virtual Cursor * CreateMouseCursor( Texture * tex, const Vector2i& hotspot, const std::string& name ) = 0;

		/** Creates a cursor from a image
		* @param img The image path
		* @param hotspot The hotspot where the mouse click is taken
		* @param name The name of the cursor
		*/
		virtual Cursor * CreateMouseCursor( Image * img, const Vector2i& hotspot, const std::string& name ) = 0;

		/** Creates a cursor from a image path
		* @param path The image pointer to use as cursor
		* @param hotspot The hotspot where the mouse click is taken
		* @param name The name of the cursor
		*/
		virtual Cursor * CreateMouseCursor( const std::string& path, const Vector2i& hotspot, const std::string& name ) = 0;

		/** Set the the current cursor by its cursor pointer */
		virtual void SetMouseCursor( Cursor * cursor ) = 0;

		/** Set the cursor using a system cursor */
		virtual void SetSystemMouseCursor( EE_SYSTEM_CURSOR syscursor ) = 0;

		/** Force to reset the state of the current seted cursor */
		virtual void RestoreCursor() = 0;

		virtual eeWindowContex GetWindowContext() = 0;
	protected:
		EE::Window::Window *	mWindow;
};

}}}

#endif
