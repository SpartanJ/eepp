#include "../tests/unit_tests/utest.hpp"
#include <algorithm>
#include <array>
#include <eepp/graphics/imagethumbnailer.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/scopedop.hpp>
#include <eepp/system/sys.hpp>

using namespace EE;
using namespace EE::Graphics;
using namespace EE::System;

namespace {

volatile Uint64 thumbnailChecksum = 0;

template <typename F> double medianMilliseconds( F&& operation ) {
	operation();
	std::array<double, 5> samples;
	for ( auto& sample : samples ) {
		Clock clock;
		operation();
		sample = clock.getElapsedTime().asMicroseconds() / 1000.;
	}
	std::sort( samples.begin(), samples.end() );
	return samples[samples.size() / 2];
}

void consume( const Image& image ) {
	Uint64 sum = 0;
	for ( size_t i = 0; i < image.getMemSize(); ++i )
		sum += image.getPixelsPtr()[i];
	thumbnailChecksum = sum;
}

} // namespace

UTEST( ImageThumbnailerBenchmark, LargeJPEGGenerationAndReduction ) {
	std::string directory = Sys::getTempPath();
	FileSystem::dirAddSlashAtEnd( directory );
	directory += "eepp-thumbnail-bench-" + String::toString( Sys::getProcessID() ) + "/";
	ASSERT_TRUE( FileSystem::makeDir( directory, true ) );
	ScopedOp cleanup( nullptr, [&] { FileSystem::dirRemoveAll( directory ); } );
	const auto path = directory + "24mp.jpg";
	Image source( 6000, 4000, 3, Color::Transparent, false );
	for ( Uint32 y = 0; y < source.getHeight(); ++y ) {
		for ( Uint32 x = 0; x < source.getWidth(); ++x ) {
			auto* pixel = source.getPixels() + ( size_t( y ) * source.getWidth() + x ) * 3;
			pixel[0] = x / 13 + y / 17;
			pixel[1] = x / 7 + y / 11;
			pixel[2] = x / 19 + y / 5;
		}
	}
	ASSERT_TRUE( source.saveToFile( path, Image::SaveType::JPG ) );
	const double general = medianMilliseconds( [&] {
		Image copy( source );
		copy.resize( 256, 170 );
		consume( copy );
	} );
	const double area = medianMilliseconds( [&] {
		auto reduced = ImageThumbnailer::reduce( source, { 256, 256 } );
		consume( reduced );
	} );
	printf( "  24MP RGB -> 256px: general Lanczos %.2f ms, thumbnail area %.2f ms\n", general,
			area );
	for ( bool optionalTurbo : { false, true } ) {
		ImageThumbnailer::Config config;
		config.diskCache = false;
		config.memoryCacheBytes = 0;
		config.useTurboJPEG = optionalTurbo;
		ImageThumbnailer thumbnails( std::move( config ) );
		bool valid = true;
		const double generation = medianMilliseconds( [&] {
			ImageThumbnailer::Source input;
			input.key = path;
			input.open = [path]( CancellationToken ) -> std::unique_ptr<IOStream> {
				return std::make_unique<IOStreamFile>( path );
			};
			auto result = std::move( thumbnails.request( std::move( input ), { 256, 256 } ) ).get();
			valid = valid && result && bool( *result );
			if ( result && *result )
				consume( *result->image );
		} );
		ASSERT_TRUE( valid );
		printf( "  24MP JPEG -> 256px, no cache, %s: %.2f ms\n",
				optionalTurbo ? "optional runtime TurboJPEG (fallback if absent)"
							  : "bundled decoder",
				generation );
	}
}
