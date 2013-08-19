#include <eepp/graphics/cimage.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/clog.hpp>
#include <eepp/system/cpack.hpp>
#include <eepp/system/cpackmanager.hpp>
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
		case SAVE_TYPE_TGA: return "tga";
		case SAVE_TYPE_BMP: return "bmp";
		case SAVE_TYPE_PNG: return "png";
		case SAVE_TYPE_DDS: return "dds";
		case SAVE_TYPE_JPG: return "jpg";
		case SAVE_TYPE_UNKNOWN:
		default:
			break;
	}

	return "";
}

EE_SAVE_TYPE cImage::ExtensionToSaveType( const std::string& Extension ) {
	EE_SAVE_TYPE saveType = SAVE_TYPE_UNKNOWN;

	if ( Extension == "tga" )		saveType = SAVE_TYPE_TGA;
	else if ( Extension == "bmp" )	saveType = SAVE_TYPE_BMP;
	else if ( Extension == "png" )	saveType = SAVE_TYPE_PNG;
	else if ( Extension == "dds" )	saveType = SAVE_TYPE_DDS;
	else if ( Extension == "jpg" || Extension == "jpeg" ) saveType = SAVE_TYPE_JPG;

	return saveType;
}

bool cImage::GetInfo( const std::string& path, int * width, int * height, int * channels ) {
	bool res = stbi_info( path.c_str(), width, height, channels ) != 0;

	if ( !res && cPackManager::instance()->FallbackToPacks() ) {
		std::string npath( path );
		cPack * tPack = cPackManager::instance()->Exists( npath );

		if ( NULL != tPack ) {
			SafeDataPointer PData;

			tPack->ExtractFileToMemory( npath, PData );

			res = 0 != stbi_info_from_memory( PData.Data, PData.DataSize, width, height, channels );
		}
	}

	return res;
}

bool cImage::IsImage( const std::string& path ) {
	return STBI_unknown != stbi_test( path.c_str() );
}

std::string cImage::GetLastFailureReason() {
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

cImage::cImage( std::string Path, const eeUint& forceChannels ) :
	mPixels(NULL),
	mWidth(0),
	mHeight(0),
	mChannels(forceChannels),
	mSize(0),
	mAvoidFree(false),
	mLoadedFromStbi(false)
{
	int w, h, c;
	cPack * tPack = NULL;
	Uint8 * data = stbi_load( Path.c_str(), &w, &h, &c, mChannels );

	if ( NULL == data ) {
		data = stbi_load( ( Sys::GetProcessPath() + Path ).c_str(), &w, &h, &c, mChannels );
	}

	if ( NULL != data ) {
		mPixels		= data;
		mWidth		= (eeUint)w;
		mHeight		= (eeUint)h;

		if ( STBI_default == mChannels )
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

		cLog::instance()->Write( "Failed to load image " + Path + reason );
	}
}

cImage::cImage( cPack * Pack, std::string FilePackPath, const eeUint& forceChannels ) :
	mPixels(NULL),
	mWidth(0),
	mHeight(0),
	mChannels(forceChannels),
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
		Uint8 * data = stbi_load_from_memory( PData.Data, PData.DataSize, &w, &h, &c, mChannels );

		if ( NULL != data ) {
			mPixels		= data;
			mWidth		= (eeUint)w;
			mHeight		= (eeUint)h;

			if ( STBI_default == mChannels )
				mChannels	= (eeUint)c;

			mSize	= mWidth * mHeight * mChannels;

			mLoadedFromStbi = true;
		} else {
			cLog::instance()->Write( "Failed to load image " + FilePackPath + ", reason: " + std::string( stbi_failure_reason() ) );
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
	eeASSERT( !( mPixels == NULL || x > mWidth || y > mHeight ) );
	eeColorA dst;
	memcpy( &dst, &mPixels[ ( ( x + y * mWidth ) * mChannels ) ], mChannels );
	return dst;
}

void cImage::SetPixel(const eeUint& x, const eeUint& y, const eeColorA& Color) {
	eeASSERT( !( mPixels == NULL || x > mWidth || y > mHeight ) );
	memcpy( &mPixels[ ( ( x + y * mWidth ) * mChannels ) ], &Color, mChannels );
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

	Int32 c = (Int32)DefaultColor.GetValue();

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

	std::string fpath( FileSystem::FileRemoveFileName( filepath ));

	if ( !FileSystem::IsDirectory( fpath ) )
		FileSystem::MakeDir( fpath );

	if ( NULL != mPixels && 0 != mWidth && 0 != mHeight && 0 != mChannels ) {
		if ( SAVE_TYPE_JPG != Format ) {
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

	eeUint size = mWidth * mHeight;

	for ( eeUint i = 0; i < size; i++ ) {
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
	eeUint size = mWidth * mHeight;

	for ( eeUint i = 0; i < size; i += mChannels ) {
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

void cImage::CopyImage( cImage * image, const Uint32& x, const Uint32& y ) {
	if ( NULL != mPixels && NULL != image->GetPixels() && mWidth >= x + image->Width() && mHeight >= y + image->Height() ) {
		eeUint dWidth 	= image->Width();
		eeUint dHeight 	= image->Height();

		for ( eeUint ty = 0; ty < dHeight; ty++ ) {
			for ( eeUint tx = 0; tx < dWidth; tx++ ) {
				SetPixel( x + tx, y + ty, image->GetPixel( tx, ty ) );
			}
		}
	}
}

void cImage::Resize(const Uint32 &newWidth, const Uint32 &newHeight ) {
	if ( NULL != mPixels && mWidth != newWidth && mHeight != newHeight ) {
		unsigned char * resampled = eeNewArray( unsigned char, mChannels * newWidth * newHeight );

		int res = up_scale_image( reinterpret_cast<const unsigned char*> ( mPixels ), mWidth, mHeight, mChannels, resampled, newWidth, newHeight );

		if ( res ) {
			ClearCache();

			mPixels 	= resampled;
			mWidth 		= newWidth;
			mHeight 	= newHeight;
		} else {
			eeSAFE_DELETE_ARRAY( resampled );
		}
	}
}

void cImage::Scale( const eeFloat& scale ) {
	if ( 1.f == scale )
		return;

	Int32 new_width 	= (Int32)( (eeFloat)mWidth * scale );
	Int32 new_height 	= (Int32)( (eeFloat)mHeight * scale );

	Resize( new_width, new_height );
}

cImage * cImage::Thumbnail( const Uint32& maxWidth, const Uint32& maxHeight ) {
	if ( NULL != mPixels && mWidth > maxWidth && mHeight > maxHeight ) {
		eeFloat iScaleX 	= ( (eeFloat)maxWidth / (eeFloat)mWidth );
		eeFloat iScaleY 	= ( (eeFloat)maxHeight / (eeFloat)mHeight );
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

void cImage::AvoidFreeImage( const bool& AvoidFree ) {
	mAvoidFree = AvoidFree;
}

void cImage::Blit( cImage * image, const Uint32& x, const Uint32& y ) {
	if ( NULL != image && NULL != image->GetPixelsPtr() && x < mWidth && y < mHeight ) {
		eeUint dh = eemin( mHeight	, y	+ image->Height() );
		eeUint dw = eemin( mWidth	, x	+ image->Width() );

		for ( eeUint ty = y; ty < dh; ty++ ) {
			for ( eeUint tx = x; tx < dw; tx++ ) {
				eeColorA ts( image->GetPixel( tx - x, ty - y ) );
				eeColorA td( GetPixel( tx, ty ) );

				SetPixel( tx, ty, Color::Blend( ts, td ) );
			}
		}
	}
}

}}
