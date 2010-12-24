#include "caudiodevice.hpp"
#include "caudiolistener.hpp"

namespace EE { namespace Audio {

cAudioDevice * cAudioDevice::mInstance = NULL;

cAudioDevice::cAudioDevice() :
	mDevice(NULL),
	mContext(NULL),
	mRefCount(0)
{
	PrintInfo();

	// Create the device
	mDevice = alcOpenDevice( NULL );

	if ( mDevice ) {
		mContext = alcCreateContext( mDevice, NULL );

		if ( mContext ) {
			// Set the context as the current one (we'll only need one)
			alcMakeContextCurrent( mContext );

			// Initialize the listener, located at the origin and looking along the Z axis
			cAudioListener::instance()->SetPosition(0.f, 0.f, 0.f);
			cAudioListener::instance()->SetTarget(0.f, 0.f, -1.f);

			cLog::instance()->Write( "OpenAL current device: " );
			cLog::instance()->Write( "\t" + std::string( (const char *)alcGetString(mDevice, ALC_DEVICE_SPECIFIER) ) );
		} else {
			cLog::instance()->Write("Failed to create the audio context");
		}
	} else {
		cLog::instance()->Write("Failed to open the audio device");
	}
}

void cAudioDevice::PrintInfo() {
	cLog::instance()->Write( "OpenAL devices detected:" );

	if ( alcIsExtensionPresent( NULL, (const ALCchar *) "ALC_ENUMERATION_EXT" ) == AL_TRUE ) {
		const char *s = (const char *) alcGetString(NULL, ALC_DEVICE_SPECIFIER);

		while (*s != '\0') {
			cLog::instance()->Write( "\t" + std::string( s ) );
			while (*s++ != '\0');
		}
	} else {
		cLog::instance()->Write( "OpenAL device enumeration isn't available." );
	}

	cLog::instance()->Write( "OpenAL default device: " );
	cLog::instance()->Write( "\t" + std::string( (const char *)alcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER) ) );
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

bool cAudioDevice::isCreated() {
	return mContext && mDevice;
}

cAudioDevice * cAudioDevice::instance() {
	// Create the audio device if it doesn't exist
	if ( NULL == mInstance )
		mInstance = eeNew( cAudioDevice, () );

	return mInstance;
}

void cAudioDevice::AddReference() {
	cAudioDevice::instance();

	// Increase the references count
	mInstance->mRefCount++;
}

void cAudioDevice::RemoveReference() {
	// Decrease the references count
	mInstance->mRefCount--;

	// Destroy the audio device if the references count reaches 0
	if (mInstance->mRefCount == 0) {
		eeDelete( mInstance );
		mInstance = NULL;
	}
}

ALCdevice * cAudioDevice::GetDevice() const {
	return mDevice;
}

bool cAudioDevice::IsExtensionSupported( const std::string& extension ) {
	cAudioDevice::instance();

    if ( ( extension.length() > 2 ) && ( extension.substr(0, 3) == "ALC" ) )
        return alcIsExtensionPresent( mDevice, extension.c_str() ) != AL_FALSE;
    else
        return alIsExtensionPresent( extension.c_str() ) != AL_FALSE;
}

ALenum cAudioDevice::GetFormatFromChannelsCount(unsigned int ChannelsCount) const {
	cAudioDevice::instance();

	switch (ChannelsCount) {
		case 1 : return AL_FORMAT_MONO16;
		case 2 : return AL_FORMAT_STEREO16;
		case 4 : return alGetEnumValue("AL_FORMAT_QUAD16");
		case 6 : return alGetEnumValue("AL_FORMAT_51CHN16");
		case 7 : return alGetEnumValue("AL_FORMAT_61CHN16");
		case 8 : return alGetEnumValue("AL_FORMAT_71CHN16");
	}
	return 0;
}

}}
