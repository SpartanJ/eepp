#include <eepp/graphics/cimage.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/image_helper.h>
#include <eepp/helper/SOIL2/src/SOIL2/stb_image.h>
#include <eepp/helper/SOIL2/src/SOIL2/SOIL2.h>
#include <eepp/helper/jpeg-compressor/jpge.h>

namespace EE { namespace Graphics {

Uint32 cImage::sJpegQuality = 85;

Uint32 cImage::JpegQuality() {
	return sJpegQuality;
}

void cImage::JpegQuality( Uint32 level ) {
	level = eemin<Uint32>( level, 100 );
	sJpegQuality = level;
}

std::string cImage::SaveTypeToExtension( const Int32& Format ) {
	switch( Format ) {
		case EE_SAVE_TYPE_TGA: return "tga";
		case EE_SAVE_TYPE_BMP: return "bmp";
		case EE_SAVE_TYPE_PNG: return "png";
		case EE_SAVE_TYPE_DDS: return "dds";
		case EE_SAVE_TYPE_JPG: return "jpg";
		case EE_SAVE_TYPE_UNKNOWN:
		default:
			break;
	}

	return "";
}

EE_SAVE_TYPE cImage::ExtensionToSaveType( const std::string& Extension ) {
	EE_SAVE_TYPE saveType = EE_SAVE_TYPE_UNKNOWN;

	if ( Extension == "tga" )		saveType = EE_SAVE_TYPE_TGA;
	else if ( Extension == "bmp" )	saveType = EE_SAVE_TYPE_BMP;
	else if ( Extension == "png" )	saveType = EE_SAVE_TYPE_PNG;
	else if ( Extension == "dds" )	saveType = EE_SAVE_TYPE_DDS;
	else if ( Extension == "jpg" || Extension == "jpeg" ) saveType = EE_SAVE_TYPE_JPG;

	return saveType;
}

bool cImage::GetInfo( const std::string& path, int * width, int * height, int * channels ) {
	return stbi_info( path.c_str(), width, height, channels ) != 0;
}

bool cImage::IsImage( const std::string& path ) {
	int w, h, c;
	return GetInfo( path, &w, &h, &c );
}

std::string cImage::GetLastFailureReason()
{
	return std::string( stbi_failure_reason() );
}

cImage::cImage() :
	mPixels(NULL),
	mWidth(0),
	mHeight(0),
	mChannels(0),
	mSize(0),
	mAvoidFree(false),
	mLoadedFromStbi(false)
{
}

cImage::cImage( const Uint8* data, const eeUint& Width, const eeUint& Height, const eeUint& Channels ) :
	mPixels(NULL),
	mWidth(Width),
	mHeight(Height),
	mChannels(Channels),
	mSize(0),
	mAvoidFree(false),
	mLoadedFromStbi(false)
{
	SetPixels( data );
}

cImage::cImage( const Uint32& Width, const Uint32& Height, const Uint32& Channels, const eeColorA& DefaultColor ) :
	mPixels(NULL),
	mWidth(Width),
	mHeight(Height),
	mChannels(Channels),
	mSize(0),
	mAvoidFree(false),
	mLoadedFromStbi(false)
{
	Create( Width, Height, Channels, DefaultColor );
}

cImage::cImage( Uint8* data, const eeUint& Width, const eeUint& Height, const eeUint& Channels ) :
	mPixels( data ),
	mWidth(Width),
	mHeight(Height),
	mChannels(Channels),
	mSize(Width*Height*Channels),
	mAvoidFree(false),
	mLoadedFromStbi(false)
{
}

cImage::cImage( std::string Path ) :
	mPixels(NULL),
	mWidth(0),
	mHeight(0),
	mChannels(0),
	mSize(0),
	mAvoidFree(false),
	mLoadedFromStbi(false)
{
	int w, h, c;
	cPack * tPack = NULL;
	Uint8 * data = stbi_load( Path.c_str(), &w, &h, &c, 0 );

	if ( NULL != data ) {
		mPixels		= data;
		mWidth		= (eeUint)w;
		mHeight		= (eeUint)h;
		mChannels	= (eeUint)c;

		mSize	= mWidth * mHeight * mChannels;

		mLoadedFromStbi = true;
	} else if ( cPackManager::instance()->FallbackToPacks() && NULL != ( tPack = cPackManager::instance()->Exists( Path ) ) ) {
		LoadFromPack( tPack, Path );
	} else {
		std::string reason = ".";

		if ( NULL != stbi_failure_reason() ) {
			reason = ", reason: " + std::string( stbi_failure_reason() );
		}

		cLog::instance()->Write( "Failed to load image" + reason );
	}
}

cImage::cImage( cPack * Pack, std::string FilePackPath ) :
	mPixels(NULL),
	mWidth(0),
	mHeight(0),
	mChannels(0),
	mSize(0),
	mAvoidFree(false),
	mLoadedFromStbi(false)
{
	LoadFromPack( Pack, FilePackPath );
}

cImage::~cImage() {
	if ( !mAvoidFree )
		ClearCache();
}

void cImage::LoadFromPack( cPack * Pack, const std::string& FilePackPath ) {
	if ( NULL != Pack && Pack->IsOpen() && -1 != Pack->Exists( FilePackPath ) ) {
		SafeDataPointer PData;

		Pack->ExtractFileToMemory( FilePackPath, PData );

		int w, h, c;
		Uint8 * data = stbi_load_from_memory( PData.Data, PData.DataSize, &w, &h, &c, 0 );

		if ( NULL != data ) {
			mPixels		= data;
			mWidth		= (eeUint)w;
			mHeight		= (eeUint)h;
			mChannels	= (eeUint)c;

			mSize	= mWidth * mHeight * mChannels;

			mLoadedFromStbi = true;
		} else {
			cLog::instance()->Write( "Failed to load image, reason: " + std::string( stbi_failure_reason() ) );
		}
	} else {
		cLog::instance()->Write( "Failed to load image " + FilePackPath + " from pack." );
	}
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
		return eeColorA::Transparent;
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

void cImage::Create( const Uint32& Width, const Uint32& Height, const Uint32& Channels, const eeColorA& DefaultColor ) {
	mWidth 		= Width;
	mHeight 	= Height;
	mChannels 	= Channels;

	Allocate( mWidth * mHeight * mChannels );
}

Uint8* cImage::GetPixels() const {
	return mPixels;
}

void cImage::Allocate( const Uint32& size, eeColorA DefaultColor ) {
	ClearCache();

	mPixels = eeNewArray( unsigned char, size );

	Int32 c = (Int32)DefaultColor.GetUint32();

	memset( mPixels, c, size );

	mSize 	= size;
}

eeUint cImage::MemSize() const {
	return mSize;
}

eeSize cImage::Size() {
	return eeSize( mWidth, mHeight );
}

void cImage::ClearCache() {
	if ( mLoadedFromStbi ) {
		free( mPixels );
	} else {
		eeSAFE_DELETE_ARRAY( mPixels );
	}
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
		if ( EE_SAVE_TYPE_JPG != Format ) {
			Res = 0 != ( SOIL_save_image ( filepath.c_str(), Format, (Int32)mWidth, (Int32)mHeight, mChannels, GetPixelsPtr() ) );
		} else {
			jpge::params params;
			params.m_quality = JpegQuality();
			Res = jpge::compress_image_to_jpeg_file( filepath.c_str(), mWidth, mHeight, mChannels, GetPixelsPtr(), params);
		}
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
