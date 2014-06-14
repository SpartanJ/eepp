#include <eepp/config.hpp>

#if defined( EE_X11_PLATFORM )

#include <X11/Xlib.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/cursorfont.h>
#undef Window
#undef Display
#undef Cursor

#include <eepp/window/platform/x11/ccursorx11.hpp>
#include <eepp/window/platform/x11/cx11impl.hpp>
#include <eepp/window/cwindow.hpp>

namespace EE { namespace Window { namespace Platform {

CursorX11::CursorX11( cTexture * tex, const Vector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	Cursor( tex, hotspot, name, window ),
	mCursor( None )
{
	Create();
}

CursorX11::CursorX11( cImage * img, const Vector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	Cursor( img, hotspot, name, window ),
	mCursor( None )
{
	Create();
}

CursorX11::CursorX11( const std::string& path, const Vector2i& hotspot, const std::string& name, Window::cWindow * window ) :
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

cX11Impl * CursorX11::GetPlatform() {
	return reinterpret_cast<cX11Impl*>( mWindow->GetPlatform() );
}

X11Cursor CursorX11::GetCursor() const {
	return mCursor;
}

}}}

#endif
