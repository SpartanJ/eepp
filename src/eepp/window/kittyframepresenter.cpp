#include <eepp/system/base64.hpp>
#include <eepp/window/terminal/kittyframepresenter.hpp>
#include <eepp/window/terminal/terminalruntime.hpp>
#include <eepp/window/window.hpp>

#include <array>
#include <cstdio>
#include <cstring>

using namespace EE::System;

namespace EE { namespace Window {

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
		sendFrame( frame );
		std::lock_guard<std::mutex> lock( mMutex );
		mRecycle = std::move( frame );
	}
}

void KittyFramePresenter::sendFrame( const Frame& frame ) {
	std::array<unsigned char, 3072> binary;
	std::array<char, 4097> encoded;
	std::array<char, 128 + 4096 + 2> packet;
	const size_t rowBytes = static_cast<size_t>( frame.size.x ) * 3;
	// glReadPixels returns bottom-left-origin rows while Kitty consumes top-left-origin pixels.
	// Reverse row traversal during streaming so no full-frame flip buffer is needed.
	Int32 row = frame.size.y - 1;
	size_t column = 0;
	bool first = true;
	while ( row >= 0 ) {
		size_t count = 0;
		while ( count < binary.size() && row >= 0 ) {
			const size_t copy = eemin( binary.size() - count, rowBytes - column );
			std::memcpy( binary.data() + count,
						 frame.pixels.data() + static_cast<size_t>( row ) * rowBytes + column,
						 copy );
			count += copy;
			column += copy;
			if ( column == rowBytes ) {
				column = 0;
				--row;
			}
		}
		const size_t encodedSize =
			Base64::encode( count, binary.data(), encoded.size(), encoded.data() );
		char header[128];
		const int headerSize =
			first
				? std::snprintf( header, sizeof( header ), "\033_Gf=24,t=d,a=T,C=1,s=%d,v=%d,m=%d;",
								 frame.size.x, frame.size.y, row >= 0 ? 1 : 0 )
				: std::snprintf( header, sizeof( header ), "\033_Gm=%d;", row >= 0 ? 1 : 0 );
		if ( encodedSize == static_cast<size_t>( -1 ) || headerSize <= 0 ||
			 static_cast<size_t>( headerSize ) >= sizeof( header ) )
			return;
		std::memcpy( packet.data(), header, static_cast<size_t>( headerSize ) );
		std::memcpy( packet.data() + headerSize, encoded.data(), encodedSize );
		packet[static_cast<size_t>( headerSize ) + encodedSize] = '\033';
		packet[static_cast<size_t>( headerSize ) + encodedSize + 1] = '\\';
		if ( !TerminalRuntime::instance().write( packet.data(), static_cast<size_t>( headerSize ) +
																	encodedSize + 2 ) )
			return;
		first = false;
	}
}
}} // namespace EE::Window
