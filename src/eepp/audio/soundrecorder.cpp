#include <cassert>
#include <cstring>
#include <eepp/audio/alcheck.hpp>
#include <eepp/audio/audiodevice.hpp>
#include <eepp/audio/soundrecorder.hpp>
#include <eepp/core/debug.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/sys.hpp>

#ifdef _MSC_VER
#pragma warning( disable : 4355 ) // 'this' used in base member initializer list
#endif

namespace {
ALCdevice* captureDevice = NULL;
}

namespace EE { namespace Audio {

SoundRecorder::SoundRecorder() :
	mThread( &SoundRecorder::record, this ),
	mSampleRate( 0 ),
	mProcessingInterval( Milliseconds( 100 ) ),
	mIsCapturing( false ),
	mDeviceName( getDefaultDevice() ),
	mChannelCount( 1 ) {}

SoundRecorder::~SoundRecorder() {
	// This assertion is triggered if the recording is still running while
	// the object is destroyed. It ensures that stop() is called in the
	// destructor of the derived class, which makes sure that the recording
	// thread finishes before the derived object is destroyed. Otherwise a
	// "pure virtual method called" exception is triggered.
	assert( !mIsCapturing &&
			"You must call stop() in the destructor of your derived class, so that the recording "
			"thread finishes before your object is destroyed." );
}

bool SoundRecorder::start( unsigned int sampleRate ) {
	// Check if the device can do audio capture
	if ( !isAvailable() ) {
		Log::error( "Failed to start capture: your system cannot capture audio data (call "
					"SoundRecorder::isAvailable to check it)" );
		return false;
	}

	// Check that another capture is not already running
	if ( captureDevice ) {
		Log::error( "Trying to start audio capture, but another capture is already running" );
		return false;
	}

	// Determine the recording format
	ALCenum format = ( mChannelCount == 1 ) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

	// Open the capture device for capturing 16 bits samples
	captureDevice = alcCaptureOpenDevice( mDeviceName.c_str(), sampleRate, format, sampleRate );
	if ( !captureDevice ) {
		Log::error( "Failed to open the audio capture device with the name: %s",
					mDeviceName.c_str() );
		return false;
	}

	// Clear the array of samples
	mSamples.clear();

	// Store the sample rate
	mSampleRate = sampleRate;

	// Notify derived class
	if ( onStart() ) {
		// Start the capture
		alcCaptureStart( captureDevice );

		// Start the capture in a new thread, to avoid blocking the main thread
		mIsCapturing = true;
		mThread.launch();

		return true;
	}

	return false;
}

void SoundRecorder::stop() {
	// Stop the capturing thread if there is one
	if ( mIsCapturing ) {
		mIsCapturing = false;
		mThread.wait();

		// Notify derived class
		onStop();
	}
}

unsigned int SoundRecorder::getSampleRate() const {
	return mSampleRate;
}

std::vector<std::string> SoundRecorder::getAvailableDevices() {
	std::vector<std::string> deviceNameList;

	const ALchar* deviceList = alcGetString( NULL, ALC_CAPTURE_DEVICE_SPECIFIER );
	if ( deviceList ) {
		while ( *deviceList ) {
			deviceNameList.push_back( deviceList );
			deviceList += std::strlen( deviceList ) + 1;
		}
	}

	return deviceNameList;
}

std::string SoundRecorder::getDefaultDevice() {
	return alcGetString( NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER );
}

bool SoundRecorder::setDevice( const std::string& name ) {
	// Store the device name
	if ( name.empty() ) {
		mDeviceName = getDefaultDevice();
	} else {
		mDeviceName = name;
	}

	if ( mIsCapturing ) {
		// Stop the capturing thread
		mIsCapturing = false;
		mThread.wait();

		// Determine the recording format
		ALCenum format = ( mChannelCount == 1 ) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;

		// Open the requested capture device for capturing 16 bits samples
		captureDevice =
			alcCaptureOpenDevice( mDeviceName.c_str(), mSampleRate, format, mSampleRate );
		if ( !captureDevice ) {
			// Notify derived class
			onStop();

			Log::error( "Failed to open the audio capture device with the name: %s",
						mDeviceName.c_str() );
			return false;
		}

		// Start the capture
		alcCaptureStart( captureDevice );

		// Start the capture in a new thread, to avoid blocking the main thread
		mIsCapturing = true;
		mThread.launch();
	}

	return true;
}

const std::string& SoundRecorder::getDevice() const {
	return mDeviceName;
}

void SoundRecorder::setChannelCount( unsigned int channelCount ) {
	if ( mIsCapturing ) {
		Log::error( "It's not possible to change the channels while recording." );
		return;
	}

	if ( channelCount < 1 || channelCount > 2 ) {
		Log::error(
			"Unsupported channel count: %d Currently only mono (1) and stereo (2) recording "
			"is supported.",
			channelCount );
		return;
	}

	mChannelCount = channelCount;
}

unsigned int SoundRecorder::getChannelCount() const {
	return mChannelCount;
}

bool SoundRecorder::isAvailable() {
	return ( Private::AudioDevice::isExtensionSupported( "ALC_EXT_CAPTURE" ) != AL_FALSE ) ||
		   ( Private::AudioDevice::isExtensionSupported( "ALC_EXT_capture" ) !=
			 AL_FALSE ); // "bug" in Mac OS X 10.5 and 10.6
}

void SoundRecorder::setProcessingInterval( Time interval ) {
	mProcessingInterval = interval;
}

bool SoundRecorder::onStart() {
	// Nothing to do
	return true;
}

void SoundRecorder::onStop() {
	// Nothing to do
}

bool SoundRecorder::isCapturing() {
	return mIsCapturing;
}

void SoundRecorder::record() {
	while ( mIsCapturing ) {
		// Process available samples
		processCapturedSamples();

		// Don't bother the CPU while waiting for more captured data
		Sys::sleep( mProcessingInterval );
	}

	// Capture is finished: clean up everything
	cleanup();
}

void SoundRecorder::processCapturedSamples() {
	// Get the number of samples available
	ALCint samplesAvailable;
	alcGetIntegerv( captureDevice, ALC_CAPTURE_SAMPLES, 1, &samplesAvailable );

	if ( samplesAvailable > 0 ) {
		// Get the recorded samples
		mSamples.resize( samplesAvailable * getChannelCount() );
		alcCaptureSamples( captureDevice, &mSamples[0], samplesAvailable );

		// Forward them to the derived class
		if ( !onProcessSamples( &mSamples[0], mSamples.size() ) ) {
			// The user wants to stop the capture
			mIsCapturing = false;
		}
	}
}

void SoundRecorder::cleanup() {
	// Stop the capture
	alcCaptureStop( captureDevice );

	// Get the samples left in the buffer
	processCapturedSamples();

	// Close the device
	alcCaptureCloseDevice( captureDevice );
	captureDevice = NULL;
}

}} // namespace EE::Audio
