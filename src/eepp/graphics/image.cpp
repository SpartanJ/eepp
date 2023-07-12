#include <SOIL2/src/SOIL2/SOIL2.h>
#include <SOIL2/src/SOIL2/image_helper.h>
#include <SOIL2/src/SOIL2/stb_image.h>
#include <algorithm>
#include <eepp/graphics/image.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/stbi_iocb.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/packmanager.hpp>
#include <imageresampler/resampler.h>
#include <jpeg-compressor/jpge.h>
#include <memory>

#define NANOSVG_IMPLEMENTATION
#include <nanosvg/nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvg/nanosvgrast.h>

namespace EE { namespace Graphics {

static const char* get_resampler_name( Image::ResamplerFilter filter ) {
	switch ( filter ) {
		case Image::ResamplerFilter::RESAMPLER_BOX:
			return "box";
		case Image::ResamplerFilter::RESAMPLER_TENT:
			return "tent";
		case Image::ResamplerFilter::RESAMPLER_BELL:
			return "bell";
		case Image::ResamplerFilter::RESAMPLER_BSPLINE:
			return "b-spline";
		case Image::ResamplerFilter::RESAMPLER_MITCHELL:
			return "mitchell";
		case Image::ResamplerFilter::RESAMPLER_LANCZOS3:
			return "lanczos3";
		case Image::ResamplerFilter::RESAMPLER_BLACKMAN:
			return "blackman";
		case Image::ResamplerFilter::RESAMPLER_LANCZOS4:
			return "lanczos4";
		case Image::ResamplerFilter::RESAMPLER_LANCZOS6:
			return "lanczos6";
		case Image::ResamplerFilter::RESAMPLER_LANCZOS12:
			return "lanczos12";
		case Image::ResamplerFilter::RESAMPLER_KAISER:
			return "kaiser";
		case Image::ResamplerFilter::RESAMPLER_GAUSSIAN:
			return "gaussian";
		case Image::ResamplerFilter::RESAMPLER_CATMULLROM:
			return "catmullrom";
		case Image::ResamplerFilter::RESAMPLER_QUADRATIC_INTERP:
			return "quadratic_interp";
		case Image::ResamplerFilter::RESAMPLER_QUADRATIC_APPROX:
			return "quadratic_approx";
		case Image::ResamplerFilter::RESAMPLER_QUADRATIC_MIX:
			return "quadratic_mix";
	}

	return "lanczos4";
}

static unsigned char* resample_image( unsigned char* pSrc_image, int src_width, int src_height,
									  int n, int dst_width, int dst_height,
									  Image::ResamplerFilter filter ) {
	const int max_components = 4;

	if ( ( std::max( src_width, src_height ) > RESAMPLER_MAX_DIMENSION ) ||
		 ( n > max_components ) ) {
		return NULL;
	}

	// Partial gamma correction looks better on mips. Set to 1.0 to disable gamma correction.
	const float source_gamma = 1.0f;

	// Filter scale - values < 1.0 cause aliasing, but create sharper looking mips.
	const float filter_scale = 1.0f; //.75f;

	const char* pFilter = get_resampler_name( filter );

	float srgb_to_linear[256];
	for ( int i = 0; i < 256; ++i )
		srgb_to_linear[i] = (float)pow( i * 1.0f / 255.0f, source_gamma );

	const int linear_to_srgb_table_size = 4096;
	unsigned char linear_to_srgb[linear_to_srgb_table_size];

	const float inv_linear_to_srgb_table_size = 1.0f / linear_to_srgb_table_size;
	const float inv_source_gamma = 1.0f / source_gamma;

	for ( int i = 0; i < linear_to_srgb_table_size; ++i ) {
		int k = (int)( 255.0f * pow( i * inv_linear_to_srgb_table_size, inv_source_gamma ) + .5f );
		if ( k < 0 )
			k = 0;
		else if ( k > 255 )
			k = 255;
		linear_to_srgb[i] = (unsigned char)k;
	}

	std::unique_ptr<Resampler> resamplers[max_components];
	std::vector<float> samples[max_components];

	resamplers[0] = std::unique_ptr<Resampler>(
		new Resampler( src_width, src_height, dst_width, dst_height, Resampler::BOUNDARY_CLAMP,
					   0.0f, 1.0f, pFilter, NULL, NULL, filter_scale, filter_scale ) );
	samples[0].resize( src_width );
	for ( int i = 1; i < n; i++ ) {
		resamplers[i] = std::unique_ptr<Resampler>(
			new Resampler( src_width, src_height, dst_width, dst_height, Resampler::BOUNDARY_CLAMP,
						   0.0f, 1.0f, pFilter, resamplers[0]->get_clist_x(),
						   resamplers[0]->get_clist_y(), filter_scale, filter_scale ) );
		samples[i].resize( src_width );
	}

	unsigned char* dst_image = eeNewArray( unsigned char, ( dst_width * n * dst_height ) );

	const int src_pitch = src_width * n;
	const int dst_pitch = dst_width * n;
	int dst_y = 0;

	for ( int src_y = 0; src_y < src_height; src_y++ ) {
		const unsigned char* pSrc = &pSrc_image[src_y * src_pitch];

		for ( int x = 0; x < src_width; x++ ) {
			for ( int c = 0; c < n; c++ ) {
				if ( ( c == 3 ) || ( ( n == 2 ) && ( c == 1 ) ) )
					samples[c][x] = *pSrc++ * ( 1.0f / 255.0f );
				else
					samples[c][x] = srgb_to_linear[*pSrc++];
			}
		}

		for ( int c = 0; c < n; c++ ) {
			if ( !resamplers[c]->put_line( &samples[c][0] ) ) {
				return NULL;
			}
		}

		for ( ;; ) {
			int c;
			for ( c = 0; c < n; c++ ) {
				const float* pOutput_samples = resamplers[c]->get_line();
				if ( !pOutput_samples )
					break;

				const bool alpha_channel = ( c == 3 ) || ( ( n == 2 ) && ( c == 1 ) );
				eeASSERT( dst_y < dst_height );
				unsigned char* pDst = &dst_image[dst_y * dst_pitch + c];

				for ( int x = 0; x < dst_width; x++ ) {
					if ( alpha_channel ) {
						int c = (int)( 255.0f * pOutput_samples[x] + .5f );
						if ( c < 0 )
							c = 0;
						else if ( c > 255 )
							c = 255;
						*pDst = (unsigned char)c;
					} else {
						int j = (int)( linear_to_srgb_table_size * pOutput_samples[x] + .5f );
						if ( j < 0 )
							j = 0;
						else if ( j >= linear_to_srgb_table_size )
							j = linear_to_srgb_table_size - 1;
						*pDst = linear_to_srgb[j];
					}

					pDst += n;
				}
			}
			if ( c < n )
				break;

			dst_y++;
		}
	}

	return dst_image;
}

static bool svg_test( const std::string& path ) {
	return FileSystem::fileExtension( path ) == "svg";
}

static bool svg_test_from_memory( const Uint8* imageData, const unsigned int& imageDataSize ) {
	return imageDataSize > 5 &&
		   ( ( imageData[0] == '<' && imageData[1] == 's' && imageData[2] == 'v' &&
			   imageData[3] == 'g' ) ||
			 ( imageData[0] == '<' && imageData[1] == '?' && imageData[2] == 'x' &&
			   imageData[3] == 'm' && imageData[4] == 'l' ) );
}

static bool svg_test_from_stream( IOStream& stream ) {
	if ( stream.isOpen() ) {
		std::string str;
		str.resize( 5 );

		stream.seek( 0 );

		stream.read( (char*)&str[0], 5 );

		String::toLowerInPlace( str );

		if ( ( str[0] == '<' && str[1] == 's' && str[2] == 'v' && str[3] == 'g' ) ||
			 ( str[0] == '<' && str[1] == '?' && str[2] == 'x' && str[3] == 'm' &&
			   str[4] == 'l' ) ) {
			return true;
		}
	}

	return false;
}

Image* Image::New() {
	return eeNew( Image, () );
}

Image* Image::New( Graphics::Image* image ) {
	return eeNew( Image, ( image ) );
}

Image* Image::New( Uint8* data, const unsigned int& width, const unsigned int& height,
				   const unsigned int& channels ) {
	return eeNew( Image, ( data, width, height, channels ) );
}

Image* Image::New( const Uint8* data, const unsigned int& width, const unsigned int& height,
				   const unsigned int& channels ) {
	return eeNew( Image, ( data, width, height, channels ) );
}

Image* Image::New( const Uint32& width, const Uint32& height, const Uint32& channels,
				   const Color& DefaultColor, const bool& initWithDefaultColor ) {
	return eeNew( Image, ( width, height, channels, DefaultColor, initWithDefaultColor ) );
}

Image* Image::New( std::string Path, const unsigned int& forceChannels,
				   const FormatConfiguration& formatConfiguration ) {
	return eeNew( Image, ( Path, forceChannels, formatConfiguration ) );
}

Image* Image::New( const Uint8* imageData, const unsigned int& imageDataSize,
				   const unsigned int& forceChannels,
				   const FormatConfiguration& formatConfiguration ) {
	return eeNew( Image, ( imageData, imageDataSize, forceChannels, formatConfiguration ) );
}

Image* Image::New( Pack* Pack, std::string FilePackPath, const unsigned int& forceChannels,
				   const FormatConfiguration& formatConfiguration ) {
	return eeNew( Image, ( Pack, FilePackPath, forceChannels, formatConfiguration ) );
}

Image* Image::New( IOStream& stream, const unsigned int& forceChannels,
				   const FormatConfiguration& formatConfiguration ) {
	return eeNew( Image, ( stream, forceChannels, formatConfiguration ) );
}

std::string Image::saveTypeToExtension( const Int32& Format ) {
	switch ( Format ) {
		case Image::SaveType::SAVE_TYPE_TGA:
			return "tga";
		case Image::SaveType::SAVE_TYPE_BMP:
			return "bmp";
		case Image::SaveType::SAVE_TYPE_PNG:
			return "png";
		case Image::SaveType::SAVE_TYPE_DDS:
			return "dds";
		case Image::SaveType::SAVE_TYPE_JPG:
			return "jpg";
		case Image::SaveType::SAVE_TYPE_QOI:
			return "qoi";
		case Image::SaveType::SAVE_TYPE_UNKNOWN:
		default:
			break;
	}

	return "";
}

Image::SaveType Image::extensionToSaveType( const std::string& Extension ) {
	SaveType saveType = SaveType::SAVE_TYPE_UNKNOWN;

	if ( Extension == "tga" )
		saveType = SaveType::SAVE_TYPE_TGA;
	else if ( Extension == "bmp" )
		saveType = SaveType::SAVE_TYPE_BMP;
	else if ( Extension == "png" )
		saveType = SaveType::SAVE_TYPE_PNG;
	else if ( Extension == "dds" )
		saveType = SaveType::SAVE_TYPE_DDS;
	else if ( Extension == "jpg" || Extension == "jpeg" )
		saveType = SaveType::SAVE_TYPE_JPG;
	else if ( Extension == "qoi" )
		saveType = SaveType::SAVE_TYPE_QOI;

	return saveType;
}

Image::PixelFormat Image::channelsToPixelFormat( const Uint32& channels ) {
	PixelFormat pf = PixelFormat::PIXEL_FORMAT_RGBA;

	if ( 3 == channels )
		pf = PixelFormat::PIXEL_FORMAT_RGB;
	else if ( 2 == channels )
		pf = PixelFormat::PIXEL_FORMAT_RG;
	else if ( 1 == channels )
		pf = PixelFormat::PIXEL_FORMAT_RED;

	return pf;
}

bool Image::getInfo( const std::string& path, int* width, int* height, int* channels,
					 const FormatConfiguration& imageFormatConfiguration ) {
	bool res = stbi_info( path.c_str(), width, height, channels ) != 0;

	if ( !res && svg_test( path ) ) {
		NSVGimage* image = nsvgParseFromFile( path.c_str(), "px", 96.0f );

		if ( NULL != image ) {
			*width = image->width * imageFormatConfiguration.svgScale();
			*height = image->height * imageFormatConfiguration.svgScale();
			*channels = 4;

			nsvgDelete( image );

			res = true;
		}
	} else if ( !res && PackManager::instance()->isFallbackToPacksActive() ) {
		std::string npath( path );
		Pack* tPack = PackManager::instance()->exists( npath );

		if ( NULL != tPack ) {
			ScopedBuffer buffer;

			tPack->extractFileToMemory( npath, buffer );

			res = 0 !=
				  stbi_info_from_memory( buffer.get(), buffer.length(), width, height, channels );

			if ( !res && svg_test_from_memory( buffer.get(), buffer.length() ) ) {
				ScopedBuffer data( buffer.length() + 1 );
				memcpy( data.get(), buffer.get(), buffer.length() );
				data[buffer.length()] = '\0';

				NSVGimage* image = nsvgParse( (char*)data.get(), "px", 96.0f );

				if ( NULL != image ) {
					*width = image->width * imageFormatConfiguration.svgScale();
					*height = image->height * imageFormatConfiguration.svgScale();
					*channels = 4;

					nsvgDelete( image );

					res = true;
				}
			}
		}
	}

	return res;
}

bool Image::getInfoFromMemory( const unsigned char* data, const size_t& dataSize, int* width,
							   int* height, int* channels,
							   const FormatConfiguration& imageFormatConfiguration ) {
	bool res = stbi_info_from_memory( data, dataSize, width, height, channels ) != 0;

	if ( !res && svg_test_from_memory( data, dataSize ) ) {
		ScopedBuffer sdata( dataSize + 1 );
		memcpy( sdata.get(), data, dataSize );
		sdata[dataSize] = '\0';
		NSVGimage* image = nsvgParse( (char*)sdata.get(), "px", 96.0f );

		if ( NULL != image ) {
			*width = image->width * imageFormatConfiguration.svgScale();
			*height = image->height * imageFormatConfiguration.svgScale();
			*channels = 4;

			nsvgDelete( image );

			res = true;
		}
	}

	return res;
}

bool Image::isImage( const std::string& path ) {
	return STBI_unknown != stbi_test( path.c_str() ) || svg_test( path );
}

bool Image::isImageExtension( const std::string& path ) {
	const std::string ext( FileSystem::fileExtension( path ) );
	return ( ext == "png" || ext == "tga" || ext == "bmp" || ext == "jpg" || ext == "gif" ||
			 ext == "jpeg" || ext == "dds" || ext == "psd" || ext == "hdr" || ext == "pic" ||
			 ext == "pvr" || ext == "pkm" || ext == "svg" || ext == "qoi" );
}

std::string Image::getLastFailureReason() {
	return std::string( stbi_failure_reason() );
}

Image::Image() :
	mPixels( NULL ),
	mWidth( 0 ),
	mHeight( 0 ),
	mChannels( 0 ),
	mSize( 0 ),
	mAvoidFree( false ),
	mLoadedFromStbi( false ) {}

Image::Image( Graphics::Image* image ) :
	mPixels( NULL ),
	mWidth( image->mWidth ),
	mHeight( image->mHeight ),
	mChannels( image->mChannels ),
	mSize( image->mSize ),
	mAvoidFree( image->mAvoidFree ),
	mLoadedFromStbi( image->mLoadedFromStbi ) {
	setPixels( image->getPixelsPtr() );
}

Image::Image( const Uint8* data, const unsigned int& Width, const unsigned int& Height,
			  const unsigned int& Channels ) :
	mPixels( NULL ),
	mWidth( Width ),
	mHeight( Height ),
	mChannels( Channels ),
	mSize( 0 ),
	mAvoidFree( false ),
	mLoadedFromStbi( false ) {
	setPixels( data );
}

Image::Image( const Uint32& Width, const Uint32& Height, const Uint32& Channels,
			  const Color& DefaultColor, const bool& initWithDefaultColor ) :
	mPixels( NULL ),
	mWidth( Width ),
	mHeight( Height ),
	mChannels( Channels ),
	mSize( 0 ),
	mAvoidFree( false ),
	mLoadedFromStbi( false ) {
	create( Width, Height, Channels, DefaultColor, initWithDefaultColor );
}

Image::Image( Uint8* data, const unsigned int& Width, const unsigned int& Height,
			  const unsigned int& Channels ) :
	mPixels( data ),
	mWidth( Width ),
	mHeight( Height ),
	mChannels( Channels ),
	mSize( Width * Height * Channels ),
	mAvoidFree( false ),
	mLoadedFromStbi( false ) {}

Image::Image( std::string Path, const unsigned int& forceChannels,
			  const FormatConfiguration& formatConfiguration ) :
	mPixels( NULL ),
	mWidth( 0 ),
	mHeight( 0 ),
	mChannels( forceChannels ),
	mSize( 0 ),
	mAvoidFree( false ),
	mLoadedFromStbi( false ),
	mFormatConfiguration( formatConfiguration ) {
	int w, h, c;
	Pack* tPack = NULL;
	Uint8* data = stbi_load( Path.c_str(), &w, &h, &c, mChannels );

	if ( NULL != data ) {
		mPixels = data;
		mWidth = (unsigned int)w;
		mHeight = (unsigned int)h;

		if ( STBI_default == mChannels )
			mChannels = (unsigned int)c;

		mSize = mWidth * mHeight * mChannels;

		mLoadedFromStbi = true;
	} else if ( svg_test( Path ) ) {
		svgLoad( nsvgParseFromFile( Path.c_str(), "px", 96.0f ) );
	} else if ( PackManager::instance()->isFallbackToPacksActive() &&
				NULL != ( tPack = PackManager::instance()->exists( Path ) ) ) {
		loadFromPack( tPack, Path );
	} else {
		Log::error( "Failed to load image %s. Reason: %s", Path.c_str(), stbi_failure_reason() );
	}
}

Image::Image( const Uint8* imageData, const unsigned int& imageDataSize,
			  const unsigned int& forceChannels, const FormatConfiguration& formatConfiguration ) :
	mPixels( NULL ),
	mWidth( 0 ),
	mHeight( 0 ),
	mChannels( forceChannels ),
	mSize( 0 ),
	mAvoidFree( false ),
	mLoadedFromStbi( false ),
	mFormatConfiguration( formatConfiguration ) {
	int w, h, c;
	Uint8* data = stbi_load_from_memory( imageData, imageDataSize, &w, &h, &c, mChannels );

	if ( NULL != data ) {
		mPixels = data;
		mWidth = (unsigned int)w;
		mHeight = (unsigned int)h;

		if ( STBI_default == mChannels )
			mChannels = (unsigned int)c;

		mSize = mWidth * mHeight * mChannels;

		mLoadedFromStbi = true;
	} else if ( svg_test_from_memory( imageData, imageDataSize ) ) {
		ScopedBuffer data( imageDataSize + 1 );
		memcpy( data.get(), imageData, imageDataSize );
		data[imageDataSize] = '\0';
		svgLoad( nsvgParse( (char*)data.get(), "px", 96.0f ) );
	} else {
		std::string reason = ".";

		if ( NULL != stbi_failure_reason() ) {
			reason = ", reason: " + std::string( stbi_failure_reason() );
		}

		Log::error( "Failed to load image from memory. Reason: %s", reason.c_str() );
	}
}

Image::Image( Pack* Pack, std::string FilePackPath, const unsigned int& forceChannels,
			  const FormatConfiguration& formatConfiguration ) :
	mPixels( NULL ),
	mWidth( 0 ),
	mHeight( 0 ),
	mChannels( forceChannels ),
	mSize( 0 ),
	mAvoidFree( false ),
	mLoadedFromStbi( false ),
	mFormatConfiguration( formatConfiguration ) {
	loadFromPack( Pack, FilePackPath );
}

Image::Image( IOStream& stream, const unsigned int& forceChannels,
			  const FormatConfiguration& formatConfiguration ) :
	mPixels( NULL ),
	mWidth( 0 ),
	mHeight( 0 ),
	mChannels( forceChannels ),
	mSize( 0 ),
	mAvoidFree( false ),
	mLoadedFromStbi( false ),
	mFormatConfiguration( formatConfiguration ) {
	if ( stream.isOpen() ) {
		stbi_io_callbacks callbacks;
		callbacks.read = &IOCb::read;
		callbacks.skip = &IOCb::skip;
		callbacks.eof = &IOCb::eof;

		int w, h, c;
		Uint8* data = stbi_load_from_callbacks( &callbacks, &stream, &w, &h, &c, mChannels );

		if ( NULL != data ) {
			mPixels = data;
			mWidth = (unsigned int)w;
			mHeight = (unsigned int)h;

			if ( STBI_default == mChannels )
				mChannels = (unsigned int)c;

			mSize = mWidth * mHeight * mChannels;

			mLoadedFromStbi = true;
		} else if ( svg_test_from_stream( stream ) ) {
			ScopedBuffer data( stream.getSize() + 1 );

			stream.seek( 0 );
			stream.read( (char*)data.get(), data.length() - 1 );

			data[data.length() - 1] = '\0';

			svgLoad( nsvgParse( (char*)data.get(), "px", 96.0f ) );
		} else {
			Log::error( "Failed to load image. Reason: %s", stbi_failure_reason() );
		}
	} else {
		Log::error( "Failed to load image from stream." );
	}
}

Image::~Image() {
	if ( !mAvoidFree )
		clearCache();
}

void Image::svgLoad( NSVGimage* image ) {
	if ( image == NULL )
		return;

	NSVGrasterizer* rast = NULL;
	unsigned char* img = NULL;
	int w, h;

	w = (int)image->width * mFormatConfiguration.svgScale();
	h = (int)image->height * mFormatConfiguration.svgScale();

	rast = nsvgCreateRasterizer();

	if ( rast != NULL ) {
		img = (unsigned char*)malloc( w * h * 4 );

		if ( img != NULL ) {
			nsvgRasterize( rast, image, 0, 0, mFormatConfiguration.svgScale(), img, w, h, w * 4 );

			mPixels = img;
			mWidth = w;
			mHeight = h;
			mChannels = 4;
			mLoadedFromStbi = true;
		}
	}

	nsvgDeleteRasterizer( rast );
	nsvgDelete( image );
}

void Image::loadFromPack( Pack* Pack, const std::string& FilePackPath ) {
	if ( NULL != Pack && Pack->isOpen() && -1 != Pack->exists( FilePackPath ) ) {
		ScopedBuffer buffer;

		Pack->extractFileToMemory( FilePackPath, buffer );

		int w, h, c;
		Uint8* data = stbi_load_from_memory( buffer.get(), buffer.length(), &w, &h, &c, mChannels );

		if ( NULL != data ) {
			mPixels = data;
			mWidth = (unsigned int)w;
			mHeight = (unsigned int)h;

			if ( STBI_default == mChannels )
				mChannels = (unsigned int)c;

			mSize = mWidth * mHeight * mChannels;

			mLoadedFromStbi = true;
		} else if ( svg_test_from_memory( buffer.get(), buffer.length() ) ) {
			ScopedBuffer data( buffer.length() + 1 );
			memcpy( data.get(), buffer.get(), buffer.length() );
			data[buffer.length()] = '\0';
			svgLoad( nsvgParse( (char*)data.get(), "px", 96.0f ) );
		} else {
			Log::error( "Failed to load image %s. Reason: %s", FilePackPath.c_str(),
						stbi_failure_reason() );
		}
	} else {
		Log::error( "Failed to load image %s from pack.", FilePackPath.c_str() );
	}
}

void Image::setPixels( const Uint8* data ) {
	if ( data != NULL ) {
		allocate( mWidth * mHeight * mChannels, Color( 0, 0, 0, 0 ), false );

		memcpy( reinterpret_cast<void*>( &mPixels[0] ), reinterpret_cast<const void*>( data ),
				mSize );
	}
}

const Uint8* Image::getPixelsPtr() {
	return reinterpret_cast<const Uint8*>( &mPixels[0] );
}

Color Image::getPixel( const unsigned int& x, const unsigned int& y ) {
	eeASSERT( !( mPixels == NULL || x > mWidth || y > mHeight ) );
	Color dst;
	memcpy( (void*)&dst, &mPixels[( ( x + y * mWidth ) * mChannels )], mChannels );
	return dst;
}

void Image::setPixel( const unsigned int& x, const unsigned int& y, const Color& color ) {
	eeASSERT( !( mPixels == NULL || x > mWidth || y > mHeight ) );
	memcpy( &mPixels[( ( x + y * mWidth ) * mChannels )], &color, mChannels );
}

void Image::create( const Uint32& Width, const Uint32& Height, const Uint32& Channels,
					const Color& DefaultColor, const bool& initWithDefaultColor ) {
	mWidth = Width;
	mHeight = Height;
	mChannels = Channels;

	allocate( mWidth * mHeight * mChannels, DefaultColor, initWithDefaultColor );
}

Uint8* Image::getPixels() const {
	return mPixels;
}

void Image::allocate( const Uint32& size, Color DefaultColor, bool memsetData ) {
	clearCache();

	mPixels = eeNewArray( unsigned char, size );
	mSize = size;

	if ( memsetData ) {
		memset( mPixels, (int)DefaultColor.getValue(), size );
	}
}

unsigned int Image::getMemSize() const {
	return mSize;
}

Sizei Image::getSize() {
	return Sizei( mWidth, mHeight );
}

void Image::clearCache() {
	if ( mLoadedFromStbi ) {
		if ( NULL != mPixels )
			free( mPixels );
	} else {
		eeSAFE_DELETE_ARRAY( mPixels );
	}
}

void Image::setWidth( const unsigned int& width ) {
	mWidth = width;
}

unsigned int Image::getWidth() const {
	return mWidth;
}

void Image::setHeight( const unsigned int& height ) {
	mHeight = height;
}

unsigned int Image::getHeight() const {
	return mHeight;
}

void Image::setChannels( const unsigned int& channels ) {
	mChannels = channels;
}

unsigned int Image::getChannels() const {
	return mChannels;
}

bool Image::saveToFile( const std::string& filepath, const SaveType& Format ) {
	bool Res = false;

	std::string fpath( FileSystem::fileRemoveFileName( filepath ) );

	if ( !FileSystem::isDirectory( fpath ) )
		FileSystem::makeDir( fpath );

	if ( NULL != mPixels && 0 != mWidth && 0 != mHeight && 0 != mChannels ) {
		if ( SaveType::SAVE_TYPE_JPG != Format ) {
			Res = 0 != ( SOIL_save_image( filepath.c_str(), Format, (Int32)mWidth, (Int32)mHeight,
										  mChannels, getPixelsPtr() ) );
		} else {
			jpge::params params;
			params.m_quality = mFormatConfiguration.jpegSaveQuality();
			Res = jpge::compress_image_to_jpeg_file( filepath.c_str(), mWidth, mHeight, mChannels,
													 getPixelsPtr(), params );
		}
	}

	return Res;
}

void Image::replaceColor( const Color& ColorKey, const Color& NewColor ) {
	unsigned int Pos = 0;

	if ( NULL == mPixels )
		return;

	unsigned int size = mWidth * mHeight;

	for ( unsigned int i = 0; i < size; i++ ) {
		Pos = i * mChannels;

		if ( 4 == mChannels ) {
			if ( mPixels[Pos] == ColorKey.r && mPixels[Pos + 1] == ColorKey.g &&
				 mPixels[Pos + 2] == ColorKey.b && mPixels[Pos + 3] == ColorKey.a ) {
				mPixels[Pos] = NewColor.r;
				mPixels[Pos + 1] = NewColor.g;
				mPixels[Pos + 2] = NewColor.b;
				mPixels[Pos + 3] = NewColor.a;
			}
		} else if ( 3 == mChannels ) {
			if ( mPixels[Pos] == ColorKey.r && mPixels[Pos + 1] == ColorKey.g &&
				 mPixels[Pos + 2] == ColorKey.b ) {
				mPixels[Pos] = NewColor.r;
				mPixels[Pos + 1] = NewColor.g;
				mPixels[Pos + 2] = NewColor.b;
			}
		} else if ( 2 == mChannels ) {
			if ( mPixels[Pos] == ColorKey.r && mPixels[Pos + 1] == ColorKey.g ) {
				mPixels[Pos] = NewColor.r;
				mPixels[Pos + 1] = NewColor.g;
			}
		} else if ( 1 == mChannels ) {
			if ( mPixels[Pos] == ColorKey.r ) {
				mPixels[Pos] = NewColor.r;
			}
		}
	}
}

void Image::createMaskFromColor( const Color& ColorKey, Uint8 Alpha ) {
	replaceColor( ColorKey, Color( ColorKey.r, ColorKey.g, ColorKey.b, Alpha ) );
}

void Image::createMaskFromColor( const RGB& ColorKey, Uint8 Alpha ) {
	createMaskFromColor( Color( ColorKey.r, ColorKey.g, ColorKey.b, 255 ), Alpha );
}

void Image::fillWithColor( const Color& Color ) {
	if ( NULL == mPixels )
		return;

	unsigned int z;
	unsigned int size = mWidth * mHeight;

	for ( unsigned int i = 0; i < size; i += mChannels ) {
		for ( z = 0; z < mChannels; z++ ) {
			if ( 0 == z )
				mPixels[i + z] = Color.r;
			else if ( 1 == z )
				mPixels[i + z] = Color.g;
			else if ( 2 == z )
				mPixels[i + z] = Color.b;
			else if ( 3 == z )
				mPixels[i + z] = Color.a;
		}
	}
}

void Image::copyImage( Graphics::Image* image, const Uint32& x, const Uint32& y ) {
	if ( NULL != mPixels && NULL != image->getPixelsPtr() && mWidth >= x + image->getWidth() &&
		 mHeight >= y + image->getHeight() ) {
		unsigned int dWidth = image->getWidth();
		unsigned int dHeight = image->getHeight();

		if ( mChannels != image->getChannels() ) {
			for ( unsigned int ty = 0; ty < dHeight; ty++ ) {
				for ( unsigned int tx = 0; tx < dWidth; tx++ ) {
					setPixel( x + tx, y + ty, image->getPixel( tx, ty ) );
				}
			}
		} else {
			// Copy per row
			for ( unsigned int ty = 0; ty < dHeight; ty++ ) {
				Uint8* pDst = &mPixels[( x + ( ( ty + y ) * mWidth ) ) * mChannels];
				const Uint8* pSrc = &( ( image->getPixelsPtr() )[( ty * dWidth ) * mChannels] );

				memcpy( pDst, pSrc, mChannels * dWidth );
			}
		}
	}
}

void Image::resize( const Uint32& newWidth, const Uint32& newHeight, ResamplerFilter filter ) {
	if ( NULL != mPixels && ( mWidth != newWidth || mHeight != newHeight ) ) {
		unsigned char* resampled =
			resample_image( mPixels, mWidth, mHeight, mChannels, newWidth, newHeight, filter );

		if ( NULL != resampled ) {
			if ( !mAvoidFree )
				clearCache();

			mPixels = resampled;
			mWidth = newWidth;
			mHeight = newHeight;
			mSize = mWidth * mHeight * mChannels;
			mLoadedFromStbi = false;
			mAvoidFree = false;
		}
	}
}

void Image::scale( const Float& scale, ResamplerFilter filter ) {
	if ( 1.f == scale )
		return;

	Int32 newWidth = (Int32)( (Float)mWidth * scale );
	Int32 newHeight = (Int32)( (Float)mHeight * scale );

	resize( newWidth, newHeight, filter );
}

Graphics::Image* Image::thumbnail( const Uint32& maxWidth, const Uint32& maxHeight,
								   ResamplerFilter filter ) {
	if ( NULL != mPixels ) {
		Float iScaleX = ( (Float)maxWidth / (Float)mWidth );
		Float iScaleY = ( (Float)maxHeight / (Float)mHeight );
		Float iScale = ( iScaleY < iScaleX ) ? iScaleY : iScaleX;
		Int32 new_width = (Int32)( (Float)mWidth * iScale );
		Int32 new_height = (Int32)( (Float)mHeight * iScale );

		unsigned char* resampled =
			resample_image( mPixels, mWidth, mHeight, mChannels, new_width, new_height, filter );

		if ( NULL != resampled ) {
			return eeNew( Image, ( (Uint8*)resampled, new_width, new_height, mChannels ) );
		}
	}

	return NULL;
}

Graphics::Image* Image::crop( Rect rect ) {
	if ( rect.Left >= 0 && rect.Right <= (Int32)mWidth && rect.Top >= 0 &&
		 rect.Bottom <= (Int32)mHeight ) {
		Image* img = Image::New( rect.getSize().getWidth(), rect.getSize().getHeight(), mChannels );

		// Copy per row
		for ( unsigned int ty = 0; ty < img->mHeight; ty++ ) {
			Uint8* pDst = &img->mPixels[( ty * img->mWidth ) * mChannels];
			const Uint8* pSrc =
				&mPixels[( rect.Left + ( ( ty + rect.Top ) * mWidth ) ) * mChannels];

			memcpy( pDst, pSrc, mChannels * img->mWidth );
		}

		return img;
	}

	return NULL;
}

void Image::flip() {
	if ( NULL != mPixels ) {
		Image tImg( mHeight, mWidth, mChannels );

		for ( unsigned int y = 0; y < mHeight; y++ )
			for ( unsigned int x = 0; x < mWidth; x++ )
				tImg.setPixel( y, x, getPixel( x, mHeight - 1 - y ) );

		clearCache();

		mPixels = tImg.getPixels();
		mWidth = tImg.getWidth();
		mHeight = tImg.getHeight();
		mLoadedFromStbi = false;

		tImg.avoidFreeImage( true );
	}
}

void Image::avoidFreeImage( const bool& AvoidFree ) {
	mAvoidFree = AvoidFree;
}

void Image::blit( Graphics::Image* image, const Uint32& x, const Uint32& y ) {
	if ( NULL != image && NULL != image->getPixelsPtr() && x < mWidth && y < mHeight ) {
		unsigned int dh = eemin( mHeight, y + image->getHeight() );
		unsigned int dw = eemin( mWidth, x + image->getWidth() );

		for ( unsigned int ty = y; ty < dh; ty++ ) {
			for ( unsigned int tx = x; tx < dw; tx++ ) {
				Color ts( image->getPixel( tx - x, ty - y ) );
				Color td( getPixel( tx, ty ) );

				setPixel( tx, ty, Color::blend( ts, td ) );
			}
		}
	}
}

Graphics::Image* Image::copy() {
	return eeNew( Graphics::Image, ( this ) );
}

Graphics::Image& Image::operator=( const Image& right ) {
	mWidth = right.mWidth;
	mHeight = right.mHeight;
	mChannels = right.mChannels;
	mSize = right.mSize;
	mAvoidFree = right.mAvoidFree;
	mLoadedFromStbi = right.mLoadedFromStbi;

	clearCache();

	if ( NULL != right.mPixels ) {
		setPixels( right.mPixels );
	}

	return *this;
}

void Image::setImageFormatConfiguration(
	const Image::FormatConfiguration& imageFormatConfiguration ) {
	mFormatConfiguration = imageFormatConfiguration;
}

const Image::FormatConfiguration& Image::getImageFormatConfiguration() const {
	return mFormatConfiguration;
}

}} // namespace EE::Graphics
