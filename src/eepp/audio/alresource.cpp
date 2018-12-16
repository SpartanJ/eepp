#include <eepp/audio/alresource.hpp>
#include <eepp/audio/audiodevice.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/lock.hpp>
using namespace EE::System;

namespace
{
	// OpenAL resources counter and its mutex
	unsigned int count = 0;
	Mutex mutex;

	// The audio device is instantiated on demand rather than at global startup,
	// which solves a lot of weird crashes and errors.
	// It is destroyed when it is no longer needed.
	EE::Audio::Private::AudioDevice* globalDevice;
}

namespace EE { namespace Audio {

AlResource::AlResource()
{
	// Protect from concurrent access
	Lock lock(mutex);

	// If this is the very first resource, trigger the global device initialization
	if (count == 0)
		globalDevice = new EE::Audio::Private::AudioDevice;

	// Increment the resources counter
	count++;
}

AlResource::~AlResource()
{
	// Protect from concurrent access
	Lock lock(mutex);

	// Decrement the resources counter
	count--;

	// If there's no more resource alive, we can destroy the device
	if (count == 0)
		delete globalDevice;
}

}}
