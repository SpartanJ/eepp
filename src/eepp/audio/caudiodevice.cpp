#include <eepp/audio/caudiodevice.hpp>
#include <eepp/audio/caudiolistener.hpp>

namespace EE { namespace Audio {

ALCdevice *		mDevice = NULL;
ALCcontext *	mContext = NULL;

cAudioDevice::cAudioDevice() {
	PrintInfo();

	// Create the device
	mDevice = alcOpenDevice( NULL );

	if ( mDevice ) {
		mContext = alcCreateContext( mDevice, NULL );

		if ( mContext ) {
			// Set the context as the current one (we'll only need one)
			alcMakeContextCurrent( mContext );

			// Initialize the listener, located at the origin and looking along the Z axis
			//cAudioListener::Position(0.f, 0.f, 0.f);
			//cAudioListener::Target(0.f, 0.f, -1.f);

			std::string log( "OpenAL current device:\n" );
			log += "\t" + std::string( (const char *)alcGetString(mDevice, ALC_DEVICE_SPECIFIER) );
			cLog::instance()->Write( log );
		} else {
			cLog::instance()->Write("Failed to create the audio context");
		}
	} else {
		cLog::instance()->Write("Failed to open the audio device");
	}
}

void cAudioDevice::PrintInfo() {
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

	cLog::instance()->Write( log );
}

cAudioDevice::~cAudioDevice() {
	// Destroy the context
	alcMakeContextCurrent( NULL );

	if ( mContext )
		alcDestroyContext( mContext );

	// Destroy the device
	if ( mDevice )
		alcCloseDevice( mDevice );
}

bool cAudioDevice::IsExtensionSupported( const std::string& extension ) {
	EnsureALInit();

    if ( ( extension.length() > 2 ) && ( extension.substr(0, 3) == "ALC" ) )
        return alcIsExtensionPresent( mDevice, extension.c_str() ) != AL_FALSE;
    else
        return alIsExtensionPresent( extension.c_str() ) != AL_FALSE;
}

ALenum cAudioDevice::GetFormatFromChannelsCount( unsigned int ChannelsCount ) {
	EnsureALInit();

	switch ( ChannelsCount ) {
		case 1 : return AL_FORMAT_MONO16;
		case 2 : return AL_FORMAT_STEREO16;
		case 4 : return alGetEnumValue("AL_FORMAT_QUAD16");
		case 6 : return alGetEnumValue("AL_FORMAT_51CHN16");
		case 7 : return alGetEnumValue("AL_FORMAT_61CHN16");
		case 8 : return alGetEnumValue("AL_FORMAT_71CHN16");
	}

	return 0;
}

bool cAudioDevice::IsAvailable() {
	return NULL != mDevice && NULL != mContext;
}

}}
