#include "../tests/unit_tests/utest.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <eepp/graphics/imagethumbnailer.hpp>
#include <eepp/graphics/renderer/renderer.hpp>
#include <eepp/graphics/texturefactory.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/uiapplication.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::Window;

namespace {

/** File-cache candidates, measured with the same already reduced pixels. Standard formats use
 * Image's encoders/decoders, matching the service. Writes include close but no fsync or rename;
 * reads use the OS page cache. GPU timings finish the upload, with mipmaps disabled. */
struct ThumbnailCacheBenchmark {
	enum class Format { PNG, QOI, Raw, DDS, WebPFast, WebP, TGA, TGARaw };

	static const char* name( Format format ) {
		constexpr const char* names[] = { "png",	   "qoi",  "raw", "dds",
										  "webp-fast", "webp", "tga", "tga-raw" };
		return names[static_cast<int>( format )];
	}

	static bool write( Format format, const std::string& path, const Image& image ) {
		if ( format == Format::WebPFast || format == Format::WebP ) {
			// Borrow pixels so changing encoder settings does not copy the thumbnail.
			Image view( image.getPixels(), image.getWidth(), image.getHeight(),
						image.getChannels() );
			view.avoidFreeImage( true );
			Image::FormatConfiguration options;
			options.webpSaveLossless( true );
			options.webpSaveQuality( format == Format::WebPFast ? 0 : 75 );
			options.webpCompressionMethod( format == Format::WebPFast ? 0 : 4 );
			view.setImageFormatConfiguration( options );
			return view.saveToFile( path, Image::SaveType::WEBP );
		}
		if ( format == Format::TGARaw ) {
			IOStreamFile stream( path, "wb" );
			std::array<Uint8, 18> header{};
			header[2] = 2; // Uncompressed true color, top-left origin, optional alpha.
			header[12] = image.getWidth() & 255;
			header[13] = image.getWidth() >> 8;
			header[14] = image.getHeight() & 255;
			header[15] = image.getHeight() >> 8;
			header[16] = image.getChannels() * 8;
			header[17] = 0x20 | ( image.getChannels() == 4 ? 8 : 0 );
			if ( !stream.isOpen() || stream.write( reinterpret_cast<const char*>( header.data() ),
												   header.size() ) != ios_size( header.size() ) )
				return false;
			std::array<Uint8, 4096> row;
			for ( Uint32 y = 0; y < image.getHeight(); ++y ) {
				const auto* source =
					image.getPixelsPtr() + size_t( y ) * image.getWidth() * image.getChannels();
				for ( Uint32 x = 0; x < image.getWidth(); ++x ) {
					auto* pixel = row.data() + x * image.getChannels();
					for ( Uint32 c = 0; c < image.getChannels(); ++c )
						pixel[c] = source[x * image.getChannels() + ( c < 3 ? 2 - c : c )];
				}
				const size_t length = image.getWidth() * image.getChannels();
				if ( stream.write( reinterpret_cast<const char*>( row.data() ), length ) !=
					 ios_size( length ) )
					return false;
			}
			return true;
		}
		if ( format == Format::Raw ) {
			IOStreamFile stream( path, "wb" );
			// A small fixed header represents dimensions/channels/version in a private raw cache.
			const std::array<Uint32, 4> header{ 1, image.getWidth(), image.getHeight(),
												image.getChannels() };
			return stream.isOpen() &&
				   stream.write( reinterpret_cast<const char*>( header.data() ),
								 sizeof( header ) ) == sizeof( header ) &&
				   stream.write( reinterpret_cast<const char*>( image.getPixelsPtr() ),
								 image.getMemSize() ) == image.getMemSize();
		}
		Image writer( image.getPixels(), image.getWidth(), image.getHeight(), image.getChannels() );
		writer.avoidFreeImage( true );
		const auto type = format == Format::QOI	  ? Image::SaveType::QOI
						  : format == Format::PNG ? Image::SaveType::PNG
						  : format == Format::TGA ? Image::SaveType::TGA
												  : Image::SaveType::DDS;
		return writer.saveToFile( path, type );
	}

	static Image read( Format format, const std::string& path ) {
		if ( format != Format::Raw )
			return Image( path );
		IOStreamFile stream( path );
		std::array<Uint32, 4> header{};
		if ( !stream.isOpen() ||
			 stream.read( reinterpret_cast<char*>( header.data() ), sizeof( header ) ) !=
				 sizeof( header ) ||
			 header[0] != 1 || header[1] > 1024 || header[2] > 1024 ||
			 ( header[3] != 3 && header[3] != 4 ) )
			return {};
		Image image( header[1], header[2], header[3], Color::Transparent, false );
		if ( stream.read( reinterpret_cast<char*>( image.getPixels() ), image.getMemSize() ) !=
			 image.getMemSize() )
			return {};
		return image;
	}

	static std::vector<Uint8> readEncoded( const std::string& path ) {
		IOStreamFile stream( path );
		std::vector<Uint8> bytes( stream.getSize() );
		if ( stream.read( reinterpret_cast<char*>( bytes.data() ), bytes.size() ) !=
			 ios_size( bytes.size() ) )
			bytes.clear();
		return bytes;
	}

	static bool compare( const Image& original, const Image& dds, const std::string& path ) {
		Image comparison( original.getWidth() * 2, original.getHeight(), 3, Color::Transparent,
						  false );
		for ( Uint32 y = 0; y < original.getHeight(); ++y ) {
			for ( Uint32 x = 0; x < original.getWidth(); ++x ) {
				for ( int side = 0; side < 2; ++side ) {
					const Image& image = side ? dds : original;
					const auto* pixel =
						image.getPixelsPtr() +
						( size_t( y ) * image.getWidth() + x ) * image.getChannels();
					const int alpha = image.getChannels() == 4 ? pixel[3] : 255;
					const int background = ( x / 8 + y / 8 ) % 2 ? 180 : 230;
					auto* out =
						comparison.getPixels() +
						( size_t( y ) * comparison.getWidth() + x + side * image.getWidth() ) * 3;
					for ( int c = 0; c < 3; ++c )
						out[c] = ( pixel[c] * alpha + background * ( 255 - alpha ) + 127 ) / 255;
				}
			}
		}
		return comparison.saveToFile( path, Image::SaveType::PNG );
	}

	static TexturePtr upload( const Image& image ) {
		return TextureFactory::instance()->loadFromPixels(
			image.getPixelsPtr(), image.getWidth(), image.getHeight(), image.getChannels(), false,
			Texture::ClampMode::ClampToEdge, false, false );
	}

	static TexturePtr upload( const std::vector<Uint8>& encoded ) {
		return TextureFactory::instance()->loadFromMemory(
			encoded.data(), encoded.size(), false, Texture::ClampMode::ClampToEdge, false, false );
	}

	template <typename F> static double microseconds( F&& operation, int batch = 8 ) {
		operation();
		std::array<double, 5> samples;
		for ( auto& sample : samples ) {
			Clock clock;
			for ( int i = 0; i < batch; ++i )
				operation();
			sample = clock.getElapsedTime().asMicroseconds() / double( batch );
		}
		std::sort( samples.begin(), samples.end() );
		return samples[samples.size() / 2];
	}
};

} // namespace

UTEST( ImageThumbnailerBenchmark, CacheFormats ) {
	using Bench = ThumbnailCacheBenchmark;
	UIApplication::Settings settings( Sys::getProcessPath() + "../", 1 );
	UIApplication app( WindowSettings( 320, 240, "Thumbnail cache formats",
									   WindowStyle::Default | WindowStyle::Hidden,
									   WindowBackend::Default, 32, {}, 1, false, true ),
					   settings );
	ASSERT_NE( app.getWindow(), nullptr );
	printf( "  GPU: %s / %s / %s\n", GLi->getVendor().c_str(), GLi->getRenderer().c_str(),
			GLi->getVersion().c_str() );
	std::string directory = Sys::getEnv( "EEPP_THUMBNAIL_BENCH_OUTPUT" );
	const bool temporary = directory.empty();
	if ( temporary )
		directory = Sys::getTempPath() + "eepp-thumbnail-cache-bench-" +
					String::toString( Sys::getProcessID() );
	FileSystem::dirAddSlashAtEnd( directory );
	ASSERT_TRUE( FileSystem::makeDir( directory, true ) );
	ScopedOp cleanup( nullptr, [&] {
		if ( temporary )
			FileSystem::dirRemoveAll( directory );
	} );

	const std::string repository = Sys::getProcessPath() + "../../";
	const std::pair<const char*, const char*> fixtures[] = {
		{ "portrait", "src/thirdparty/SOIL2/bin/lenna1.jpg" },
		{ "mars", "src/thirdparty/SOIL2/bin/img_mars.jpg" },
		{ "ui", "bin/unit_tests/assets/html/eepp-uihtmltable-complex-layout.webp" },
		{ "alpha-icon", "bin/assets/icon/eefm.png" } };
	printf( "THUMB_CACHE,fixture,box,format,width,height,bytes,write_us,warm_read_us,upload_us,"
			"total_us,compressed,RGB_MAE,alpha_MAE\n" );
	for ( const auto& fixture : fixtures ) {
		Image source( repository + fixture.second );
		ASSERT_TRUE( source.getPixelsPtr() );
		ASSERT_TRUE( source.getChannels() == 3 || source.getChannels() == 4 );
		for ( int box : { 128, 256, 512 } ) {
			auto reduced = ImageThumbnailer::reduce( source, { box, box } );
			ASSERT_TRUE( reduced.getPixelsPtr() );
			for ( auto format : { Bench::Format::PNG, Bench::Format::QOI, Bench::Format::Raw,
								  Bench::Format::DDS, Bench::Format::WebPFast, Bench::Format::WebP,
								  Bench::Format::TGA, Bench::Format::TGARaw } ) {
				const auto path = directory + fixture.first + "-" + String::toString( box ) + "." +
								  Bench::name( format );
				bool valid = true;
				const auto write = Bench::microseconds(
					[&] { valid = Bench::write( format, path, reduced ) && valid; } );
				ASSERT_TRUE( valid );
				const auto bytes = FileSystem::fileSize( path );
				auto decoded = Bench::read( format, path );
				ASSERT_TRUE( decoded.getPixelsPtr() );
				ASSERT_EQ( decoded.getWidth(), reduced.getWidth() );
				ASSERT_EQ( decoded.getHeight(), reduced.getHeight() );
				ASSERT_GE( decoded.getChannels(), reduced.getChannels() );
				double rgbError = 0, alphaError = 0;
				const size_t pixels = size_t( reduced.getWidth() ) * reduced.getHeight();
				for ( size_t i = 0; i < pixels; ++i ) {
					const auto* expected = reduced.getPixelsPtr() + i * reduced.getChannels();
					const auto* actual = decoded.getPixelsPtr() + i * decoded.getChannels();
					for ( int c = 0; c < 3; ++c )
						rgbError += std::abs( int( expected[c] ) - actual[c] );
					if ( reduced.getChannels() == 4 )
						alphaError += std::abs( int( expected[3] ) - actual[3] );
				}
				if ( format != Bench::Format::DDS ) {
					ASSERT_EQ( rgbError, 0. );
					ASSERT_EQ( alphaError, 0. );
				}
				std::vector<Uint8> encoded;
				const bool direct = format == Bench::Format::DDS;
				if ( direct )
					encoded = Bench::readEncoded( path );
				const auto read = Bench::microseconds( [&] {
					if ( direct ) {
						auto data = Bench::readEncoded( path );
						valid = !data.empty() && valid;
					} else {
						auto image = Bench::read( format, path );
						valid = image.getPixelsPtr() && valid;
					}
				} );
				TexturePtr texture;
				const auto upload = Bench::microseconds( [&] {
					texture = direct ? Bench::upload( encoded ) : Bench::upload( decoded );
					GLi->waitForIdle();
					valid = texture && valid;
				} );
				ASSERT_TRUE( texture );
				const bool compressed = texture->isCompressed();
				const auto total = Bench::microseconds( [&] {
					if ( direct ) {
						auto data = Bench::readEncoded( path );
						texture = Bench::upload( data );
					} else {
						auto image = Bench::read( format, path );
						texture = Bench::upload( image );
					}
					GLi->waitForIdle();
					valid = texture && valid;
				} );
				ASSERT_TRUE( valid );
				if ( direct && !temporary )
					ASSERT_TRUE( Bench::compare( reduced, decoded, path + ".comparison.png" ) );
				printf( "THUMB_CACHE,%s,%d,%s,%u,%u,%llu,%.3f,%.3f,%.3f,%.3f,%d,%.3f,%.3f\n",
						fixture.first, box, Bench::name( format ), reduced.getWidth(),
						reduced.getHeight(), static_cast<unsigned long long>( bytes ), write, read,
						upload, total, compressed, rgbError / ( pixels * 3 ), alphaError / pixels );
				fflush( stdout );
			}
		}
	}
	printf( "  Cache benchmark artifacts: %s\n", directory.c_str() );
}
