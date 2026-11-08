#include <eepp/audio/alcheck.hpp>
#include <eepp/audio/alresource.hpp>
#include <eepp/audio/audiodevice.hpp>
#include <eepp/audio/listener.hpp>
#include <eepp/core/core.hpp>
#include <eepp/system/clock.hpp>
#include <eepp/system/log.hpp>
#include <memory>

namespace {
ALCdevice* audioDevice = NULL;
ALCcontext* audioContext = NULL;

float listenerVolume = 100.f;
Vector3f listenerPosition( 0.f, 0.f, 0.f );
Vector3f listenerDirection( 0.f, 0.f, -1.f );
Vector3f listenerUpVector( 0.f, 1.f, 0.f );

// Holds the shared device open for the duration of a static query. Going through AlResource
// keeps its lock and count authoritative: a temporary AudioDevice created outside them could
// overwrite, and on deletion close, a device a concurrent resource has just opened.
struct DeviceReference : EE::Audio::AlResource {};
} // namespace

using namespace EE::System;

namespace EE { namespace Audio { namespace Private {

AudioDevice* AudioDevice::New() {
	return eeNew( AudioDevice, () );
}

AudioDevice::AudioDevice() {
	Clock c;

	// Create the device
	audioDevice = alcOpenDevice( NULL );
	Log::debug( "alcOpenDevice took: %s", c.getElapsedTimeAndReset().toString() );

	if ( audioDevice ) {
		// Create the context
		audioContext = alcCreateContext( audioDevice, NULL );
		Log::debug( "alcCreateContext took: %s", c.getElapsedTimeAndReset().toString() );

		if ( audioContext ) {
			// Set the context as the current one (we'll only need one)
			alcMakeContextCurrent( audioContext );
			Log::debug( "alcMakeContextCurrent took: %s", c.getElapsedTimeAndReset().toString() );

			// Apply the listener properties the user might have set
			float orientation[] = { listenerDirection.x, listenerDirection.y, listenerDirection.z,
									listenerUpVector.x,	 listenerUpVector.y,  listenerUpVector.z };
			alCheck( alListenerf( AL_GAIN, listenerVolume * 0.01f ) );
			alCheck( alListener3f( AL_POSITION, listenerPosition.x, listenerPosition.y,
								   listenerPosition.z ) );
			alCheck( alListenerfv( AL_ORIENTATION, orientation ) );
		} else {
			Log::error( "Failed to create the audio context" );
		}
	} else {
		Log::error( "Failed to open the audio device" );
	}
}

AudioDevice::~AudioDevice() {
	// Destroy the context
	alcMakeContextCurrent( NULL );
	if ( audioContext )
		alcDestroyContext( audioContext );
	audioContext = NULL;

	// Destroy the device, leaving no dangling globals for later queries.
	if ( audioDevice )
		alcCloseDevice( audioDevice );
	audioDevice = NULL;
}

bool AudioDevice::isPlaybackAvailable() {
	DeviceReference reference;
	return audioDevice != NULL && audioContext != NULL;
}

bool AudioDevice::isExtensionSupported( const std::string& extension ) {
	// Makes sure there is a valid OpenAL device for extension queries.
	DeviceReference reference;
	if ( ( extension.length() > 2 ) && ( extension.substr( 0, 3 ) == "ALC" ) )
		return alcIsExtensionPresent( audioDevice, extension.c_str() ) != AL_FALSE;
	return alIsExtensionPresent( extension.c_str() ) != AL_FALSE;
}

int AudioDevice::getFormatFromChannelCount( unsigned int channelCount ) {
	// Makes sure there is a valid OpenAL device for format queries.
	DeviceReference reference;

	// Find the good format according to the number of channels
	int format = 0;
	switch ( channelCount ) {
		case 1:
			format = AL_FORMAT_MONO16;
			break;
		case 2:
			format = AL_FORMAT_STEREO16;
			break;
		case 4:
			format = alGetEnumValue( "AL_FORMAT_QUAD16" );
			break;
		case 6:
			format = alGetEnumValue( "AL_FORMAT_51CHN16" );
			break;
		case 7:
			format = alGetEnumValue( "AL_FORMAT_61CHN16" );
			break;
		case 8:
			format = alGetEnumValue( "AL_FORMAT_71CHN16" );
			break;
		default:
			format = 0;
			break;
	}

	// Fixes a bug on OS X
	if ( format == -1 )
		format = 0;

	return format;
}

void AudioDevice::setGlobalVolume( float volume ) {
	if ( audioContext )
		alCheck( alListenerf( AL_GAIN, volume * 0.01f ) );

	listenerVolume = volume;
}

float AudioDevice::getGlobalVolume() {
	return listenerVolume;
}

void AudioDevice::setPosition( const Vector3f& position ) {
	if ( audioContext )
		alCheck( alListener3f( AL_POSITION, position.x, position.y, position.z ) );

	listenerPosition = position;
}

Vector3f AudioDevice::getPosition() {
	return listenerPosition;
}

void AudioDevice::setDirection( const Vector3f& direction ) {
	if ( audioContext ) {
		float orientation[] = { direction.x,		direction.y,		direction.z,
								listenerUpVector.x, listenerUpVector.y, listenerUpVector.z };
		alCheck( alListenerfv( AL_ORIENTATION, orientation ) );
	}

	listenerDirection = direction;
}

Vector3f AudioDevice::getDirection() {
	return listenerDirection;
}

void AudioDevice::setUpVector( const Vector3f& upVector ) {
	if ( audioContext ) {
		float orientation[] = { listenerDirection.x, listenerDirection.y, listenerDirection.z,
								upVector.x,			 upVector.y,		  upVector.z };
		alCheck( alListenerfv( AL_ORIENTATION, orientation ) );
	}

	listenerUpVector = upVector;
}

Vector3f AudioDevice::getUpVector() {
	return listenerUpVector;
}

}}} // namespace EE::Audio::Private
