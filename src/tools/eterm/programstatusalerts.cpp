#include "programstatusalerts.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <eepp/audio/sound.hpp>
#include <eepp/audio/soundbuffer.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/notification.hpp>
#include <eepp/system/threadpool.hpp>

namespace eterm {

ProgramStatusAlerts::ProgramStatusAlerts() = default;

ProgramStatusAlerts::~ProgramStatusAlerts() {
	shutdown();
}

void ProgramStatusAlerts::shutdown() {
	mWorker.reset();
	mSound.reset();
	mBuffer.reset();
	mLoadedSound.clear();
}

bool ProgramStatusAlerts::deliver( std::string title, std::string message, bool notification,
								   bool sound, std::string soundName, int volume, bool preview,
								   std::function<void( bool, std::string )> completed ) {
	if ( ( !notification && !sound ) ||
		 ( !preview && mDelivered && mLastDelivery.getElapsedTime() < Seconds( 2 ) ) ||
		 mBusy.exchange( true ) )
		return false;
	mDelivered = true;
	mLastDelivery.restart();
	if ( !mWorker )
		mWorker = ThreadPool::createUnique( 1 );
	mWorker->run( [this, title = std::move( title ), message = std::move( message ), notification,
				   sound, soundName = std::move( soundName ), volume,
				   completed = std::move( completed )] {
		if ( sound )
			playSound( soundName, volume );
		std::string error;
		const bool success = !notification || Notification::show( title, message, &error );
		if ( !success )
			Log::warning( "eterm: %s", error.c_str() );
		mBusy = false;
		if ( completed )
			completed( success, std::move( error ) );
	} );
	return true;
}

void ProgramStatusAlerts::playSound( const std::string& name, int volume ) {
	if ( !mBuffer ) {
		mBuffer = std::make_unique<SoundBuffer>();
		mSound = std::make_unique<Sound>();
	}
	if ( mLoadedSound != name ) {
		mSound->stop();
		bool loaded = false;
		if ( name == "chime" || name == "bell" || name == "pulse" ) {
			// Original synthesized cues, generated once per selection. No asset lookup or decoding
			// on the UI thread, and no audio device is created while sound remains disabled.
			constexpr unsigned rate = 22050;
			constexpr double pi = 3.14159265358979323846;
			std::array<Int16, rate / 2> samples;
			for ( size_t i = 0; i < samples.size(); ++i ) {
				const double t = static_cast<double>( i ) / rate;
				const double frequency = name == "bell"	   ? 880
										 : name == "pulse" ? 440
														   : ( t < .18 ? 660 : 880 );
				const double age = name == "chime" && t >= .18 ? t - .18 : t;
				const double envelope = std::min( age * 100, 1.0 ) * std::exp( -age * 9 ) *
										std::min( ( .5 - t ) * 100, 1.0 );
				samples[i] =
					static_cast<Int16>( 10000 * envelope * std::sin( 2 * pi * frequency * t ) );
			}
			loaded = mBuffer->loadFromSamples( samples.data(), samples.size(), 1, rate );
		} else {
			loaded = mBuffer->loadFromFile( name );
		}
		if ( !loaded ) {
			Log::warning( "eterm: could not load program status sound: %s", name.c_str() );
			mLoadedSound.clear();
			return;
		}
		mLoadedSound = name;
		mSound->setBuffer( *mBuffer );
	}
	mSound->setVolume( std::clamp( volume, 0, 100 ) );
	mSound->play();
}

} // namespace eterm
