#include <eepp/window/cursor.hpp>

namespace EE { namespace Window {

Cursor::Cursor( Texture * tex, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	mId( String::hash( name ) ),
	mName( name ),
	mImage( NULL ),
	mHotSpot( hotspot ),
	mWindow( window )
{
	if ( NULL != tex && tex->lock() ) {
		mImage = eeNew( Graphics::Image, ( tex->getPixelsPtr(), tex->getWidth(), tex->getHeight(), tex->getChannels() ) );

		tex->unlock();
	} else {
		eePRINTL( "Cursor::Cursor: Error creating cursor from Texture." );
	}
}

Cursor::Cursor( Graphics::Image * img, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	mId( String::hash( name ) ),
	mName( name ),
	mImage( NULL ),
	mHotSpot( hotspot ),
	mWindow( window )
{
	if ( img->getMemSize() ) {
		mImage = eeNew( Graphics::Image, ( img->getPixelsPtr(), img->getWidth(), img->getHeight(), img->getChannels() ) );
	} else {
		eePRINTL( "Cursor::Cursor: Error creating cursor from Image." );
	}
}

Cursor::Cursor( const std::string& path, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	mId( String::hash( name ) ),
	mName( name ),
	mImage( NULL ),
	mHotSpot( hotspot ),
	mWindow( window )
{
	mImage = eeNew( Graphics::Image, ( path ) );

	if ( NULL == mImage->getPixels() ) {
		eePRINTL( "Cursor::Cursor: Error creating cursor from path." );
	}
}

Cursor::~Cursor() {
	eeSAFE_DELETE( mImage );
}

const Vector2i& Cursor::getHotSpot() const {
	return mHotSpot;
}

const Uint32& Cursor::getId() const {
	return mId;
}

const std::string& Cursor::getName() const {
	return mName;
}

Graphics::Image * Cursor::getImage() const {
	return mImage;
}

}}
