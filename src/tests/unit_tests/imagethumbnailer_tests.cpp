#include "exif_test_utils.hpp"
#include "utest.hpp"
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <eepp/graphics/imagethumbnailer.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/system/sys.hpp>
#include <mutex>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::System;

namespace {

struct Fixtures {
	std::string dir, image;
	Fixtures() {
		static std::atomic<Uint64> next{ 0 };
		dir = Sys::getTempPath();
		FileSystem::dirAddSlashAtEnd( dir );
		dir += "eepp-thumbnails-test-" + String::toString( Sys::getProcessID() ) + "-" +
			   String::toString( ++next ) + "/";
		FileSystem::makeDir( dir, true );
		image = dir + "image.png";
		Image source( 37, 19, 4, Color::Transparent, false );
		source.fillWithColor( Color( 255, 0, 0, 128 ) );
		source.saveToFile( image, Image::SaveType::PNG );
	}

	~Fixtures() { FileSystem::dirRemoveAll( dir ); }

	ImageThumbnailer::Config config() const {
		ImageThumbnailer::Config result;
		result.cachePath = dir + "cache/";
		result.useTurboJPEG = false;
		return result;
	}

	ImageThumbnailer::Source source( std::atomic<int>& opens, std::string revision = "1" ) const {
		ImageThumbnailer::Source result;
		result.key = image;
		result.revision = std::move( revision );
		result.open = [path = image, &opens]( CancellationToken ) -> std::unique_ptr<IOStream> {
			++opens;
			return std::make_unique<IOStreamFile>( path );
		};
		return result;
	}
};

ImageThumbnailer::Result finish( Task<ImageThumbnailer::Result> task ) {
	Clock clock;
	while ( task.isPending() && clock.getElapsedTime() < Seconds( 5 ) )
		Sys::sleep( Milliseconds( 1 ) );
	if ( !task.isReady() )
		return { {}, ImageThumbnailer::Error::Busy };
	return std::move( task ).takeValue();
}

struct Gate {
	std::mutex mutex;
	std::condition_variable changed;
	bool entered{ false }, released{ false };

	void wait() {
		std::unique_lock<std::mutex> lock( mutex );
		entered = true;
		changed.notify_all();
		changed.wait_for( lock, std::chrono::seconds( 5 ), [&] { return released; } );
	}

	void enteredWait() {
		std::unique_lock<std::mutex> lock( mutex );
		changed.wait_for( lock, std::chrono::seconds( 5 ), [&] { return entered; } );
	}

	void release() {
		{
			std::lock_guard<std::mutex> lock( mutex );
			released = true;
		}
		changed.notify_all();
	}
};

} // namespace

UTEST( ImageThumbnailer, FractionalAreaIncludesEdgesAndPreservesAlphaWeightedColors ) {
	const Uint8 grayscale[] = { 0, 0, 255, 0, 0 };
	Image source( grayscale, 5, 1, 1 );
	auto result = ImageThumbnailer::reduce( source, { 2, 1 } );
	EXPECT_EQ( result.getWidth(), 2u );
	EXPECT_EQ( int( result.getPixelsPtr()[0] ), 51 );
	EXPECT_EQ( int( result.getPixelsPtr()[1] ), 51 );
	const Uint8 transparent[] = { 255, 0, 0, 255, 0, 0, 255, 0 };
	Image rgba( transparent, 2, 1, 4 );
	auto reduced = ImageThumbnailer::reduce( rgba, { 1, 1 } );
	EXPECT_EQ( int( reduced.getPixelsPtr()[0] ), 255 );
	EXPECT_EQ( int( reduced.getPixelsPtr()[1] ), 0 );
	EXPECT_EQ( int( reduced.getPixelsPtr()[2] ), 0 );
	EXPECT_EQ( int( reduced.getPixelsPtr()[3] ), 128 );
	const Uint8 edge[] = { 0, 0, 0, 0, 0, 0, 0, 0, 255 };
	Image last( edge, 3, 3, 1 );
	EXPECT_EQ( int( ImageThumbnailer::reduce( last, { 1, 1 } ).getPixelsPtr()[0] ), 28 );
	Image narrow( 1, 1001, 3, Color::Red );
	auto thin = ImageThumbnailer::reduce( narrow, { 128, 128 } );
	EXPECT_EQ( thin.getWidth(), 1u );
	EXPECT_EQ( thin.getHeight(), 128u );
	EXPECT_EQ( ImageThumbnailer::reduce( rgba, { 128, 128 } ).getWidth(), 2u );
	CancellationSource cancelled;
	cancelled.cancel();
	EXPECT_FALSE( ImageThumbnailer::reduce( rgba, { 1, 1 }, cancelled.token() ).getPixelsPtr() );
}

UTEST( ImageThumbnailer, DiskCacheSurvivesSessionsAndCorruptEntriesRegenerate ) {
	Fixtures files;
	std::atomic<int> opens{ 0 };
	{
		ImageThumbnailer service( files.config() );
		auto first = finish( service.request( files.source( opens ), { 16, 16 } ) );
		ASSERT_TRUE( first );
		EXPECT_FALSE( first.cached );
		EXPECT_EQ( first.image->getWidth(), 16u );
		EXPECT_EQ( first.image->getHeight(), 8u );
		auto second = finish( service.request( files.source( opens ), { 16, 16 } ) );
		ASSERT_TRUE( second );
		EXPECT_TRUE( second.cached );
		EXPECT_EQ( opens.load(), 1 );
		EXPECT_TRUE( first.image == second.image );
	}
	{
		ImageThumbnailer service( files.config() );
		auto cached = finish( service.request( files.source( opens ), { 16, 16 } ) );
		ASSERT_TRUE( cached );
		EXPECT_TRUE( cached.cached );
		EXPECT_EQ( opens.load(), 1 );
	}
	const auto entries = FileSystem::filesInfoGetInPath( files.config().cachePath );
	ASSERT_EQ( entries.size(), size_t{ 1 } );
	ASSERT_TRUE(
		FileSystem::fileWrite( entries[0].getFilepath(), std::string_view( "broken PNG" ) ) );
	{
		ImageThumbnailer service( files.config() );
		auto regenerated = finish( service.request( files.source( opens ), { 16, 16 } ) );
		ASSERT_TRUE( regenerated );
		EXPECT_FALSE( regenerated.cached );
		EXPECT_EQ( opens.load(), 2 );
		EXPECT_TRUE( std::move( service.clearCache() ).get() );
	}
	EXPECT_TRUE( FileSystem::filesInfoGetInPath( files.config().cachePath ).empty() );
}

UTEST( ImageThumbnailer, DuplicateRequestsShareWorkButCancellationIsPerConsumer ) {
	Fixtures files;
	std::atomic<int> opens{ 0 };
	ImageThumbnailer service( files.config() );
	Gate gate;
	CancellationSource firstCancel;
	auto first = files.source( opens );
	first.open = [path = files.image, &gate,
				  &opens]( CancellationToken ) -> std::unique_ptr<IOStream> {
		++opens;
		gate.wait();
		return std::make_unique<IOStreamFile>( path );
	};
	auto one = service.request( std::move( first ), { 16, 16 }, firstCancel.token() );
	gate.enteredWait();
	auto two = service.request( files.source( opens ), { 16, 16 } );
	firstCancel.cancel();
	gate.release();
	EXPECT_TRUE( finish( std::move( one ) ).error == ImageThumbnailer::Error::Cancelled );
	EXPECT_TRUE( finish( std::move( two ) ) );
	EXPECT_EQ( opens.load(), 1 );
}

UTEST( ImageThumbnailer, FullQueueCancellationAndShutdownAreBounded ) {
	Fixtures files;
	auto config = files.config();
	config.workers = 1;
	config.maxPending = 1;
	ImageThumbnailer service( config );
	std::atomic<int> opens{ 0 };
	Gate gate;
	CancellationSource cancellation;
	auto source = files.source( opens );
	source.open = [path = files.image, &gate]( CancellationToken ) -> std::unique_ptr<IOStream> {
		gate.wait();
		return std::make_unique<IOStreamFile>( path );
	};
	auto first = service.request( std::move( source ), { 16, 16 }, cancellation.token() );
	gate.enteredWait();
	EXPECT_TRUE( finish( service.request( files.source( opens, "second" ), { 16, 16 } ) ).error ==
				 ImageThumbnailer::Error::Busy );
	cancellation.cancel();
	gate.release();
	EXPECT_TRUE( finish( std::move( first ) ).error == ImageThumbnailer::Error::Cancelled );
	service.shutdown();
	EXPECT_TRUE( finish( service.request( files.source( opens ), { 16, 16 } ) ).error ==
				 ImageThumbnailer::Error::Cancelled );
}

UTEST( ImageThumbnailer, ChangedRevisionsAndMemoryLimitsDoNotPersistInvalidResults ) {
	Fixtures files;
	std::atomic<int> opens{ 0 };
	{
		ImageThumbnailer service( files.config() );
		auto source = files.source( opens );
		source.currentRevision = [] { return std::string( "changed" ); };
		EXPECT_TRUE( finish( service.request( std::move( source ), { 16, 16 } ) ).error ==
					 ImageThumbnailer::Error::Changed );
	}
	EXPECT_TRUE( FileSystem::filesInfoGetInPath( files.config().cachePath ).empty() );
	auto config = files.config();
	config.decodeBytes = 1;
	ImageThumbnailer bounded( config );
	EXPECT_TRUE( finish( bounded.request( files.source( opens ), { 16, 16 } ) ).error ==
				 ImageThumbnailer::Error::TooLarge );
}

UTEST( ImageThumbnailer, RuntimeJPEGHeadersAreOptionalAndDiskCachingCanBeDisabled ) {
	Fixtures files;
	const auto jpeg = files.dir + "photo.jpg";
	Image pixels( 200, 100, 3, Color::Red );
	ASSERT_TRUE( pixels.saveToFile( jpeg, Image::SaveType::JPG ) );
	std::atomic<int> opens{ 0 };
	for ( bool runtime : { false, true } ) {
		auto config = files.config();
		config.useTurboJPEG = runtime;
		if ( runtime )
			config.turboJPEGPath = files.dir + "missing-runtime-library";
		config.diskCache = false;
		ImageThumbnailer service( config );
		auto source = files.source( opens );
		source.key = jpeg;
		source.open = [jpeg]( CancellationToken ) -> std::unique_ptr<IOStream> {
			return std::make_unique<IOStreamFile>( jpeg );
		};
		auto image = finish( service.request( std::move( source ), { 32, 32 } ) );
		ASSERT_TRUE( image );
		EXPECT_EQ( image.image->getWidth(), 32u );
		EXPECT_EQ( image.image->getHeight(), 16u );
	}
	EXPECT_TRUE( FileSystem::filesInfoGetInPath( files.config().cachePath ).empty() );
}

UTEST( ImageThumbnailer, EmbeddedThumbnailOrientationIsIndependentAndSmallThumbnailsFallBack ) {
	Fixtures files;
	const auto originalPath = files.dir + "original.jpg";
	Image original( 80, 40, 3, Color::Transparent, false );
	original.fillWithColor( Color::Red );
	ASSERT_TRUE( original.saveToFile( originalPath, Image::SaveType::JPG ) );
	std::string encoded;
	ASSERT_TRUE( FileSystem::fileGet( originalPath, encoded ) );
	std::atomic<int> opens{ 0 };
	for ( bool adequate : { true, false } ) {
		const auto thumbPath = files.dir + "embedded.jpg";
		Image thumbnail( adequate ? 32 : 4, adequate ? 64 : 8, 3, Color::Transparent, false );
		thumbnail.fillWithColor( Color( 0, 255, 0 ) );
		ASSERT_TRUE( thumbnail.saveToFile( thumbPath, Image::SaveType::JPG ) );
		std::string small;
		ASSERT_TRUE( FileSystem::fileGet( thumbPath, small ) );
		auto metadata = exiftest::tiff( 6 );
		metadata.resize( 80, '\0' );
		metadata[22] = 26;
		metadata[26] = 4;
		const auto entry = [&]( size_t pos, Uint16 tag, Uint16 type, Uint32 value ) {
			metadata[pos] = tag & 255;
			metadata[pos + 1] = tag >> 8;
			metadata[pos + 2] = type;
			metadata[pos + 4] = 1;
			for ( int i = 0; i < 4; ++i )
				metadata[pos + 8 + i] = char( value >> ( 8 * i ) );
		};
		entry( 28, 0x103, 3, 6 );
		entry( 40, 0x112, 3, 1 );
		entry( 52, 0x201, 4, 80 );
		entry( 64, 0x202, 4, small.size() );
		metadata += small;
		ASSERT_TRUE( FileSystem::fileWrite( files.image, exiftest::jpeg( encoded, metadata ) ) );
		ImageThumbnailer service( files.config() );
		auto result = finish( service.request( files.source( opens, adequate ? "large" : "small" ),
											   adequate ? Sizei( 16, 16 ) : Sizei( 64, 64 ) ) );
		ASSERT_TRUE( result );
		EXPECT_EQ( result.image->getWidth(), adequate ? 8u : 32u );
		EXPECT_EQ( result.image->getHeight(), adequate ? 16u : 64u );
		const auto* pixel = result.image->getPixelsPtr();
		EXPECT_GT( int( pixel[adequate ? 1 : 0] ), 200 );
		EXPECT_LT( int( pixel[adequate ? 0 : 1] ), 20 );
	}
}

UTEST( ImageThumbnailer, ReplacingCancelledQueuedWorkResolvesEveryConsumer ) {
	Fixtures files;
	auto config = files.config();
	config.workers = 1;
	ImageThumbnailer service( config );
	std::atomic<int> opens{ 0 };
	Gate gate;
	auto blocking = files.source( opens, "blocking" );
	blocking.open = [path = files.image, &gate]( CancellationToken ) -> std::unique_ptr<IOStream> {
		gate.wait();
		return std::make_unique<IOStreamFile>( path );
	};
	auto first = service.request( std::move( blocking ), { 16, 16 } );
	gate.enteredWait();
	CancellationSource cancellation;
	auto cancelled = service.request( files.source( opens ), { 16, 16 }, cancellation.token() );
	cancellation.cancel();
	auto replacement = service.request( files.source( opens ), { 16, 16 } );
	gate.release();
	EXPECT_TRUE( finish( std::move( first ) ) );
	EXPECT_TRUE( finish( std::move( cancelled ) ).error == ImageThumbnailer::Error::Cancelled );
	EXPECT_TRUE( finish( std::move( replacement ) ) );
	EXPECT_EQ( opens.load(), 1 );
}

UTEST( ImageThumbnailer, DeliversAndPublishesMemoryCacheBeforeDiskPersistence ) {
	Fixtures files;
	std::atomic<int> opens{ 0 };
	ImageThumbnailer service( files.config() );
	Gate gate;
	auto source = files.source( opens );
	source.currentRevision = [&gate] {
		gate.wait();
		return std::string( "1" );
	};
	auto pending = service.request( std::move( source ), { 16, 16 } );
	gate.enteredWait();
	auto delivered = std::move( pending ).then( [&]( ImageThumbnailer::Result&& result ) {
		const bool beforePersistence =
			FileSystem::filesInfoGetInPath( files.config().cachePath ).empty();
		auto memory = finish( service.request( files.source( opens ), { 16, 16 } ) );
		return result && beforePersistence && memory.cached && memory.image == result.image;
	} );
	gate.release();
	const auto observed = std::move( delivered ).get();
	EXPECT_TRUE( observed && *observed );
	service.shutdown();
	EXPECT_EQ( FileSystem::filesInfoGetInPath( files.config().cachePath ).size(), size_t{ 1 } );
	EXPECT_EQ( opens.load(), 1 );
}

UTEST( Image, WebPSaveHonorsLosslessAndPreservesTransparentPixels ) {
	Fixtures files;
	Image image( 17, 11, 4, Color::Transparent, false );
	for ( size_t i = 0; i < image.getMemSize(); ++i )
		image.getPixels()[i] = Uint8( i * 37 );
	image.getPixels()[3] = 0;
	for ( bool lossless : { false, true } ) {
		Image::FormatConfiguration options;
		options.webpSaveLossless( lossless );
		options.webpSaveQuality( 0 );
		options.webpCompressionMethod( 0 );
		image.setImageFormatConfiguration( options );
		const auto path = files.dir + "settings.webp";
		ASSERT_TRUE( image.saveToFile( path, Image::SaveType::WEBP ) );
		Image decoded( path, 4 );
		ASSERT_TRUE( decoded.getPixelsPtr() );
		ASSERT_EQ( decoded.getMemSize(), image.getMemSize() );
		const bool exact =
			std::memcmp( decoded.getPixelsPtr(), image.getPixelsPtr(), image.getMemSize() ) == 0;
		// Lossless preserves every byte, including RGB under transparent pixels. Lossy at
		// quality zero must change this deliberately varied fixture.
		EXPECT_TRUE( exact == lossless );
	}
}

UTEST( ImageThumbnailer, EveryCacheFormatPreservesColorsAlphaAndGrayscaleBetweenSessions ) {
	using Format = ImageThumbnailer::CacheFormat;
	EXPECT_TRUE( ImageThumbnailer::Config{}.cacheFormat == Format::QOI );
	for ( auto format : { Format::QOI, Format::PNG, Format::WebP, Format::TGA } ) {
		EXPECT_TRUE( ImageThumbnailer::cacheFormatFromString(
						 ImageThumbnailer::cacheFormatToString( format ) ) == format );
		for ( Uint32 channels = 1; channels <= 4; ++channels ) {
			Fixtures files;
			Image original( 13, 9, channels, Color::Transparent, false );
			for ( size_t i = 0; i < original.getMemSize(); ++i )
				original.getPixels()[i] = Uint8( i * 37 );
			ASSERT_TRUE( original.saveToFile( files.image, Image::SaveType::PNG ) );
			auto config = files.config();
			config.cacheFormat = format;
			std::atomic<int> opens{ 0 };
			{
				ImageThumbnailer service( config );
				ASSERT_TRUE( finish( service.request( files.source( opens ), { 16, 16 } ) ) );
			}
			const auto entries = FileSystem::filesInfoGetInPath( config.cachePath );
			ASSERT_EQ( entries.size(), size_t{ 1 } );
			EXPECT_STDSTREQ( entries[0].getExtension(),
							 ImageThumbnailer::cacheFormatToString( format ) );
			{
				ImageThumbnailer service( config );
				auto loaded = finish( service.request( files.source( opens ), { 16, 16 } ) );
				ASSERT_TRUE( loaded );
				EXPECT_TRUE( loaded.cached );
				EXPECT_EQ( opens.load(), 1 );
				ASSERT_EQ( loaded.image->getWidth(), original.getWidth() );
				ASSERT_EQ( loaded.image->getHeight(), original.getHeight() );
				for ( size_t i = 0; i < original.getWidth() * original.getHeight(); ++i ) {
					const auto* source = original.getPixelsPtr() + i * channels;
					const auto* cached =
						loaded.image->getPixelsPtr() + i * loaded.image->getChannels();
					if ( loaded.image->getChannels() == channels ) {
						EXPECT_EQ( std::memcmp( source, cached, channels ), 0 );
					} else {
						ASSERT_TRUE( channels <= 2 );
						EXPECT_EQ( cached[0], source[0] );
						EXPECT_EQ( cached[1], source[0] );
						EXPECT_EQ( cached[2], source[0] );
						if ( channels == 2 ) {
							ASSERT_EQ( loaded.image->getChannels(), 4u );
							EXPECT_EQ( cached[3], source[1] );
						}
					}
				}
			}
			// All formats regenerate invalid cached data without exposing it to a loader unchecked.
			ASSERT_TRUE( FileSystem::fileWrite( entries[0].getFilepath(),
												std::string_view( "corrupt thumbnail" ) ) );
			ImageThumbnailer service( config );
			auto regenerated = finish( service.request( files.source( opens ), { 16, 16 } ) );
			ASSERT_TRUE( regenerated );
			EXPECT_FALSE( regenerated.cached );
			EXPECT_EQ( opens.load(), 2 );
		}
	}
	EXPECT_TRUE( ImageThumbnailer::cacheFormatFromString( "WeBp" ) == Format::WebP );
	EXPECT_TRUE( ImageThumbnailer::cacheFormatFromString( "invalid" ) == Format::QOI );
}

UTEST( ImageThumbnailer, ChangingFormatKeepsInFlightEncodingAndClearsAllOwnedFormats ) {
	using Format = ImageThumbnailer::CacheFormat;
	Fixtures files;
	auto config = files.config();
	config.cacheFormat = Format::PNG;
	ImageThumbnailer service( config );
	std::atomic<int> opens{ 0 };
	Gate gate;
	auto source = files.source( opens );
	source.currentRevision = [&gate] {
		gate.wait();
		return std::string( "1" );
	};
	auto first = service.request( std::move( source ), { 16, 16 } );
	gate.enteredWait();
	service.setCacheFormat( Format::QOI );
	gate.release();
	ASSERT_TRUE( finish( std::move( first ) ) );
	ASSERT_TRUE( finish( service.request( files.source( opens, "2" ), { 16, 16 } ) ) );
	service.shutdown();
	const auto entries = FileSystem::filesInfoGetInPath( config.cachePath );
	ASSERT_EQ( entries.size(), size_t{ 2 } );
	EXPECT_TRUE( std::any_of( entries.begin(), entries.end(),
							  []( const auto& info ) { return info.getExtension() == "png"; } ) );
	EXPECT_TRUE( std::any_of( entries.begin(), entries.end(),
							  []( const auto& info ) { return info.getExtension() == "qoi"; } ) );
	const auto unrelated = config.cachePath + "keep.png";
	ASSERT_TRUE( FileSystem::fileWrite( unrelated, std::string_view( "unrelated file" ) ) );
	ImageThumbnailer clear( config );
	EXPECT_TRUE( std::move( clear.clearCache() ).get() );
	EXPECT_TRUE( FileSystem::fileExists( unrelated ) );
	EXPECT_EQ( FileSystem::filesInfoGetInPath( config.cachePath ).size(), size_t{ 1 } );
}

UTEST( ImageThumbnailer, PersistentKeyEncodingRemainsCompatible ) {
	Fixtures files;
	std::atomic<int> opens{ 0 };
	ImageThumbnailer service( files.config() );
	auto source = files.source( opens, "r:1234" );
	source.key = "file:///images/a:b.jpg";
	ASSERT_TRUE( finish( service.request( std::move( source ), { 16, 8 } ) ) );
	service.shutdown();
	// Stable digest of the original length-prefixed fields: changing it invalidates disk entries.
	EXPECT_TRUE( FileSystem::fileExists( files.config().cachePath +
										 "d27d6db22e947e890c2045c80efeca6ff2d0b94d.qoi" ) );
}

UTEST( Image, DecodeSizeHintUsesWebPScalingAndSVGRasterSize ) {
	Fixtures files;
	for ( Uint32 channels : { 3u, 4u } ) {
		Image original( 65, 33, channels, Color::Transparent, false );
		original.fillWithColor( Color( 20, 60, 120, 128 ) );
		Image::FormatConfiguration options;
		options.webpSaveLossless( true );
		options.webpSaveQuality( 0 );
		options.webpCompressionMethod( 0 );
		original.setImageFormatConfiguration( options );
		const auto path = files.dir + "scaled.webp";
		ASSERT_TRUE( original.saveToFile( path, Image::SaveType::WEBP ) );
		IOStreamFile stream( path );
		Image scaled( stream, Sizei( 16, 16 ) );
		ASSERT_TRUE( scaled.getPixelsPtr() );
		EXPECT_EQ( scaled.getWidth(), 16u );
		EXPECT_EQ( scaled.getHeight(), 8u );
		EXPECT_EQ( scaled.getChannels(), channels );
		EXPECT_EQ( scaled.getMemSize(), 16u * 8u * channels );
		for ( size_t i = 0; i < 16u * 8u; ++i ) {
			const auto* pixel = scaled.getPixelsPtr() + i * channels;
			EXPECT_LE( std::abs( int( pixel[0] ) - 20 ), 1 );
			EXPECT_LE( std::abs( int( pixel[1] ) - 60 ), 1 );
			EXPECT_LE( std::abs( int( pixel[2] ) - 120 ), 1 );
			if ( channels == 4 )
				EXPECT_LE( std::abs( int( pixel[3] ) - 128 ), 1 );
		}
		stream.seek( 0 );
		Image largerHint( stream, Sizei( 256, 256 ) );
		EXPECT_EQ( largerHint.getWidth(), 65u );
		EXPECT_EQ( largerHint.getHeight(), 33u );
		stream.seek( 0 );
		Image unchanged( stream );
		EXPECT_EQ( unchanged.getWidth(), 65u );
		EXPECT_EQ( unchanged.getHeight(), 33u );
	}
	const std::string svg = "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"80\" "
							"height=\"40\"><rect width=\"80\" height=\"40\" fill=\"red\"/></svg>";
	IOStreamMemory vector( svg.data(), svg.size() );
	Image scaled( vector, Sizei( 16, 16 ) );
	ASSERT_TRUE( scaled.getPixelsPtr() );
	EXPECT_EQ( scaled.getWidth(), 16u );
	EXPECT_EQ( scaled.getHeight(), 8u );
	EXPECT_EQ( scaled.getMemSize(), 16u * 8u * 4u );
}

UTEST( Image, StreamInfoSkipsLargeJPEGMetadataAndRestoresPosition ) {
	Fixtures files;
	const auto path = files.dir + "info.jpg";
	Image original( 65, 33, 3, Color::White );
	ASSERT_TRUE( original.saveToFile( path, Image::SaveType::JPG ) );
	std::string jpeg;
	ASSERT_TRUE( FileSystem::fileGet( path, jpeg ) );
	std::string bytes( "\xff\xd8", 2 );
	for ( int i = 0; i < 3; ++i ) {
		bytes.append( "\xff\xe2\xff\xff", 4 );
		bytes.append( 65533, '\0' );
	}
	bytes.append( jpeg.data() + 2, jpeg.size() - 2 );
	IOStreamMemory stream( bytes.data(), bytes.size() );
	stream.seek( 7 );
	int width = 0, height = 0, channels = 0;
	ASSERT_TRUE( Image::getInfoFromStream( stream, &width, &height, &channels ) );
	EXPECT_EQ( width, 65 );
	EXPECT_EQ( height, 33 );
	EXPECT_EQ( channels, 3 );
	EXPECT_EQ( stream.tell(), 7 );
	ImageThumbnailer thumbnails( files.config() );
	std::atomic<int> opens{ 0 };
	ASSERT_TRUE( FileSystem::fileWrite( files.image, bytes ) );
	auto preview = finish( thumbnails.request( files.source( opens ), { 16, 16 } ) );
	ASSERT_TRUE( preview );
	EXPECT_EQ( preview.image->getWidth(), 16u );
	EXPECT_EQ( preview.image->getHeight(), 8u );
}

UTEST( ImageThumbnailer, ConcurrentServicesShareRuntimeBindingsWithIndependentDecoders ) {
	Fixtures files;
	Image original( 200, 100, 3, Color::White );
	ASSERT_TRUE( original.saveToFile( files.image, Image::SaveType::JPG ) );
	std::atomic<int> opens{ 0 };
	std::vector<std::unique_ptr<ImageThumbnailer>> services;
	std::vector<Task<ImageThumbnailer::Result>> tasks;
	services.reserve( 4 );
	tasks.reserve( 4 );
	for ( int i = 0; i < 4; ++i ) {
		auto config = files.config();
		config.useTurboJPEG = true;
		config.diskCache = false;
		config.workers = 1;
		services.push_back( std::make_unique<ImageThumbnailer>( std::move( config ) ) );
		tasks.push_back( services.back()->request( files.source( opens ), { 32, 32 } ) );
	}
	for ( auto& task : tasks ) {
		auto image = finish( std::move( task ) );
		ASSERT_TRUE( image );
		EXPECT_EQ( image.image->getWidth(), 32u );
		EXPECT_EQ( image.image->getHeight(), 16u );
	}
	EXPECT_EQ( opens.load(), 4 );
}
