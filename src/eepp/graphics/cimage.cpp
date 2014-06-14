#include <eepp/graphics/cimage.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/helper/SOIL2/src/SOIL2/image_helper.h>
#include <eepp/helper/SOIL2/src/SOIL2/stb_image.h>
#include <eepp/helper/SOIL2/src/SOIL2/SOIL2.h>
#include <eepp/helper/jpeg-compressor/jpge.h>
#include <eepp/helper/imageresampler/resampler.h>

namespace EE { namespace Graphics {

static const char * get_resampler_name( EE_RESAMPLER_FILTER filter ) {
	switch ( filter )
	{
		case RESAMPLER_BOX: return "box";
		case RESAMPLER_TENT: return "tent";
		case RESAMPLER_BELL: return "bell";
		case RESAMPLER_BSPLINE: return "b-spline";
		case RESAMPLER_MITCHELL: return "mitchell";
		case RESAMPLER_LANCZOS3: return "lanczos3";
		case RESAMPLER_BLACKMAN: return "blackman";
		case RESAMPLER_LANCZOS4: return "lanczos4";
		case RESAMPLER_LANCZOS6: return "lanczos6";
		case RESAMPLER_LANCZOS12: return "lanczos12";
		case RESAMPLER_KAISER: return "kaiser";
		case RESAMPLER_GAUSSIAN: return "gaussian";
		case RESAMPLER_CATMULLROM: return "catmullrom";
		case RESAMPLER_QUADRATIC_INTERP: return "quadratic_interp";
		case RESAMPLER_QUADRATIC_APPROX: return "quadratic_approx";
		case RESAMPLER_QUADRATIC_MIX: return "quadratic_mix";
	}

	return "lanczos4";
}

static unsigned char * resample_image( unsigned char* pSrc_image, int src_width, int src_height, int n, int dst_width, int dst_height, EE_RESAMPLER_FILTER filter ) {
	const int max_components = 4;

	if ((std::max(src_width, src_height) > RESAMPLER_MAX_DIMENSION) || (n > max_components))
	{
		return NULL;
	}

	// Partial gamma correction looks better on mips. Set to 1.0 to disable gamma correction.
	const float source_gamma = 1.0f;

	// Filter scale - values < 1.0 cause aliasing, but create sharper looking mips.
	const float filter_scale = 1.0f;//.75f;

	const char* pFilter = get_resampler_name( filter );

	float srgb_to_linear[256];
	for (int i = 0; i < 256; ++i)
		srgb_to_linear[i] = (float)pow(i * 1.0f/255.0f, source_gamma);

	const int linear_to_srgb_table_size = 4096;
	unsigned char linear_to_srgb[linear_to_srgb_table_size];

	const float inv_linear_to_srgb_table_size = 1.0f / linear_to_srgb_table_size;
	const float inv_source_gamma = 1.0f / source_gamma;

	for (int i = 0; i < linear_to_srgb_table_size; ++i)
	{
		int k = (int)(255.0f * pow(i * inv_linear_to_srgb_table_size, inv_source_gamma) + .5f);
		if (k < 0) k = 0; else if (k > 255) k = 255;
		linear_to_srgb[i] = (unsigned char)k;
	}

	Resampler* resamplers[max_components];
	std::vector<float> samples[max_components];

	resamplers[0] = new Resampler(src_width, src_height, dst_width, dst_height, Resampler::BOUNDARY_CLAMP, 0.0f, 1.0f, pFilter, NULL, NULL, filter_scale, filter_scale);
	samples[0].resize(src_width);
	for (int i = 1; i < n; i++)
	{
		resamplers[i] = new Resampler(src_width, src_height, dst_width, dst_height, Resampler::BOUNDARY_CLAMP, 0.0f, 1.0f, pFilter, resamplers[0]->get_clist_x(), resamplers[0]->get_clist_y(), filter_scale, filter_scale);
		samples[i].resize(src_width);
	}

	unsigned char* dst_image = eeNewArray( unsigned char, (dst_width * n * dst_height) );

	const int src_pitch = src_width * n;
	const int dst_pitch = dst_width * n;
	int dst_y = 0;

	for (int src_y = 0; src_y < src_height; src_y++)
	{
		const unsigned char* pSrc = &pSrc_image[src_y * src_pitch];

		for (int x = 0; x < src_width; x++)
		{
			for (int c = 0; c < n; c++)
			{
				if ((c == 3) || ((n == 2) && (c == 1)))
					samples[c][x] = *pSrc++ * (1.0f/255.0f);
				else
					samples[c][x] = srgb_to_linear[*pSrc++];
			}
		}

		for (int c = 0; c < n; c++)
		{
			if (!resamplers[c]->put_line(&samples[c][0]))
			{
				return NULL;
			}
		}

		for ( ; ; )
		{
			int c;
			for (c = 0; c < n; c++)
			{
				const float* pOutput_samples = resamplers[c]->get_line();
				if (!pOutput_samples)
					break;

				const bool alpha_channel = (c == 3) || ((n == 2) && (c == 1));
				eeASSERT(dst_y < dst_height);
				unsigned char* pDst = &dst_image[dst_y * dst_pitch + c];

				for (int x = 0; x < dst_width; x++)
				{
					if (alpha_channel)
					{
						int c = (int)(255.0f * pOutput_samples[x] + .5f);
						if (c < 0) c = 0; else if (c > 255) c = 255;
							*pDst = (unsigned char)c;
					}
					else
					{
						int j = (int)(linear_to_srgb_table_size * pOutput_samples[x] + .5f);
						if (j < 0) j = 0; else if (j >= linear_to_srgb_table_size) j = linear_to_srgb_table_size - 1;
						*pDst = linear_to_srgb[j];
					}

					pDst += n;
				}
			}
			if (c < n)
				break;

			dst_y++;
		}
	}

	return dst_image;
}

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

EE_PIXEL_FORMAT cImage::ChannelsToPixelFormat( const Uint32& channels ) {
	EE_PIXEL_FORMAT pf = PF_RGBA;;

	if ( 3 == channels )
		pf = PF_RGB;
	else if ( 2 == channels )
		pf = PF_RG;
	else if ( 1 == channels )
		pf = PF_RED;

	return pf;
}

bool cImage::GetInfo( const std::string& path, int * width, int * height, int * channels ) {
	bool res = stbi_info( path.c_str(), width, height, channels ) != 0;

	if ( !res && PackManager::instance()->FallbackToPacks() ) {
		std::string npath( path );
		Pack * tPack = PackManager::instance()->Exists( npath );

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

cImage::cImage( cImage * image ) :
	mPixels(NULL),
	mWidth(image->mWidth),
	mHeight(image->mHeight),
	mChannels(image->mChannels),
	mSize(image->mSize),
	mAvoidFree(image->mAvoidFree),
	mLoadedFromStbi(image->mLoadedFromStbi)
{
	SetPixels( image->GetPixelsPtr() );
}

cImage::cImage( const Uint8* data, const unsigned int& Width, const unsigned int& Height, const unsigned int& Channels ) :
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

cImage::cImage( const Uint32& Width, const Uint32& Height, const Uint32& Channels, const eeColorA& DefaultColor, const bool& initWithDefaultColor ) :
	mPixels(NULL),
	mWidth(Width),
	mHeight(Height),
	mChannels(Channels),
	mSize(0),
	mAvoidFree(false),
	mLoadedFromStbi(false)
{
	Create( Width, Height, Channels, DefaultColor, initWithDefaultColor );
}

cImage::cImage( Uint8* data, const unsigned int& Width, const unsigned int& Height, const unsigned int& Channels ) :
	mPixels( data ),
	mWidth(Width),
	mHeight(Height),
	mChannels(Channels),
	mSize(Width*Height*Channels),
	mAvoidFree(false),
	mLoadedFromStbi(false)
{
}

cImage::cImage( std::string Path, const unsigned int& forceChannels ) :
	mPixels(NULL),
	mWidth(0),
	mHeight(0),
	mChannels(forceChannels),
	mSize(0),
	mAvoidFree(false),
	mLoadedFromStbi(false)
{
	int w, h, c;
	Pack * tPack = NULL;
	Uint8 * data = stbi_load( Path.c_str(), &w, &h, &c, mChannels );

	if ( NULL == data ) {
		data = stbi_load( ( Sys::GetProcessPath() + Path ).c_str(), &w, &h, &c, mChannels );
	}

	if ( NULL != data ) {
		mPixels		= data;
		mWidth		= (unsigned int)w;
		mHeight		= (unsigned int)h;

		if ( STBI_default == mChannels )
			mChannels	= (unsigned int)c;

		mSize	= mWidth * mHeight * mChannels;

		mLoadedFromStbi = true;
	} else if ( PackManager::instance()->FallbackToPacks() && NULL != ( tPack = PackManager::instance()->Exists( Path ) ) ) {
		LoadFromPack( tPack, Path );
	} else {
		std::string reason = ".";

		if ( NULL != stbi_failure_reason() ) {
			reason = ", reason: " + std::string( stbi_failure_reason() );
		}

		eePRINTL( "Failed to load image %s. Reason: %s", Path.c_str(), reason.c_str() );
	}
}

cImage::cImage( Pack * Pack, std::string FilePackPath, const unsigned int& forceChannels ) :
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

void cImage::LoadFromPack( Pack * Pack, const std::string& FilePackPath ) {
	if ( NULL != Pack && Pack->IsOpen() && -1 != Pack->Exists( FilePackPath ) ) {
		SafeDataPointer PData;

		Pack->ExtractFileToMemory( FilePackPath, PData );

		int w, h, c;
		Uint8 * data = stbi_load_from_memory( PData.Data, PData.DataSize, &w, &h, &c, mChannels );

		if ( NULL != data ) {
			mPixels		= data;
			mWidth		= (unsigned int)w;
			mHeight		= (unsigned int)h;

			if ( STBI_default == mChannels )
				mChannels	= (unsigned int)c;

			mSize	= mWidth * mHeight * mChannels;

			mLoadedFromStbi = true;
		} else {
			eePRINTL( "Failed to load image %s. Reason: %s", stbi_failure_reason(), FilePackPath.c_str() );
		}
	} else {
		eePRINTL( "Failed to load image %s from pack.", FilePackPath.c_str() );
	}
}

void cImage::SetPixels( const Uint8* data ) {
	if ( data != NULL ) {
		Allocate( mWidth * mHeight * mChannels, eeColorA(0,0,0,0), false );

		memcpy( reinterpret_cast<void*>( &mPixels[0] ), reinterpret_cast<const void*> ( data ), mSize );
	}
}

const Uint8* cImage::GetPixelsPtr() {
	return reinterpret_cast<const Uint8*> (&mPixels[0]);
}

eeColorA cImage::GetPixel( const unsigned int& x, const unsigned int& y ) {
	eeASSERT( !( mPixels == NULL || x > mWidth || y > mHeight ) );
	eeColorA dst;
	memcpy( &dst, &mPixels[ ( ( x + y * mWidth ) * mChannels ) ], mChannels );
	return dst;
}

void cImage::SetPixel(const unsigned int& x, const unsigned int& y, const eeColorA& Color) {
	eeASSERT( !( mPixels == NULL || x > mWidth || y > mHeight ) );
	memcpy( &mPixels[ ( ( x + y * mWidth ) * mChannels ) ], &Color, mChannels );
}

void cImage::Create( const Uint32& Width, const Uint32& Height, const Uint32& Channels, const eeColorA& DefaultColor, const bool& initWithDefaultColor ) {
	mWidth 		= Width;
	mHeight 	= Height;
	mChannels 	= Channels;

	Allocate( mWidth * mHeight * mChannels, DefaultColor, initWithDefaultColor );
}

Uint8* cImage::GetPixels() const {
	return mPixels;
}

void cImage::Allocate( const Uint32& size, eeColorA DefaultColor, bool memsetData ) {
	ClearCache();

	mPixels = eeNewArray( unsigned char, size );
	mSize 	= size;

	if ( memsetData ) {
		memset( mPixels, (int)DefaultColor.GetValue(), size );
	}
}

unsigned int cImage::MemSize() const {
	return mSize;
}

eeSize cImage::Size() {
	return eeSize( mWidth, mHeight );
}

void cImage::ClearCache() {
	if ( mLoadedFromStbi ) {
		if ( NULL != mPixels )
			free( mPixels );
	} else {
		eeSAFE_DELETE_ARRAY( mPixels );
	}
}

void cImage::Width( const unsigned int& width ) {
	mWidth = width;
}

unsigned int cImage::Width() const {
	return mWidth;
}

void cImage::Height( const unsigned int& height ) {
	mHeight = height;
}

unsigned int cImage::Height() const {
	return mHeight;
}

void cImage::Channels( const unsigned int& channels ) {
	mChannels = channels;
}

unsigned int cImage::Channels() const {
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
	unsigned int Pos = 0;

	if ( NULL == mPixels )
		return;

	unsigned int size = mWidth * mHeight;

	for ( unsigned int i = 0; i < size; i++ ) {
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

	unsigned int z;
	unsigned int size = mWidth * mHeight;

	for ( unsigned int i = 0; i < size; i += mChannels ) {
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
		unsigned int dWidth 	= image->Width();
		unsigned int dHeight 	= image->Height();

		if ( mChannels != image->Channels() ) {
			for ( unsigned int ty = 0; ty < dHeight; ty++ ) {
				for ( unsigned int tx = 0; tx < dWidth; tx++ ) {
					SetPixel( x + tx, y + ty, image->GetPixel( tx, ty ) );
				}
			}
		} else {
			// Copy per row
			for ( unsigned int ty = 0; ty < dHeight; ty++ ) {
				Uint8 *			pDst	= &mPixels[ ( x + ( ( ty + y ) * mWidth ) ) * mChannels ];
				const Uint8 *	pSrc	= &( ( image->GetPixelsPtr() )[ ( ty * dWidth ) * mChannels  ] );

				memcpy( pDst, pSrc, mChannels * dWidth );
			}
		}
	}
}

void cImage::Resize( const Uint32 &newWidth, const Uint32 &newHeight , EE_RESAMPLER_FILTER filter ) {
	if ( NULL != mPixels && mWidth != newWidth && mHeight != newHeight ) {

		unsigned char * resampled = resample_image( mPixels, mWidth, mHeight, mChannels, newWidth, newHeight, filter );

		if ( NULL != resampled ) {
			ClearCache();

			mPixels 	= resampled;
			mWidth 		= newWidth;
			mHeight 	= newHeight;
		}
	}
}

void cImage::Scale( const Float& scale , EE_RESAMPLER_FILTER filter ) {
	if ( 1.f == scale )
		return;

	Int32 new_width 	= (Int32)( (Float)mWidth * scale );
	Int32 new_height 	= (Int32)( (Float)mHeight * scale );

	Resize( new_width, new_height, filter );
}

cImage * cImage::Thumbnail( const Uint32& maxWidth, const Uint32& maxHeight, EE_RESAMPLER_FILTER filter ) {
	if ( NULL != mPixels ) {
		Float iScaleX 	= ( (Float)maxWidth / (Float)mWidth );
		Float iScaleY 	= ( (Float)maxHeight / (Float)mHeight );
		Float iScale		= ( iScaleY < iScaleX ) ? iScaleY : iScaleX;
		Int32 new_width 	= (Int32)( (Float)mWidth * iScale );
		Int32 new_height 	= (Int32)( (Float)mHeight * iScale );

		unsigned char * resampled = resample_image( mPixels, mWidth, mHeight, mChannels, new_width, new_height, filter );

		if ( NULL != resampled ) {
			return eeNew( cImage, ( (Uint8*)resampled, new_width, new_height, mChannels ) );
		}
	}

	return NULL;
}

cImage * cImage::Crop( eeRecti rect ) {
	if ( rect.Left >= 0 && rect.Right <= (Int32)mWidth && rect.Top >= 0 && rect.Bottom <= (Int32)mHeight ) {
		cImage * img = eeNew( cImage, ( rect.Size().Width(), rect.Size().Height(), mChannels ) );

		// Copy per row
		for ( unsigned int ty = 0; ty < img->mHeight; ty++ ) {
			Uint8 *			pDst	= &img->mPixels[ ( ty * img->mWidth ) * mChannels ];
			const Uint8 *	pSrc	= &mPixels[ ( rect.Left + ( ( ty + rect.Top ) * mWidth ) ) * mChannels  ];

			memcpy( pDst, pSrc, mChannels * img->mWidth );
		}

		return img;
	}

	return NULL;
}

void cImage::Flip() {
	if ( NULL != mPixels ) {
		cImage tImg( mHeight, mWidth, mChannels );

		for ( unsigned int y = 0; y < mHeight; y++ )
			for ( unsigned int x = 0; x < mWidth; x++ )
				tImg.SetPixel( y, x, GetPixel( x, mHeight - 1 - y ) );

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
		unsigned int dh = eemin( mHeight	, y	+ image->Height() );
		unsigned int dw = eemin( mWidth	, x	+ image->Width() );

		for ( unsigned int ty = y; ty < dh; ty++ ) {
			for ( unsigned int tx = x; tx < dw; tx++ ) {
				eeColorA ts( image->GetPixel( tx - x, ty - y ) );
				eeColorA td( GetPixel( tx, ty ) );

				SetPixel( tx, ty, Color::Blend( ts, td ) );
			}
		}
	}
}

cImage * cImage::Copy() {
	return eeNew( cImage, ( this ) );
}

cImage &cImage::operator =(const cImage &right) {
	mWidth = right.mWidth;
	mHeight = right.mHeight;
	mChannels = right.mChannels;
	mSize = right.mSize;
	mAvoidFree = right.mAvoidFree;
	mLoadedFromStbi = right.mLoadedFromStbi;

	ClearCache();

	if ( NULL != right.mPixels ) {
		SetPixels( right.mPixels );
	}

	return *this;
}

}}
