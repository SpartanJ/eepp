#include "cimage.hpp"

namespace EE { namespace Graphics {

cImage::cImage() :
	mPixels(NULL),
	mWidth(0),
	mHeight(0),
	mChannels(0),
	mSize(0)
{
}

cImage::cImage( const Uint8* data, const eeUint& Width, const eeUint& Height, const eeUint& Channels ) :
	mPixels(NULL),
	mWidth(Width),
	mHeight(Height),
	mChannels(Channels),
	mSize(0)
{
	SetPixels( data );
}

cImage::cImage( const Uint32& Width, const Uint32& Height, const Uint32& Channels ) :
	mPixels(NULL),
	mWidth(Width),
	mHeight(Height),
	mChannels(Channels),
	mSize(0)
{
	Create( Width, Height, Channels );
}

cImage::~cImage() {
	ClearCache();
}

const Uint8* cImage::GetPixelsPtr() {
	return reinterpret_cast<const Uint8*> (&mPixels[0]);
}

eeColorA cImage::GetPixel( const eeUint& x, const eeUint& y ) {
	if ( mPixels == NULL || x > mWidth || y > mHeight ) {
		return eeColorA::Black;
	}

	eeUint Pos = ( x + y * mWidth ) * mChannels;

	if ( 4 == mChannels )
		return eeColorA( mPixels[ Pos ], mPixels[ Pos + 1 ], mPixels[ Pos + 2 ], mPixels[ Pos + 3 ] );
	else if ( 3 == mChannels )
		return eeColorA( mPixels[ Pos ], mPixels[ Pos + 1 ], mPixels[ Pos + 2 ], 255 );
	else if ( 2 == mChannels )
		return eeColorA( mPixels[ Pos ], mPixels[ Pos + 1 ], 255, 255 );
	else
		return eeColorA( mPixels[ Pos ], 255, 255, 255 );
}

void cImage::SetPixel(const eeUint& x, const eeUint& y, const eeColorA& Color) {
	if ( mPixels == NULL || x > mWidth || y > mHeight ) {
		return;
	}

	eeUint Pos = ( x + y * mWidth ) * mChannels;

	if ( mChannels >= 1 ) mPixels[ Pos ]		= Color.R();
	if ( mChannels >= 2 ) mPixels[ Pos + 1 ]	= Color.G();
	if ( mChannels >= 3 ) mPixels[ Pos + 2 ]	= Color.B();
	if ( mChannels >= 4 ) mPixels[ Pos + 3 ]	= Color.A();
}

void cImage::Create( const Uint32& Width, const Uint32& Height, const Uint32& Channels ) {
	mWidth 		= Width;
	mHeight 	= Height;
	mChannels 	= Channels;

	Allocate( mWidth * mHeight * mChannels );
}

void cImage::SetPixels( const Uint8* data ) {
	if ( data != NULL ) {
		eeUint size = (eeUint)mWidth * (eeUint)mHeight * mChannels;

		Allocate( size );

		memcpy( reinterpret_cast<void*>( &mPixels[0] ), reinterpret_cast<const void*> ( data ), size );
	}
}

Uint8* cImage::GetPixels() const {
	return mPixels;
}

void cImage::Allocate( const Uint32& size ) {
	ClearCache();

	mPixels = new unsigned char[ size ];
	mSize 	= size;
}

eeUint cImage::Size() const {
	return mSize;
}

void cImage::ClearCache() {
	eeSAFE_DELETE_ARRAY( mPixels );
}

void cImage::Width( const eeUint& width ) {
	mWidth = width;
}

eeUint cImage::Width() const {
	return mWidth;
}

void cImage::Height( const eeUint& height ) {
	mHeight = height;
}

eeUint cImage::Height() const {
	return mHeight;
}

void cImage::Channels( const eeUint& channels ) {
	mChannels = channels;
}

eeUint cImage::Channels() const {
	return mChannels;
}

bool cImage::SaveToFile( const std::string& filepath, const EE_SAVETYPE& Format ) {
	bool Res = false;

	if ( NULL != mPixels && 0 != mWidth && 0 != mHeight && 0 != mChannels ) {
		Res = 0 != ( SOIL_save_image ( filepath.c_str(), Format, (Int32)mWidth, (Int32)mHeight, mChannels, GetPixelsPtr() ) );
	}

	return Res;
}

}}
