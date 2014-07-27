#include <eepp/audio/audiodevice.hpp>
#include <eepp/audio/audiolistener.hpp>
#include <eepp/audio/openal.hpp>

namespace EE { namespace Audio {

ALCdevice *		mDevice = NULL;
ALCcontext *	mContext = NULL;

AudioDevice::AudioDevice() {
	PrintInfo();

	// Create the device
	mDevice = alcOpenDevice( NULL );

	if ( mDevice ) {
		mContext = alcCreateContext( mDevice, NULL );

		if ( mContext ) {
			// Set the context as the current one (we'll only need one)
			alcMakeContextCurrent( mContext );

			// Initialize the listener, located at the origin and looking along the Z axis
			#if EE_PLATFORM != EE_PLATFORM_EMSCRIPTEN
			ALCheck( alListenerf( AL_GAIN, 1.f ) );
			#endif

			float Position[] = {0.f, 0.f, 0.f};
			ALCheck( alListenerfv( AL_POSITION, Position ) );

			float Orientation[] = {0.f, 0.f, -1.f, 0.f, 1.f, 0.f};
			ALCheck( alListenerfv( AL_ORIENTATION, Orientation ) );

			std::string log( "OpenAL current device:\n" );
			log += "\t" + std::string( (const char *)alcGetString(mDevice, ALC_DEVICE_SPECIFIER) );
			eePRINTL( log.c_str() );
		} else {
			eePRINTL("Failed to create the audio context");
		}
	} else {
		eePRINTL("Failed to open the audio device");
	}
}

void AudioDevice::PrintInfo() {
	std::string log( "OpenAL devices detected:\n" );

	if ( alcIsExtensionPresent( NULL, (const ALCchar *) "ALC_ENUMERATION_EXT" ) == AL_TRUE ) {
		const char *s = (const char *) alcGetString(NULL, ALC_DEVICE_SPECIFIER);

		while (*s != '\0') {
			log += "\t" + std::string( s ) + "\n";
			while (*s++ != '\0');
		}
	} else {
		log += "OpenAL device enumeration isn't available.";
	}

	log += "OpenAL default device:\n";
	log += "\t" + std::string( (const char *)alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER) );

	eePRINTL( log.c_str() );
}

AudioDevice::~AudioDevice() {
	// Destroy the context
	alcMakeContextCurrent( NULL );

	if ( mContext )
		alcDestroyContext( mContext );

	// Destroy the device
	if ( mDevice )
		alcCloseDevice( mDevice );
}

bool AudioDevice::IsExtensionSupported( const std::string& extension ) {
	EnsureALInit();

	if ( ( extension.length() > 2 ) && ( extension.substr(0, 3) == "ALC" ) )
		return alcIsExtensionPresent( mDevice, extension.c_str() ) != AL_FALSE;
	else
		return alIsExtensionPresent( extension.c_str() ) != AL_FALSE;
}

int AudioDevice::GetFormatFromChannelCount( unsigned int ChannelCount ) {
	EnsureALInit();

	int format = 0;

	switch ( ChannelCount ) {
		case 1 : format = AL_FORMAT_MONO16;						break;
		case 2 : format = AL_FORMAT_STEREO16;					break;
		case 4 : format = alGetEnumValue("AL_FORMAT_QUAD16");	break;
		case 6 : format = alGetEnumValue("AL_FORMAT_51CHN16");	break;
		case 7 : format = alGetEnumValue("AL_FORMAT_61CHN16");	break;
		case 8 : format = alGetEnumValue("AL_FORMAT_71CHN16");	break;
		default: format = 0;
	}

	if ( -1 == format )
		format = 0;

	return format;
}

bool AudioDevice::IsAvailable() {
	return NULL != mDevice && NULL != mContext;
}

}}
