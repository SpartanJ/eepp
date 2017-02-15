#include <eepp/audio/soundstream.hpp>
#include <eepp/audio/audiodevice.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/audio/openal.hpp>
using namespace EE::System;

namespace EE { namespace Audio {

SoundStream::SoundStream() :
	mIsStreaming(false),
	mChannelCount(0),
	mSampleRate (0),
	mFormat (0),
	mLoop(false),
	mSamplesProcessed(0)
{
}

SoundStream::~SoundStream() {
	stop(); // Stop the sound if it was playing
}

void SoundStream::initialize(unsigned int ChannelCount, unsigned int SampleRate) {
	mChannelCount	= ChannelCount;
	mSampleRate		= SampleRate;

	// Deduce the format from the number of channels
	mFormat = AudioDevice::getFormatFromChannelCount(ChannelCount);

	if ( mFormat == 0 ) { // Check if the format is valid
		mChannelCount = 0;
		mSampleRate	= 0;
		eePRINTL( "Unsupported number of channels." );
	}
}

void SoundStream::play() {
	if ( mFormat == 0 ) { // Check if the sound parameters have been set
		eePRINTL( "Failed to play audio stream : sound parameters have not been initialized (call Initialize first)." );
		return;
	}

	if ( mIsStreaming ) { // If the sound is already playing (probably paused), just resume it
		Sound::play();
		return;
	}

	onSeek( Time::Zero );

	mSamplesProcessed = 0;
	mIsStreaming = true; // Start updating the stream in a separate thread to avoid blocking the application
	launch();
}

void SoundStream::pause() {
	ALCheck( alSourcePause( mSource ) );
}

void SoundStream::stop() {
	mIsStreaming = false; // Wait for the thread to terminate
	wait();
}

unsigned int SoundStream::getChannelCount() const {
	return mChannelCount;
}

unsigned int SoundStream::getSampleRate() const {
	return mSampleRate;
}

Sound::Status SoundStream::getState() const {
	Status status = Sound::getState();

	if ( ( status == Sound::Stopped ) && mIsStreaming ) // To compensate for the lag between Play() and alSourcePlay()
		status = Sound::Playing;

	return status;
}

Time SoundStream::playingOffset() const {
	if ( mSampleRate && mChannelCount ) {
		float secs = 0.f;

		ALCheck( alGetSourcef( mSource, AL_SEC_OFFSET, &secs ) );

		return Seconds( secs + (float)mSamplesProcessed / (float)mSampleRate / (float)mChannelCount );
	}

	return Time::Zero;
}

void SoundStream::playingOffset( const Time &timeOffset ) {
	Status oldStatus = state();

	// Stop the stream
	stop();

	// Let the derived class update the current position
	onSeek( timeOffset );

	// Restart streaming
	mSamplesProcessed = static_cast<Uint32>( timeOffset.asSeconds() ) * mSampleRate * mChannelCount;

	mIsStreaming = true;

	launch();

	// Recover old status
	if ( oldStatus == Stopped ) {
		stop();
	} else if ( oldStatus == Paused ) {
		pause();
	}
}

void SoundStream::loop( const bool& Loop ) {
	mLoop = Loop;
}

bool SoundStream::loop() const {
	return mLoop;
}

void SoundStream::run() {
	ALCheck( alGenBuffers( BuffersCount, mBuffers ) );

	for ( int i = 0; i < BuffersCount; ++i )
		mEndBuffers[i] = false;

	// Fill the queue
	bool RequestStop = fillQueue();

	Sound::play();

	while ( mIsStreaming ) {
		// The stream has been interrupted !
		if ( Sound::getState() == Sound::Stopped ) {
			// User requested to stop : finish the streaming loop
			if ( !RequestStop ) {
				Sound::play();
			} else {
				// Streaming is not completed : restart the sound
				mIsStreaming = false;
			}
		}

		// Get the number of buffers that have been processed (ie. ready for reuse)
		ALint NbProcessed = 0;
		ALCheck( alGetSourcei( Sound::mSource, AL_BUFFERS_PROCESSED, &NbProcessed ) );

		while ( NbProcessed-- ) {
			// Pop the first unused buffer from the queue
			ALuint Buffer;
			ALCheck( alSourceUnqueueBuffers( Sound::mSource, 1, &Buffer ) );

			// Find its number
			Uint32 bufferNum = 0;
			for (int i = 0; i < BuffersCount; ++i) {
				if ( mBuffers[i] == Buffer ) {
					bufferNum = i;
					break;
				}
			}

			// Retrieve its size and add it to the samples count
			if ( mEndBuffers[bufferNum] ) {
				// This was the last buffer: reset the sample count
				mSamplesProcessed = 0;
				mEndBuffers[bufferNum] = false;
			} else {
				ALint size, bits;
				ALCheck( alGetBufferi( Buffer, AL_SIZE, &size ) );
				ALCheck( alGetBufferi( Buffer, AL_BITS, &bits ) );
				mSamplesProcessed += size / (bits / 8);
			}

			// Fill it and push it back into the playing queue
			if ( !RequestStop ) {
				if ( fillAndPushBuffer( bufferNum ) )
					RequestStop = true;
			}
		}

		// Leave some time for the other threads if the stream is still playing
		if ( Sound::getState() != Sound::Stopped )
			Sys::sleep(10);
	}

	// Stop the playback
	Sound::stop();

	// Unqueue any buffer left in the queue
	clearQueue();

	// Delete the buffers
	ALCheck( alSourcei( Sound::mSource, AL_BUFFER, 0 ) );
	ALCheck( alDeleteBuffers( BuffersCount, mBuffers ) );
}

bool SoundStream::fillAndPushBuffer( const unsigned int& Buffer ) {
	bool RequestStop = false;

	// Acquire audio data
	Chunk Data = {NULL, 0};

	if ( !onGetData( Data ) ) {
		// Mark the buffer as the last one (so that we know when to reset the playing position)
		mEndBuffers[ Buffer ] = true;

		// Check if the stream must loop or stop
		if ( mLoop ) {
			// Return to the beginning of the stream source
			onSeek( Time::Zero );

			// If we previously had no data, try to fill the buffer once again
			if ( !Data.Samples || ( Data.SamplesCount == 0 ) ) {
				return fillAndPushBuffer( Buffer );
			}
		} else {
			// Not looping: request stop
			RequestStop = true;
		}
	}

	// Create and fill the buffer, and push it to the queue
	if ( Data.Samples && Data.SamplesCount ) {
		Uint32 buffer = mBuffers[ Buffer ];

		// Fill the buffer
		ALsizei Size = static_cast<ALsizei>( Data.SamplesCount ) * sizeof(Int16);

		ALCheck( alBufferData( buffer, mFormat, Data.Samples, Size, mSampleRate ) );

		// Push it into the sound queue
		ALCheck( alSourceQueueBuffers( Sound::mSource, 1, &buffer ) );
	}

	return RequestStop;
}

bool SoundStream::fillQueue() {
	// Fill and enqueue all the available buffers
	bool RequestStop = false;

	for ( int i = 0; (i < BuffersCount) && !RequestStop; ++i ) {
		if ( fillAndPushBuffer( i ) )
			RequestStop = true;
	}

	return RequestStop;
}

void SoundStream::clearQueue() {
	// Get the number of buffers still in the queue
	ALint  NbQueued;
	ALCheck( alGetSourcei( Sound::mSource, AL_BUFFERS_QUEUED, &NbQueued ) );

	// Unqueue them all
	ALuint Buffer;
	for ( ALint i = 0; i < NbQueued; ++i )
		ALCheck( alSourceUnqueueBuffers( Sound::mSource, 1, &Buffer ) );
}

Sound::Status SoundStream::state() const {
	return getState();
}

}}
