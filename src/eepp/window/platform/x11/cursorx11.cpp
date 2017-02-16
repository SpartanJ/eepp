#include <eepp/config.hpp>

#if defined( EE_X11_PLATFORM )

#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/cursorfont.h>
#undef Window
#undef Display
#undef Cursor

#include <eepp/window/platform/x11/cursorx11.hpp>
#include <eepp/window/platform/x11/x11impl.hpp>
#include <eepp/window/window.hpp>

namespace EE { namespace Window { namespace Platform {

CursorX11::CursorX11( Texture * tex, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	Cursor( tex, hotspot, name, window ),
	mCursor( None )
{
	create();
}

CursorX11::CursorX11( Graphics::Image * img, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	Cursor( img, hotspot, name, window ),
	mCursor( None )
{
	create();
}

CursorX11::CursorX11( const std::string& path, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	Cursor( path, hotspot, name, window ),
	mCursor( None )
{
	create();
}

CursorX11::~CursorX11() {
	if ( None != mCursor )
		XFreeCursor( getPlatform()->GetDisplay(), mCursor );
}

void CursorX11::create() {
	if ( NULL == mImage || 0 == mImage->memSize() )
		return;

	XcursorImage * image;
	unsigned int c, ix, iy;

	image = XcursorImageCreate( mImage->width(), mImage->height() );

	if ( image == None )
	  return;

	c = 0;
	for ( iy = 0; iy < mImage->height(); iy++ ) {
		for ( ix = 0; ix < mImage->width(); ix++ ) {
			ColorA C = mImage->getPixel( ix, iy );

			image->pixels[c++] = ( C.a() << 24 ) | ( C.r() << 16 ) | ( C.g() <<8 ) | ( C.b() );
		}
	}

	image->xhot = mHotSpot.x;
	image->yhot = mHotSpot.y;

	getPlatform()->Lock();

	mCursor = XcursorImageLoadCursor( getPlatform()->GetDisplay(), image );

	getPlatform()->Unlock();

	XcursorImageDestroy( image );
}

X11Impl * CursorX11::getPlatform() {
	return reinterpret_cast<X11Impl*>( mWindow->getPlatform() );
}

X11Cursor CursorX11::GetCursor() const {
	return mCursor;
}

}}}

#endif
