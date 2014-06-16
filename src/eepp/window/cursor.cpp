#include <eepp/window/cursor.hpp>

namespace EE { namespace Window {

Cursor::Cursor( cTexture * tex, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	mId( String::Hash( name ) ),
	mName( name ),
	mImage( NULL ),
	mHotSpot( hotspot ),
	mWindow( window )
{
	if ( NULL != tex && tex->Lock() ) {
		mImage = eeNew( cImage, ( tex->GetPixelsPtr(), tex->Width(), tex->Height(), tex->Channels() ) );

		tex->Unlock();
	} else {
		eePRINTL( "Cursor::Cursor: Error creating cursor from cTexture." );
	}
}

Cursor::Cursor( cImage * img, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	mId( String::Hash( name ) ),
	mName( name ),
	mImage( NULL ),
	mHotSpot( hotspot ),
	mWindow( window )
{
	if ( img->MemSize() ) {
		mImage = eeNew( cImage, ( img->GetPixelsPtr(), img->Width(), img->Height(), img->Channels() ) );
	} else {
		eePRINTL( "Cursor::Cursor: Error creating cursor from cImage." );
	}
}

Cursor::Cursor( const std::string& path, const Vector2i& hotspot, const std::string& name, EE::Window::Window * window ) :
	mId( String::Hash( name ) ),
	mName( name ),
	mImage( NULL ),
	mHotSpot( hotspot ),
	mWindow( window )
{
	mImage = eeNew( cImage, ( path ) );

	if ( NULL == mImage->GetPixels() ) {
		eePRINTL( "Cursor::Cursor: Error creating cursor from path." );
	}
}

Cursor::~Cursor() {
	eeSAFE_DELETE( mImage );
}

const Vector2i& Cursor::HotSpot() const {
	return mHotSpot;
}

const Uint32& Cursor::Id() const {
	return mId;
}

const std::string& Cursor::Name() const {
	return mName;
}

cImage * Cursor::Image() const {
	return mImage;
}

}}
