#include <eepp/window/platform/x11/ccursorx11.hpp>
#include <eepp/window/platform/x11/cx11impl.hpp>

#if defined( EE_X11_PLATFORM )

namespace EE { namespace Window { namespace Platform {

cCursorX11::cCursorX11( cTexture * tex, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	cCursor( tex, hotspot, name, window ),
	mCursor( None )
{
	Create();
}

cCursorX11::cCursorX11( cImage * img, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	cCursor( img, hotspot, name, window ),
	mCursor( None )
{
	Create();
}

cCursorX11::cCursorX11( const std::string& path, const eeVector2i& hotspot, const std::string& name, Window::cWindow * window ) :
	cCursor( path, hotspot, name, window ),
	mCursor( None )
{
	Create();
}

cCursorX11::~cCursorX11() {
	if ( None != mCursor )
		XFreeCursor( GetPlatform()->GetDisplay(), mCursor );
}

void cCursorX11::Create() {
	if ( NULL == mImage && mImage->Size() )
		return;

	XcursorImage * image;
	unsigned int c, ix, iy;

	image = XcursorImageCreate( mImage->Width(), mImage->Height() );

	if ( image == None )
	  return;

	c = 0;
	for ( iy = 0; iy < mImage->Height(); iy++ ) {
		for ( ix = 0; ix < mImage->Width(); ix++ ) {
			eeColorA C = mImage->GetPixel( ix, iy );

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

cX11Impl * cCursorX11::GetPlatform() {
	return reinterpret_cast<cX11Impl*>( mWindow->GetPlatform() );
}

X11Cursor cCursorX11::GetCursor() const {
	return mCursor;
}

}}}

#endif
