#include "cimage.hpp"
#include "../helper/SOIL/image_helper.h"
#include "../helper/SOIL/SOIL.h"

namespace EE { namespace Graphics {

cImage::cImage() :
	mPixels(NULL),
	mWidth(0),
	mHeight(0),
	mChannels(0),
	mSize(0),
	mAvoidFree(false)
{
}

cImage::cImage( const Uint8* data, const eeUint& Width, const eeUint& Height, const eeUint& Channels ) :
	mPixels(NULL),
	mWidth(Width),
	mHeight(Height),
	mChannels(Channels),
	mSize(0),
	mAvoidFree(false)
{
	SetPixels( data );
}

cImage::cImage( const Uint32& Width, const Uint32& Height, const Uint32& Channels ) :
	mPixels(NULL),
	mWidth(Width),
	mHeight(Height),
	mChannels(Channels),
	mSize(0),
	mAvoidFree(false)
{
	Create( Width, Height, Channels );
}

cImage::cImage( Uint8* data, const eeUint& Width, const eeUint& Height, const eeUint& Channels ) :
	mPixels( data ),
	mWidth(Width),
	mHeight(Height),
	mChannels(Channels),
	mSize(Width*Height*Channels),
	mAvoidFree(false)
{
}

cImage::cImage( const std::string& Path ) :
	mPixels(NULL),
	mWidth(0),
	mHeight(0),
	mChannels(0),
	mSize(0),
	mAvoidFree(false)
{
	int w, h, c;
	Uint8 * data = SOIL_load_image( Path.c_str(), &w, &h, &c, SOIL_LOAD_AUTO );

	if ( NULL != data ) {
		mPixels		= data;
		mWidth		= (eeUint)w;
		mHeight		= (eeUint)h;
		mChannels	= (eeUint)c;

		mSize	= mWidth * mHeight * mChannels;

		//! HACK: This is a hack to make the memory manager recognize the allocated data
		#ifdef EE_MEMORY_MANAGER
		MemoryManager::AddPointer( cAllocatedPointer( (void*)data, __FILE__, __LINE__, mSize ) );
		#endif
	}
}

cImage::~cImage() {
	if ( !mAvoidFree )
		ClearCache();
}

void cImage::SetPixels( const Uint8* data ) {
	if ( data != NULL ) {
		eeUint size = (eeUint)mWidth * (eeUint)mHeight * mChannels;

		Allocate( size );

		memcpy( reinterpret_cast<void*>( &mPixels[0] ), reinterpret_cast<const void*> ( data ), size );
	}
}

const Uint8* cImage::GetPixelsPtr() {
	return reinterpret_cast<const Uint8*> (&mPixels[0]);
}

eeColorA cImage::GetPixel( const eeUint& x, const eeUint& y ) {
	if ( mPixels == NULL || x > mWidth || y > mHeight ) {
		return eeColorA::Black;
	}

	eeUint Pos = ( x + y * mWidth ) * mChannels;

	if ( 4 == mChannels ) {
		return eeColorA( mPixels[ Pos ], mPixels[ Pos + 1 ], mPixels[ Pos + 2 ], mPixels[ Pos + 3 ] );
	} else if ( 3 == mChannels )
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

Uint8* cImage::GetPixels() const {
	return mPixels;
}

void cImage::Allocate( const Uint32& size ) {
	ClearCache();

	mPixels = eeNewArray( unsigned char, size );

	memset( mPixels, 0, size );

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

bool cImage::SaveToFile( const std::string& filepath, const EE_SAVE_TYPE& Format ) {
	bool Res = false;

	if ( NULL != mPixels && 0 != mWidth && 0 != mHeight && 0 != mChannels ) {
		Res = 0 != ( SOIL_save_image ( filepath.c_str(), Format, (Int32)mWidth, (Int32)mHeight, mChannels, GetPixelsPtr() ) );
	}

	return Res;
}

void cImage::ReplaceColor( const eeColorA& ColorKey, const eeColorA& NewColor ) {
	eeUint Pos = 0;

	if ( NULL == mPixels )
		return;

	for ( eeUint i = 0; i < mWidth * mHeight; i++ ) {
		Pos = i * mChannels;

		if ( 4 == mChannels ) {
			if ( mPixels[ Pos ] == ColorKey.R() && mPixels[ Pos + 1 ] == ColorKey.G() && mPixels[ Pos + 2 ] == ColorKey.B() && mPixels[ Pos + 3 ] == ColorKey.A() ) {
				mPixels[ Pos ] 		= NewColor.R();
				mPixels[ Pos + 1 ]	= NewColor.G();
				mPixels[ Pos + 2 ]	= NewColor.B();
				mPixels[ Pos + 3 ]	= NewColor.A();
			}
		} else if ( 3 == mChannels ) {
			if ( mPixels[ Pos ] == ColorKey.R() && mPixels[ Pos + 1 ] == ColorKey.G() && mPixels[ Pos + 2 ] == ColorKey.B() ) {
				mPixels[ Pos ] 		= NewColor.R();
				mPixels[ Pos + 1 ]	= NewColor.G();
				mPixels[ Pos + 2 ]	= NewColor.B();
			}
		} else if ( 2 == mChannels ) {
			if ( mPixels[ Pos ] == ColorKey.R() && mPixels[ Pos + 1 ] == ColorKey.G() ) {
				mPixels[ Pos ] 		= NewColor.R();
				mPixels[ Pos + 1 ]	= NewColor.G();
			}
		} else if ( 1 == mChannels ) {
			if ( mPixels[ Pos ] == ColorKey.R() ) {
				mPixels[ Pos ] 		= NewColor.R();
			}
		}
	}
}

void cImage::CreateMaskFromColor( const eeColorA& ColorKey, Uint8 Alpha ) {
	ReplaceColor( ColorKey, eeColorA( ColorKey.R(), ColorKey.G(), ColorKey.B(), Alpha ) );
}

void cImage::CreateMaskFromColor( const eeColor& ColorKey, Uint8 Alpha ) {
	CreateMaskFromColor( eeColorA( ColorKey.R(), ColorKey.G(), ColorKey.B(), 255 ), Alpha );
}

void cImage::FillWithColor( const eeColorA& Color ) {
	if ( NULL == mPixels )
		return;

	eeUint z;

	for ( eeUint i = 0; i < mWidth * mHeight; i += mChannels ) {
		for ( z = 0; z < mChannels; z++ ) {
			if ( 0 == z )
				mPixels[ i + z ] = Color.R();
			else if ( 1 == z )
				mPixels[ i + z ] = Color.G();
			else if ( 2 == z )
				mPixels[ i + z ] = Color.B();
			else if ( 3 == z )
				mPixels[ i + z ] = Color.A();
		}
	}
}

void cImage::CopyImage( cImage * Img, const eeUint& x, const eeUint& y ) {
	if ( NULL != mPixels && NULL != Img->GetPixels() && mWidth >= x + Img->Width() && mHeight >= y + Img->Height() ) {
		eeUint dWidth 	= Img->Width();
		eeUint dHeight 	= Img->Height();

		for ( eeUint ty = 0; ty < dHeight; ty++ ) {
			for ( eeUint tx = 0; tx < dWidth; tx++ ) {
				SetPixel( x + tx, y + ty, Img->GetPixel( tx, ty ) );
			}
		}
	}
}

void cImage::Resize( const eeUint& new_width, const eeUint& new_height ) {
	if ( NULL != mPixels && mWidth != new_width && mHeight != new_height ) {
		unsigned char * resampled = eeNewArray( unsigned char, mChannels * new_width * new_height );

		int res = up_scale_image( reinterpret_cast<const unsigned char*> ( mPixels ), mWidth, mHeight, mChannels, resampled, new_width, new_height );

		if ( res ) {
			ClearCache();

			mPixels 	= resampled;
			mWidth 		= new_width;
			mHeight 	= new_height;
		} else
			eeSAFE_DELETE_ARRAY( resampled );
	}
}

void cImage::Scale( const eeFloat& scale ) {
	if ( 1.f == scale )
		return;

	Int32 new_width 	= (Int32)( (eeFloat)mWidth * scale );
	Int32 new_height 	= (Int32)( (eeFloat)mHeight * scale );

	Resize( new_width, new_height );
}

cImage * cImage::Thumbnail( const eeUint& max_width, const eeUint& max_height ) {
	if ( NULL != mPixels && mWidth > max_width && mHeight > max_height ) {
		eeFloat iScaleX 	= ( (eeFloat)max_width / (eeFloat)mWidth );
		eeFloat iScaleY 	= ( (eeFloat)max_height / (eeFloat)mHeight );
		eeFloat iScale		= ( iScaleY < iScaleX ) ? iScaleY : iScaleX;
		Int32 new_width 	= (Int32)( (eeFloat)mWidth * iScale );
		Int32 new_height 	= (Int32)( (eeFloat)mHeight * iScale );

		unsigned char * resampled = eeNewArray( unsigned char, mChannels * new_width * new_height );

		int res = up_scale_image( reinterpret_cast<const unsigned char*> ( mPixels ), mWidth, mHeight, mChannels, resampled, new_width, new_height );

		if ( res ) {
			return eeNew( cImage, ( (Uint8*)resampled, new_width, new_height, mChannels ) );
		} else {
			eeSAFE_DELETE_ARRAY( resampled );
		}
	}

	return NULL;
}

void cImage::Flip() {
	if ( NULL != mPixels ) {
		cImage tImg( mHeight, mWidth, mChannels );

		for ( eeUint y = 0; y < mHeight; y++ )
			for ( eeUint x = 0; x < mWidth; x++ )
				tImg.SetPixel( y, x, GetPixel( x, y ) );

		ClearCache();

		mPixels = tImg.GetPixels();
		mWidth 	= tImg.Width();
		mHeight = tImg.Height();

		tImg.AvoidFreeImage( true );
	}
}

}}
