#include <SOIL2/src/SOIL2/SOIL2.h>
#include <SOIL2/src/SOIL2/image_helper.h>
#include <SOIL2/src/SOIL2/stb_image.h>

#include <eepp/graphics/image.hpp>
#include <eepp/graphics/pixeldensity.hpp>
#include <eepp/graphics/stbi_iocb.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/packmanager.hpp>

#include <algorithm>
#include <memory>

#include <imageresampler/resampler.h>
#include <jpeg-compressor/jpge.h>
#include <optional>

#define NANOSVG_IMPLEMENTATION
#include <nanosvg/nanosvg.h>
#define NANOSVGRAST_IMPLEMENTATION
#include <nanosvg/nanosvgrast.h>

#include <webp/decode.h>
#include <webp/encode.h>

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

struct ImageInfo {
	int width{ 0 };
	int height{ 0 };
	int channels{ 4 };
};

static std::optional<ImageInfo> webp_test_from_memory( const Uint8* imageData,
													   const unsigned int& imageDataSize ) {
	if ( !imageData || imageDataSize < 12 ) // Minimum WebP header is ~12 bytes (RIFF+size+WEBP)
		return {};
	WebPDecoderConfig config;
	if ( !WebPInitDecoderConfig( &config ) )
		return {};
	VP8StatusCode status =
		WebPGetFeatures( imageData, static_cast<size_t>( imageDataSize ), &config.input );
	if ( status == VP8_STATUS_OK )
		return ImageInfo{ config.input.width, config.input.height, config.input.has_alpha ? 4 : 3 };
	return {};
}

static std::optional<ImageInfo> webp_test_from_stream( IOStream& stream ) {
	char buf[128]{ 0 };
	stream.read( buf, 128 );
	return webp_test_from_memory( reinterpret_cast<const Uint8*>( &buf[0] ), 128 );
}

static std::optional<ImageInfo> webp_test( const std::string& path ) {
	IOStreamFile stream( path );
	return webp_test_from_stream( stream );
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

std::string Image::saveTypeToExtension( Image::SaveType Format ) {
	switch ( Format ) {
		case Image::SaveType::TGA:
			return "tga";
		case Image::SaveType::BMP:
			return "bmp";
		case Image::SaveType::PNG:
			return "png";
		case Image::SaveType::DDS:
			return "dds";
		case Image::SaveType::JPG:
			return "jpg";
		case Image::SaveType::QOI:
			return "qoi";
		case Image::SaveType::WEBP:
			return "webp";
		case Image::SaveType::Unknown:
		default:
			break;
	}

	return "";
}

Image::SaveType Image::extensionToSaveType( const std::string& Extension ) {
	SaveType saveType = SaveType::Unknown;

	if ( Extension == "tga" )
		saveType = SaveType::TGA;
	else if ( Extension == "bmp" )
		saveType = SaveType::BMP;
	else if ( Extension == "png" )
		saveType = SaveType::PNG;
	else if ( Extension == "dds" )
		saveType = SaveType::DDS;
	else if ( Extension == "jpg" || Extension == "jpeg" )
		saveType = SaveType::JPG;
	else if ( Extension == "qoi" )
		saveType = SaveType::QOI;
	else if ( Extension == "webp" )
		saveType = SaveType::WEBP;

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
	std::optional<ImageInfo> info;

	if ( !res && svg_test( path ) ) {
		NSVGimage* image = nsvgParseFromFile( path.c_str(), "px", 96.0f, 0xFFFFFFFF );

		if ( NULL != image ) {
			*width = image->width * imageFormatConfiguration.svgScale();
			*height = image->height * imageFormatConfiguration.svgScale();
			*channels = 4;

			nsvgDelete( image );

			res = true;
		}
	} else if ( !res && ( info = webp_test( path ) ) ) {
		*width = info->width;
		*height = info->height;
		*channels = info->channels;
		res = true;
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

				NSVGimage* image = nsvgParse( (char*)data.get(), "px", 96.0f, 0xFFFFFFFF );

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
	std::optional<ImageInfo> info;

	if ( !res && svg_test_from_memory( data, dataSize ) ) {
		ScopedBuffer sdata( dataSize + 1 );
		memcpy( sdata.get(), data, dataSize );
		sdata[dataSize] = '\0';
		NSVGimage* image = nsvgParse( (char*)sdata.get(), "px", 96.0f, 0xFFFFFFFF );

		if ( NULL != image ) {
			*width = image->width * imageFormatConfiguration.svgScale();
			*height = image->height * imageFormatConfiguration.svgScale();
			*channels = 4;

			nsvgDelete( image );

			res = true;
		}
	} else if ( !res && ( info = webp_test_from_memory( data, dataSize ) ) ) {
		*width = info->width;
		*height = info->height;
		*channels = info->channels;
		res = true;
	}

	return res;
}

bool Image::isImage( const std::string& path ) {
	return STBI_unknown != stbi_test( path.c_str() ) || svg_test( path ) || webp_test( path );
}

bool Image::isImage( const unsigned char* data, const size_t& dataSize ) {
	return STBI_unknown != stbi_test_from_memory( data, dataSize ) ||
		   svg_test_from_memory( data, dataSize ) || webp_test_from_memory( data, dataSize );
}

Image::Format Image::getFormat( const std::string& path ) {
	auto format = static_cast<Image::Format>( stbi_test( path.c_str() ) );
	if ( format == Image::Format::Unknown )
		format = svg_test( path ) ? Image::Format::SVG : Image::Format::Unknown;
	if ( format == Image::Format::Unknown )
		format = webp_test( path ) ? Image::Format::WEBP : Image::Format::Unknown;
	return format;
}

Image::Format Image::getFormat( const unsigned char* data, const size_t& dataSize ) {
	auto format = static_cast<Image::Format>( stbi_test_from_memory( data, dataSize ) );
	if ( format == Image::Format::Unknown ) {
		format =
			svg_test_from_memory( data, dataSize ) ? Image::Format::SVG : Image::Format::Unknown;
	}
	if ( format == Image::Format::Unknown ) {
		format =
			webp_test_from_memory( data, dataSize ) ? Image::Format::WEBP : Image::Format::Unknown;
	}
	return format;
}

Image::Format Image::getFormat( IOStream& stream ) {
	stbi_io_callbacks callbacks;
	callbacks.read = &IOCb::read;
	callbacks.skip = &IOCb::skip;
	callbacks.eof = &IOCb::eof;
	auto format = static_cast<Image::Format>( stbi_test_from_callbacks( &callbacks, &stream ) );
	if ( format == Image::Format::Unknown )
		format = svg_test_from_stream( stream ) ? Image::Format::SVG : Image::Format::Unknown;
	if ( format == Image::Format::Unknown )
		format = webp_test_from_stream( stream ) ? Image::Format::WEBP : Image::Format::Unknown;
	return format;
}

bool Image::isImageExtension( const std::string& path ) {
	const std::string ext( FileSystem::fileExtension( path ) );
	return ( ext == "png" || ext == "tga" || ext == "bmp" || ext == "jpg" || ext == "gif" ||
			 ext == "jpeg" || ext == "dds" || ext == "psd" || ext == "hdr" || ext == "pic" ||
			 ext == "pvr" || ext == "pkm" || ext == "svg" || ext == "qoi" || ext == "webp" ||
			 ext == "jpe" );
}

std::vector<std::string> Image::getImageExtensionsSupported() {
	return std::vector<std::string>{ "png", "tga", "bmp", "jpg", "gif", "jpeg", "dds",	"psd",
									 "hdr", "pic", "pvr", "pkm", "svg", "qoi",	"webp", "jpe" };
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
		svgLoad( nsvgParseFromFile( Path.c_str(), "px", 96.0f, 0xFFFFFFFF ) );
	} else if ( webp_test( Path ) ) {
		ScopedBuffer buf;
		FileSystem::fileGet( Path, buf );
		webpLoad( buf.get(), buf.size() );
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
		svgLoad( nsvgParse( (char*)data.get(), "px", 96.0f, 0xFFFFFFFF ) );
	} else if ( webp_test_from_memory( imageData, imageDataSize ) ) {
		webpLoad( imageData, imageDataSize );
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

			svgLoad( nsvgParse( (char*)data.get(), "px", 96.0f, 0xFFFFFFFF ) );
		} else if ( webp_test_from_stream( stream ) ) {
			ScopedBuffer data( stream.getSize() );
			stream.seek( 0 );
			stream.read( (char*)data.get(), data.length() );
			webpLoad( data.get(), data.size() );
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

void Image::webpLoad( const Uint8* imageData, size_t imageDataSize ) {
	WebPBitstreamFeatures features;
	if ( WebPGetFeatures( imageData, imageDataSize, &features ) != VP8_STATUS_OK )
		return;

	int channels = features.has_alpha ? 4 : 3;
	int datasize = features.width * features.height * channels;
	int stride = channels * features.width;
	Uint8* dstImage = (Uint8*)malloc( datasize );

	bool errdec = false;
	if ( features.has_alpha ) {
		errdec =
			WebPDecodeRGBAInto( imageData, imageDataSize, dstImage, datasize, stride ) == nullptr;
	} else {
		errdec =
			WebPDecodeRGBInto( imageData, imageDataSize, dstImage, datasize, stride ) == nullptr;
	}

	if ( errdec || nullptr == dstImage ) {
		eeSAFE_FREE( dstImage );
		return;
	}

	mPixels = dstImage;
	mWidth = features.width;
	mHeight = features.height;
	mChannels = channels;
	mLoadedFromStbi = true;
}

std::vector<Uint8> webpPacker( const Image& img, Uint32 quality, Uint32 compressionMethod,
							   bool lossless ) {
	quality = eeclamp( quality, 0u, 100u );
	compressionMethod = eeclamp( quality, 0u, 6u );

	if ( img.getChannels() < 3 )
		return {};

	Sizei s( img.getWidth(), img.getHeight() );
	const Uint8* r = img.getPixelsPtr();

	WebPConfig config;
	WebPPicture pic;
	if ( !WebPConfigInit( &config ) || !WebPPictureInit( &pic ) ) {
		return {};
	}

	WebPMemoryWriter wrt;
	if ( lossless ) {
		config.lossless = 1;
		config.exact = 1;
	}
	config.method = compressionMethod;
	config.quality = quality;
	config.use_sharp_yuv = 1;
	pic.use_argb = 1;
	pic.width = s.getWidth();
	pic.height = s.getHeight();
	pic.writer = WebPMemoryWrite;
	pic.custom_ptr = &wrt;
	WebPMemoryWriterInit( &wrt );

	bool successImport = false;
	if ( img.getChannels() == 3 ) {
		successImport = WebPPictureImportRGB( &pic, r, 3 * s.getWidth() );
	} else {
		successImport = WebPPictureImportRGBA( &pic, r, 4 * s.getHeight() );
	}

	bool successEncode = false;
	if ( successImport ) {
		successEncode = WebPEncode( &config, &pic );
	}
	WebPPictureFree( &pic );

	if ( !successEncode ) {
		WebPMemoryWriterClear( &wrt );
		return {};
	}

	// copy from wrt
	std::vector<Uint8> dst;
	dst.resize( wrt.size );
	Uint8* w = dst.data();
	memcpy( w, wrt.mem, wrt.size );
	WebPMemoryWriterClear( &wrt );
	return dst;
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
			svgLoad( nsvgParse( (char*)data.get(), "px", 96.0f, 0xFFFFFFFF ) );
		} else if ( webp_test_from_memory( buffer.get(), buffer.length() ) ) {
			webpLoad( buffer.get(), buffer.length() );
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

const Uint8* Image::getPixelsPtr() const {
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
		if ( SaveType::WEBP == Format ) {
			auto data = webpPacker( *this, mFormatConfiguration.webpSaveQuality(),
									mFormatConfiguration.webpSaveLossless(),
									mFormatConfiguration.webpCompressionMethod() );
			if ( !data.empty() )
				FileSystem::fileWrite( filepath, data );
		} else if ( SaveType::JPG != Format ) {
			Res = 0 != ( SOIL_save_image( filepath.c_str(), (int)Format, (Int32)mWidth,
										  (Int32)mHeight, mChannels, getPixelsPtr() ) );
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

	switch ( mChannels ) {
		case 4: {
			for ( unsigned int i = 0; i < size; i++ ) {
				Pos = i * mChannels;
				if ( mPixels[Pos] == ColorKey.r && mPixels[Pos + 1] == ColorKey.g &&
					 mPixels[Pos + 2] == ColorKey.b && mPixels[Pos + 3] == ColorKey.a ) {
					mPixels[Pos] = NewColor.r;
					mPixels[Pos + 1] = NewColor.g;
					mPixels[Pos + 2] = NewColor.b;
					mPixels[Pos + 3] = NewColor.a;
				}
			}
			break;
		}
		case 3: {
			for ( unsigned int i = 0; i < size; i++ ) {
				Pos = i * mChannels;
				if ( mPixels[Pos] == ColorKey.r && mPixels[Pos + 1] == ColorKey.g &&
					 mPixels[Pos + 2] == ColorKey.b ) {
					mPixels[Pos] = NewColor.r;
					mPixels[Pos + 1] = NewColor.g;
					mPixels[Pos + 2] = NewColor.b;
				}
			}
			break;
		}
		case 2: {
			for ( unsigned int i = 0; i < size; i++ ) {
				Pos = i * mChannels;
				if ( mPixels[Pos] == ColorKey.r && mPixels[Pos + 1] == ColorKey.g ) {
					mPixels[Pos] = NewColor.r;
					mPixels[Pos + 1] = NewColor.g;
				}
			}
			break;
		}
		case 1: {
			for ( unsigned int i = 0; i < size; i++ ) {
				Pos = i * mChannels;
				if ( mPixels[Pos] == ColorKey.r ) {
					mPixels[Pos] = NewColor.r;
				}
			}
			break;
		}
		default:
			break;
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

	Uint64 size = mWidth * mHeight;

	switch ( mChannels ) {
		case 4: {
			for ( Uint64 i = 0; i < size; i += mChannels ) {
				mPixels[i + 0] = Color.r;
				mPixels[i + 1] = Color.g;
				mPixels[i + 2] = Color.b;
				mPixels[i + 3] = Color.a;
			}
			break;
		}
		case 3: {
			for ( Uint64 i = 0; i < size; i += mChannels ) {
				mPixels[i + 0] = Color.r;
				mPixels[i + 1] = Color.g;
				mPixels[i + 2] = Color.b;
			}
			break;
		}
		case 2: {
			for ( Uint64 i = 0; i < size; i += mChannels ) {
				mPixels[i + 0] = Color.r;
				mPixels[i + 1] = Color.g;
			}
			break;
		}
		case 1: {
			for ( Uint64 i = 0; i < size; i += mChannels ) {
				mPixels[i] = Color.r;
			}
			break;
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

std::pair<std::vector<Image>, int> Image::loadGif( IOStream& stream ) {
	stbi_io_callbacks callbacks;
	callbacks.read = &IOCb::read;
	callbacks.skip = &IOCb::skip;
	callbacks.eof = &IOCb::eof;
	stream.seek( 0 );
	auto type = stbi_test_from_callbacks( &callbacks, &stream );
	if ( type != STBI_gif )
		return {};
	stream.seek( 0 );
	std::vector<Image> gif;
	ScopedBuffer buf( stream.getSize() );
	stream.read( (char*)buf.get(), buf.size() );
	int width, height, frames, comp;
	int* delays = NULL;
	unsigned char* data = stbi_load_gif_from_memory( buf.get(), buf.size(), &delays, &width,
													 &height, &frames, &comp, 0 );

	if ( data == nullptr )
		return {};

	gif.reserve( frames );

	unsigned char* start = data;
	size_t frame_size = width * height * sizeof( unsigned char ) * comp;
	for ( int i = 0; i < frames; ++i ) {
		gif.emplace_back( (const Uint8*)start, width, height, comp );
		start += frame_size;
	}

	auto delay = delays[0];
	if ( delay == 0 ) {
		for ( int i = 0; i < frames; i++ ) {
			if ( delays[i] != 0 ) {
				delay = delays[i];
				break;
			}
		}
	}
	free( data );
	free( delays );
	return { std::move( gif ), delay ? delay : 100 };
}

}} // namespace EE::Graphics
