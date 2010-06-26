#include "caudiodevice.hpp"
#include "caudiolistener.hpp"

namespace EE { namespace Audio {

cAudioDevice* cAudioDevice::ourInstance;

cAudioDevice::cAudioDevice() : myRefCount(0) {
	// Create the device
	myDevice = alcOpenDevice(NULL);

	if (myDevice) {
		myContext = alcCreateContext(myDevice, NULL);
		
		if (myContext) {
			// Set the context as the current one (we'll only need one)
			alcMakeContextCurrent(myContext);
			
			// Initialize the listener, located at the origin and looking along the Z axis
			cAudioListener::instance()->SetPosition(0.f, 0.f, 0.f);
			cAudioListener::instance()->SetTarget(0.f, 0.f, -1.f);
		} else
			cLog::instance()->Write("Failed to create the audio context");
	} else
		cLog::instance()->Write("Failed to open the audio device");
}

cAudioDevice::~cAudioDevice() {
	// Destroy the context
	alcMakeContextCurrent(NULL);
	if (myContext)
		alcDestroyContext(myContext);
	
	// Destroy the device
	if (myDevice)
		alcCloseDevice(myDevice);
}

bool cAudioDevice::isCreated() {
	return myContext && myDevice;
}

cAudioDevice& cAudioDevice::GetInstance() {
	// Create the audio device if it doesn't exist
	if (!ourInstance)
		ourInstance = new cAudioDevice;
	
	return *ourInstance;
}

cAudioDevice& cAudioDevice::instance() {
	if (!ourInstance)
		return cAudioDevice::GetInstance();
	
	return *ourInstance;
}

void cAudioDevice::AddReference() {
	// Create the audio device if it doesn't exist
	if (!ourInstance)
		ourInstance = new cAudioDevice;
	
	// Increase the references count
	ourInstance->myRefCount++;
}

void cAudioDevice::RemoveReference() {
	// Decrease the references count
	ourInstance->myRefCount--;

	// Destroy the audio device if the references count reaches 0
	if (ourInstance->myRefCount == 0) {
		delete ourInstance;
		ourInstance = NULL;
	}
}

ALCdevice* cAudioDevice::GetDevice() const {
	return myDevice;
}

ALenum cAudioDevice::GetFormatFromChannelsCount(unsigned int ChannelsCount) const {
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
