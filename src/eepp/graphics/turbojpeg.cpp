#include "turbojpeg.hpp"
#include <algorithm>
#include <cmath>
#include <eepp/core/containers.hpp>
#include <eepp/system/sys.hpp>
#include <limits>
#include <memory>
#include <mutex>
#include <vector>

namespace EE::Graphics::Private {

using namespace EE::System;

const TurboJpeg& TurboJpeg::instance( const std::string& libraryPath ) {
	if ( libraryPath.empty() ) {
		static const TurboJpeg library( std::string{} );
		return library;
	}
	// Explicit paths are independent of auto-detection, including cached failed lookups.
	static std::mutex mutex;
	static UnorderedMap<std::string, std::unique_ptr<TurboJpeg>> libraries;
	std::lock_guard<std::mutex> lock( mutex );
	auto found = libraries.find( libraryPath );
	if ( found != libraries.end() )
		return *found->second;
	return *libraries
				.emplace( libraryPath, std::unique_ptr<TurboJpeg>( new TurboJpeg( libraryPath ) ) )
				.first->second;
}

TurboJpeg::TurboJpeg( const std::string& libraryPath ) {
	if ( !libraryPath.empty() ) {
		load( libraryPath );
		return;
	}
#if EE_PLATFORM == EE_PLATFORM_WIN
	const char* names[] = { "turbojpeg.dll", "libturbojpeg.dll" };
#elif EE_PLATFORM == EE_PLATFORM_MACOS
	const char* names[] = { "libturbojpeg.0.dylib", "libturbojpeg.dylib",
							"/opt/homebrew/opt/jpeg-turbo/lib/libturbojpeg.0.dylib",
							"/usr/local/opt/jpeg-turbo/lib/libturbojpeg.0.dylib" };
#elif defined( EE_PLATFORM_POSIX )
	const char* names[] = { "libturbojpeg.so.0", "libturbojpeg.so" };
#else
	return;
#endif
#if defined( EE_PLATFORM_POSIX ) || EE_PLATFORM == EE_PLATFORM_WIN
	for ( const auto* name : names ) {
		if ( load( name ) )
			break;
	}
#endif
}

bool TurboJpeg::load( const std::string& path ) {
	auto* library = Sys::loadObject( path, false );
	if ( !library )
		return false;
	mInit = reinterpret_cast<Init>( Sys::loadFunction( library, "tjInitDecompress", false ) );
	mHeader =
		reinterpret_cast<Header>( Sys::loadFunction( library, "tjDecompressHeader3", false ) );
	mScales =
		reinterpret_cast<Scales>( Sys::loadFunction( library, "tjGetScalingFactors", false ) );
	mDecode = reinterpret_cast<Decode>( Sys::loadFunction( library, "tjDecompress2", false ) );
	mDestroy = reinterpret_cast<Destroy>( Sys::loadFunction( library, "tjDestroy", false ) );
	if ( mInit && mHeader && mScales && mDecode && mDestroy ) {
		mLibrary = library;
		return true;
	}
	Sys::unloadObject( library );
	return false;
}

TurboJpeg::~TurboJpeg() {
	if ( mLibrary )
		Sys::unloadObject( mLibrary );
}

Image TurboJpeg::decode( IOStream& stream, Sizei target, Uint64 maxPixels ) const {
	if ( !available() || stream.getSize() <= 0 ||
		 Uint64( stream.getSize() ) > std::numeric_limits<unsigned long>::max() ||
		 stream.seek( 0 ) != 0 )
		return {};
	// TurboJPEG takes contiguous compressed input; the caller enforces the source byte budget.
	std::vector<Uint8> bytes( stream.getSize() );
	size_t received = 0;
	while ( received < bytes.size() ) {
		const auto count = stream.read( reinterpret_cast<char*>( bytes.data() + received ),
										bytes.size() - received );
		if ( count <= 0 || Uint64( count ) > bytes.size() - received )
			return {};
		received += count;
	}
	std::unique_ptr<void, Destroy> decoder( mInit(), mDestroy );
	int width = 0, height = 0, subsampling = 0, colorSpace = 0;
	if ( !decoder ||
		 mHeader( decoder.get(), bytes.data(), bytes.size(), &width, &height, &subsampling,
				  &colorSpace ) != 0 ||
		 width <= 0 || height <= 0 || Uint64( width ) * height > maxPixels )
		return {};
	const double scale =
		std::min( { 1., double( target.x ) / width, double( target.y ) / height } );
	const int desiredWidth = std::max( 1, int( std::ceil( width * scale ) ) );
	const int desiredHeight = std::max( 1, int( std::ceil( height * scale ) ) );
	int count = 0, outputWidth = width, outputHeight = height;
	const auto* factors = mScales( &count );
	if ( !factors || count <= 0 || count > 64 )
		return {};
	for ( int i = 0; i < count; ++i ) {
		if ( factors[i].numerator <= 0 || factors[i].denominator <= 0 )
			continue;
		const auto w = ( Int64( width ) * factors[i].numerator + factors[i].denominator - 1 ) /
					   factors[i].denominator;
		const auto h = ( Int64( height ) * factors[i].numerator + factors[i].denominator - 1 ) /
					   factors[i].denominator;
		if ( w >= desiredWidth && h >= desiredHeight && w <= outputWidth && h <= outputHeight ) {
			outputWidth = w;
			outputHeight = h;
		}
	}
	Image image( outputWidth, outputHeight, 3, Color::Transparent, false );
	// RGB's stable TurboJPEG C ABI value is 0. No ABI-dependent public decoder struct is used.
	if ( mDecode( decoder.get(), bytes.data(), bytes.size(), image.getPixels(), outputWidth, 0,
				  outputHeight, 0, 0 ) != 0 )
		return {};
	return image;
}

} // namespace EE::Graphics::Private
