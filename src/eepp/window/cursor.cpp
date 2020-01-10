#include <eepp/graphics/image.hpp>
#include <eepp/graphics/texture.hpp>
#include <eepp/window/cursor.hpp>

namespace EE { namespace Window {

Cursor::Cursor( Texture* tex, const Vector2i& hotspot, const std::string& name,
				EE::Window::Window* window ) :
	mId( String::hash( name ) ),
	mName( name ),
	mImage( NULL ),
	mHotSpot( hotspot ),
	mWindow( window ) {
	if ( NULL != tex && tex->lock() ) {
		mImage = Graphics::Image::New( tex->getPixelsPtr(), tex->getWidth(), tex->getHeight(),
									   tex->getChannels() );

		tex->unlock();
	} else {
		eePRINTL( "Cursor::Cursor: Error creating cursor from Texture." );
	}
}

Cursor::Cursor( Graphics::Image* img, const Vector2i& hotspot, const std::string& name,
				EE::Window::Window* window ) :
	mId( String::hash( name ) ),
	mName( name ),
	mImage( NULL ),
	mHotSpot( hotspot ),
	mWindow( window ) {
	if ( img->getMemSize() ) {
		mImage = Graphics::Image::New( img->getPixelsPtr(), img->getWidth(), img->getHeight(),
									   img->getChannels() );
	} else {
		eePRINTL( "Cursor::Cursor: Error creating cursor from Image." );
	}
}

Cursor::Cursor( const std::string& path, const Vector2i& hotspot, const std::string& name,
				EE::Window::Window* window ) :
	mId( String::hash( name ) ),
	mName( name ),
	mImage( NULL ),
	mHotSpot( hotspot ),
	mWindow( window ) {
	mImage = Graphics::Image::New( path );

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

Cursor::Type Cursor::fromName( std::string name ) {
	String::toLowerInPlace( name );
	if ( "arrow" == name )
		return Arrow;
	if ( "hand" == name )
		return Hand;
	if ( "ibream" == name )
		return IBeam;
	if ( "wait" == name )
		return Wait;
	if ( "crosshair" == name )
		return Crosshair;
	if ( "waitarrow" == name )
		return WaitArrow;
	if ( "sizenwse" == name )
		return SizeNWSE;
	if ( "sizenesw" == name )
		return SizeNESW;
	if ( "sizewe" == name )
		return SizeWE;
	if ( "sizens" == name )
		return SizeNS;
	if ( "sizeall" == name )
		return SizeAll;
	if ( "nocursor" == name )
		return NoCursor;
	return Arrow;
}

const Uint32& Cursor::getId() const {
	return mId;
}

const std::string& Cursor::getName() const {
	return mName;
}

Graphics::Image* Cursor::getImage() const {
	return mImage;
}

}} // namespace EE::Window
