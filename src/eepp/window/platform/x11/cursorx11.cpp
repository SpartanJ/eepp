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
	Create();
}

CursorX11::CursorX11( Graphics::Image * img, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	Cursor( img, hotspot, name, window ),
	mCursor( None )
{
	Create();
}

CursorX11::CursorX11( const std::string& path, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	Cursor( path, hotspot, name, window ),
	mCursor( None )
{
	Create();
}

CursorX11::~CursorX11() {
	if ( None != mCursor )
		XFreeCursor( GetPlatform()->GetDisplay(), mCursor );
}

void CursorX11::Create() {
	if ( NULL == mImage || 0 == mImage->MemSize() )
		return;

	XcursorImage * image;
	unsigned int c, ix, iy;

	image = XcursorImageCreate( mImage->Width(), mImage->Height() );

	if ( image == None )
	  return;

	c = 0;
	for ( iy = 0; iy < mImage->Height(); iy++ ) {
		for ( ix = 0; ix < mImage->Width(); ix++ ) {
			ColorA C = mImage->GetPixel( ix, iy );

			image->pixels[c++] = ( C.A() << 24 ) | ( C.R() << 16 ) | ( C.G() <<8 ) | ( C.B() );
		}
	}

	image->xhot = mHotSpot.x;
	image->yhot = mHotSpot.y;

	GetPlatform()->Lock();

	mCursor = XcursorImageLoadCursor( GetPlatform()->GetDisplay(), image );

	GetPlatform()->Unlock();

	XcursorImageDestroy( image );
}

X11Impl * CursorX11::GetPlatform() {
	return reinterpret_cast<X11Impl*>( mWindow->GetPlatform() );
}

X11Cursor CursorX11::GetCursor() const {
	return mCursor;
}

}}}

#endif
