#include <eepp/window/cursor.hpp>

namespace EE { namespace Window {

Cursor::Cursor( Texture * tex, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	mId( String::hash( name ) ),
	mName( name ),
	mImage( NULL ),
	mHotSpot( hotspot ),
	mWindow( window )
{
	if ( NULL != tex && tex->Lock() ) {
		mImage = eeNew( Graphics::Image, ( tex->GetPixelsPtr(), tex->Width(), tex->Height(), tex->Channels() ) );

		tex->Unlock();
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
	if ( img->MemSize() ) {
		mImage = eeNew( Graphics::Image, ( img->GetPixelsPtr(), img->Width(), img->Height(), img->Channels() ) );
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

	if ( NULL == mImage->GetPixels() ) {
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
