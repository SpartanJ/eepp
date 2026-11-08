#include "utest.h"
#include <array>
#include <condition_variable>
#include <eepp/audio/listener.hpp>
#include <eepp/audio/sound.hpp>
#include <eepp/audio/soundbuffer.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/window/engine.hpp>
#include <mutex>
#include <thread>

using namespace EE;
using namespace EE::Audio;
using namespace EE::System;
using namespace EE::Window;

namespace {

struct AudioWindowFixture {
	AudioWindowFixture() { Engine::destroySingleton(); }

	~AudioWindowFixture() { Engine::destroySingleton(); }

	static EE::Window::Window* createWindow() {
		return Engine::instance()->createWindow( WindowSettings{
			64, 64, "Audio/window lifetime", WindowStyle::Default, WindowBackend::Default } );
	}
};

} // namespace

UTEST( AudioWindowLifetime, PlaybackContinuesAcrossEngineRecreation ) {
	AudioWindowFixture fixture;
	auto* window = fixture.createWindow();
	ASSERT_TRUE( window != nullptr );
	if ( !EE::Audio::Listener::isPlaybackAvailable() )
		UTEST_SKIP( "No audio output device" );

	std::array<Int16, 8192> samples{};
	SoundBuffer buffer;
	ASSERT_TRUE( buffer.loadFromSamples( samples.data(), samples.size(), 1, 44100 ) );
	Sound sound( buffer );
	sound.setLoop( true );
	sound.play();
	for ( int cycle = 0; cycle < 3; ++cycle ) {
		Engine::destroySingleton();
		const auto previous = sound.getPlayingOffset();
		Clock clock;
		while ( sound.getPlayingOffset() == previous && clock.getElapsedTime() < Seconds( 2 ) )
			Sys::sleep( Milliseconds( 5 ) );
		EXPECT_EQ( sound.getStatus(), SoundSource::Playing );
		EXPECT_TRUE( sound.getPlayingOffset() != previous );
		ASSERT_TRUE( fixture.createWindow() != nullptr );
	}
	sound.stop();
}

UTEST( AudioWindowLifetime, WorkerCanReleaseLastAudioResourceDuringEngineShutdown ) {
	AudioWindowFixture fixture;
	auto* window = fixture.createWindow();
	ASSERT_TRUE( window != nullptr );
	// Without an output device there is no device to release, and each failed open can be slow.
	if ( !EE::Audio::Listener::isPlaybackAvailable() )
		UTEST_SKIP( "No audio output device" );

	for ( int cycle = 0; cycle < 16; ++cycle ) {
		std::mutex mutex;
		std::condition_variable condition;
		bool ready = false;
		bool release = false;
		bool loaded = false;
		std::thread worker( [&] {
			SoundBuffer buffer;
			std::array<Int16, 64> samples{};
			loaded = buffer.loadFromSamples( samples.data(), samples.size(), 1, 44100 );
			std::unique_lock<std::mutex> lock( mutex );
			ready = true;
			condition.notify_one();
			condition.wait( lock, [&] { return release; } );
			// Both native context and device are released here, on the worker.
		} );
		{
			std::unique_lock<std::mutex> lock( mutex );
			condition.wait( lock, [&] { return ready; } );
			release = true;
		}
		condition.notify_one();
		Engine::destroySingleton();
		worker.join();
		// Join before asserting so a failure cannot leave a joinable thread behind.
		ASSERT_TRUE( loaded );
		ASSERT_TRUE( fixture.createWindow() != nullptr );
	}
}
