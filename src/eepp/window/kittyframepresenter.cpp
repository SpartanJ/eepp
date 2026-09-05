#include <eepp/core/string.hpp>
#include <eepp/system/base64.hpp>
#include <eepp/system/compression.hpp>
#include <eepp/system/iostreammemory.hpp>
#include <eepp/window/terminal/kittyframepresenter.hpp>
#include <eepp/window/terminal/terminalruntime.hpp>
#include <eepp/window/window.hpp>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace EE::System;

namespace EE { namespace Window {

namespace {

constexpr Int32 TileSize = 32;
constexpr size_t CompressionThreshold = 4096;
constexpr Uint32 StreamImageId = 0x45455050;
constexpr Uint32 StreamPlacementId = 1;
enum class DamageResult : Uint8 { None, Rectangle, Full };

bool environmentFlag( const char* name, bool defaultValue ) {
	const char* value = std::getenv( name );
	if ( !value || !value[0] )
		return defaultValue;
	if ( 0 == std::strcmp( value, "0" ) || String::iequals( value, "false" ) ||
		 String::iequals( value, "off" ) || String::iequals( value, "no" ) )
		return false;
	if ( 0 == std::strcmp( value, "1" ) || String::iequals( value, "true" ) ||
		 String::iequals( value, "on" ) || String::iequals( value, "yes" ) )
		return true;
	return defaultValue;
}

int environmentCompressionLevel() {
	const char* value = std::getenv( "EEPP_TERMINAL_ZLIB_LEVEL" );
	if ( !value || !value[0] )
		return 1;
	char* end = nullptr;
	const long level = std::strtol( value, &end, 10 );
	return end != value && *end == '\0' && level >= 0 && level <= 9 ? static_cast<int>( level ) : 1;
}

template <typename Rectangle>
DamageResult findDamage( const std::vector<Uint8>& current, const std::vector<Uint8>& previous,
						 const Math::Sizei& size, std::vector<Rectangle>& rectangles ) {
	rectangles.clear();
	const Int32 tilesX = ( size.x + TileSize - 1 ) / TileSize;
	const Int32 tilesY = ( size.y + TileSize - 1 ) / TileSize;
	const size_t rowBytes = static_cast<size_t>( size.x ) * 3;
	const size_t totalPixels = static_cast<size_t>( size.x ) * size.y;
	size_t changedPixels = 0;
	Int32 left = size.x;
	Int32 top = size.y;
	Int32 right = 0;
	Int32 bottom = 0;

	for ( Int32 tileY = 0; tileY < tilesY; ++tileY ) {
		const Int32 y = tileY * TileSize;
		const Int32 height = eemin( TileSize, size.y - y );
		for ( Int32 tileX = 0; tileX < tilesX; ++tileX ) {
			bool changed = false;
			const Int32 x = tileX * TileSize;
			const Int32 width = eemin( TileSize, size.x - x );
			for ( Int32 row = 0; row < height && !changed; ++row ) {
				const Int32 sourceRow = size.y - 1 - ( y + row );
				const size_t offset =
					static_cast<size_t>( sourceRow ) * rowBytes + static_cast<size_t>( x ) * 3;
				changed = 0 != std::memcmp( current.data() + offset, previous.data() + offset,
											static_cast<size_t>( width ) * 3 );
			}
			if ( changed ) {
				changedPixels += static_cast<size_t>( width ) * height;
				left = eemin( left, x );
				top = eemin( top, y );
				right = eemax( right, x + width );
				bottom = eemax( bottom, y + height );
				if ( changedPixels * 5 >= totalPixels * 3 )
					return DamageResult::Full;
			}
		}
	}

	if ( left < right && top < bottom ) {
		rectangles.push_back( { left, top, right - left, bottom - top } );
		return DamageResult::Rectangle;
	}
	return DamageResult::None;
}

bool compressPixels( const std::vector<Uint8>& input, std::vector<Uint8>& output, int level ) {
	if ( level == 0 || input.size() < CompressionThreshold )
		return false;
	const int maxSize = Compression::getMaxCompressedBufferSize( input.size() );
	if ( maxSize <= 0 )
		return false;
	output.resize( static_cast<size_t>( maxSize ) );
	IOStreamMemory source( reinterpret_cast<const char*>( input.data() ), input.size() );
	IOStreamMemory destination( reinterpret_cast<char*>( output.data() ), output.size() );
	Compression::Config config;
	config.zlib.level = level;
	if ( Compression::compress( destination, source, Compression::MODE_DEFLATE, config ) !=
		 Compression::OK )
		return false;
	const size_t compressedSize = static_cast<size_t>( destination.tell() );
	if ( compressedSize + 64 >= input.size() * 9 / 10 )
		return false;
	output.resize( compressedSize );
	return true;
}

} // namespace

KittyFramePresenter::~KittyFramePresenter() {
	{
		std::lock_guard<std::mutex> lock( mMutex );
		mRunning = false;
	}
	mCondition.notify_one();
	if ( mWorker.joinable() )
		mWorker.join();
}

bool KittyFramePresenter::initialize( Window& window ) {
	if ( !TerminalRuntime::instance().initialize() )
		return false;
	mPersistentUpdatesEnabled = environmentFlag( "EEPP_TERMINAL_PERSISTENT_UPDATES", true );
	mDamageUpdatesEnabled =
		mPersistentUpdatesEnabled && environmentFlag( "EEPP_TERMINAL_DAMAGE_UPDATES", true );
	mZlibCompressionLevel = environmentCompressionLevel();
	TerminalRuntime::instance().attach( window );
	mRunning = true;
	mWorker = std::thread( &KittyFramePresenter::run, this );
	return true;
}

void KittyFramePresenter::present( Window& window ) {
	Frame frame;
	{
		std::lock_guard<std::mutex> lock( mMutex );
		frame = std::move( mRecycle );
	}
	frame.size = window.getSize();
	frame.pixels.resize( static_cast<size_t>( frame.size.x ) * frame.size.y * 3 );
	FrameReadback readback{ frame.pixels.data(), frame.size,
							static_cast<size_t>( frame.size.x ) * 3 };
	if ( !window.readFrameBuffer( readback ) )
		return;

	{
		std::lock_guard<std::mutex> lock( mMutex );
		if ( mHasPending )
			mRecycle = std::move( mPending );
		mPending = std::move( frame );
		mHasPending = true;
	}
	mCondition.notify_one();
}

void KittyFramePresenter::shutdown( Window& ) {
	{
		std::lock_guard<std::mutex> lock( mMutex );
		mRunning = false;
	}
	mCondition.notify_one();
	if ( mWorker.joinable() )
		mWorker.join();
	TerminalRuntime::instance().detach();
	TerminalRuntime::instance().shutdown();
}

void KittyFramePresenter::run() {
	for ( ;; ) {
		Frame frame;
		{
			std::unique_lock<std::mutex> lock( mMutex );
			mCondition.wait( lock, [this] { return !mRunning || mHasPending; } );
			if ( !mRunning && !mHasPending )
				return;
			frame = std::move( mPending );
			mHasPending = false;
		}
		if ( sendFrame( frame ) )
			std::swap( mPresented, frame );
		std::lock_guard<std::mutex> lock( mMutex );
		mRecycle = std::move( frame );
	}
}

bool KittyFramePresenter::sendFrame( const Frame& frame ) {
	if ( frame.size.x <= 0 || frame.size.y <= 0 )
		return false;
	const DamageRectangle full{ 0, 0, frame.size.x, frame.size.y };
	if ( !mDamageUpdatesEnabled ) {
		if ( mPresented.size == frame.size && mPresented.pixels.size() == frame.pixels.size() &&
			 0 ==
				 std::memcmp( mPresented.pixels.data(), frame.pixels.data(), frame.pixels.size() ) )
			return true;
		extractRectangle( frame, full );
		const bool initial = !mPersistentUpdatesEnabled || mPresented.size != frame.size ||
							 mPresented.pixels.size() != frame.pixels.size();
		return sendTransfer( mTransferPixels, full, initial );
	}
	if ( mPresented.size != frame.size || mPresented.pixels.size() != frame.pixels.size() ) {
		extractRectangle( frame, full );
		return sendTransfer( mTransferPixels, full, true );
	}

	const DamageResult damage =
		findDamage( frame.pixels, mPresented.pixels, frame.size, mDamageRectangles );
	if ( damage == DamageResult::Full ) {
		extractRectangle( frame, full );
		return sendTransfer( mTransferPixels, full, false );
	}
	if ( damage == DamageResult::None )
		return true;
	for ( const DamageRectangle& rectangle : mDamageRectangles ) {
		extractRectangle( frame, rectangle );
		if ( !sendTransfer( mTransferPixels, rectangle, false ) )
			return false;
	}
	return true;
}

bool KittyFramePresenter::sendTransfer( const std::vector<Uint8>& pixels,
										const DamageRectangle& rectangle, bool initial ) {
	const bool useCompression = compressPixels( pixels, mCompressedPixels, mZlibCompressionLevel );
	const std::vector<Uint8>& payload = useCompression ? mCompressedPixels : pixels;
	std::array<char, 4097> encoded;
	std::array<char, 256 + 4096 + 2> packet;
	size_t offset = 0;
	bool first = true;
	while ( offset < payload.size() ) {
		const size_t count = eemin( static_cast<size_t>( 3072 ), payload.size() - offset );
		const size_t encodedSize =
			Base64::encode( count, payload.data() + offset, encoded.size(), encoded.data() );
		offset += count;
		char header[256];
		int headerSize;
		if ( first && initial ) {
			if ( mDamageUpdatesEnabled || mPersistentUpdatesEnabled ) {
				headerSize = std::snprintf(
					header, sizeof( header ),
					"\033_Gf=24,t=d,a=T,C=1,q=2,i=%u,p=%u,s=%d,v=%d%s,m=%d;", StreamImageId,
					StreamPlacementId, rectangle.width, rectangle.height,
					useCompression ? ",o=z" : "", offset < payload.size() ? 1 : 0 );
			} else {
				headerSize = std::snprintf(
					header, sizeof( header ), "\033_Gf=24,t=d,a=T,C=1,q=2,s=%d,v=%d%s,m=%d;",
					rectangle.width, rectangle.height, useCompression ? ",o=z" : "",
					offset < payload.size() ? 1 : 0 );
			}
		} else if ( first ) {
			headerSize = std::snprintf(
				header, sizeof( header ),
				"\033_Gf=24,t=d,a=f,q=2,i=%u,r=1,x=%d,y=%d,s=%d,v=%d,X=1%s,m=%d;", StreamImageId,
				rectangle.x, rectangle.y, rectangle.width, rectangle.height,
				useCompression ? ",o=z" : "", offset < payload.size() ? 1 : 0 );
		} else {
			headerSize = std::snprintf( header, sizeof( header ),
										initial ? "\033_Gm=%d;" : "\033_Ga=f,m=%d;",
										offset < payload.size() ? 1 : 0 );
		}
		if ( encodedSize == static_cast<size_t>( -1 ) || headerSize <= 0 ||
			 static_cast<size_t>( headerSize ) >= sizeof( header ) )
			return false;
		std::memcpy( packet.data(), header, static_cast<size_t>( headerSize ) );
		std::memcpy( packet.data() + headerSize, encoded.data(), encodedSize );
		packet[static_cast<size_t>( headerSize ) + encodedSize] = '\033';
		packet[static_cast<size_t>( headerSize ) + encodedSize + 1] = '\\';
		if ( !TerminalRuntime::instance().write( packet.data(), static_cast<size_t>( headerSize ) +
																	encodedSize + 2 ) )
			return false;
		first = false;
	}
	return true;
}

void KittyFramePresenter::extractRectangle( const Frame& frame, const DamageRectangle& rectangle ) {
	mTransferPixels.resize( static_cast<size_t>( rectangle.width ) * rectangle.height * 3 );
	const size_t frameStride = static_cast<size_t>( frame.size.x ) * 3;
	const size_t rectangleStride = static_cast<size_t>( rectangle.width ) * 3;
	for ( Int32 row = 0; row < rectangle.height; ++row ) {
		const Int32 sourceRow = frame.size.y - 1 - ( rectangle.y + row );
		std::memcpy( mTransferPixels.data() + static_cast<size_t>( row ) * rectangleStride,
					 frame.pixels.data() + static_cast<size_t>( sourceRow ) * frameStride +
						 static_cast<size_t>( rectangle.x ) * 3,
					 rectangleStride );
	}
}
}} // namespace EE::Window
