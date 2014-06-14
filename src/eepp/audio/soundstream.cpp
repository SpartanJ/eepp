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
	Stop(); // Stop the sound if it was playing
}

void SoundStream::Initialize(unsigned int ChannelCount, unsigned int SampleRate) {
	mChannelCount	= ChannelCount;
	mSampleRate		= SampleRate;

	// Deduce the format from the number of channels
	mFormat = AudioDevice::GetFormatFromChannelCount(ChannelCount);

	if ( mFormat == 0 ) { // Check if the format is valid
		mChannelCount = 0;
		mSampleRate	= 0;
		eePRINTL( "Unsupported number of channels." );
	}
}

void SoundStream::Play() {
	if ( mFormat == 0 ) { // Check if the sound parameters have been set
		eePRINTL( "Failed to play audio stream : sound parameters have not been initialized (call Initialize first)." );
		return;
	}

	if ( mIsStreaming ) { // If the sound is already playing (probably paused), just resume it
		Sound::Play();
		return;
	}

	OnSeek( cTime::Zero );

	mSamplesProcessed = 0;
	mIsStreaming = true; // Start updating the stream in a separate thread to avoid blocking the application
	Launch();
}

void SoundStream::Pause() {
	ALCheck( alSourcePause( mSource ) );
}

void SoundStream::Stop() {
	mIsStreaming = false; // Wait for the thread to terminate
	Wait();
}

unsigned int SoundStream::GetChannelCount() const {
	return mChannelCount;
}

unsigned int SoundStream::GetSampleRate() const {
	return mSampleRate;
}

Sound::Status SoundStream::GetState() const {
	Status status = Sound::GetState();

	if ( ( status == Sound::Stopped ) && mIsStreaming ) // To compensate for the lag between Play() and alSourcePlay()
		status = Sound::Playing;

	return status;
}

cTime SoundStream::PlayingOffset() const {
	if ( mSampleRate && mChannelCount ) {
		float secs = 0.f;

		ALCheck( alGetSourcef( mSource, AL_SEC_OFFSET, &secs ) );

		return Seconds( secs + (float)mSamplesProcessed / (float)mSampleRate / (float)mChannelCount );
	}

	return cTime::Zero;
}

void SoundStream::PlayingOffset( const cTime &timeOffset ) {
	Status oldStatus = State();

    // Stop the stream
    Stop();

    // Let the derived class update the current position
    OnSeek( timeOffset );

    // Restart streaming
	mSamplesProcessed = static_cast<Uint32>( timeOffset.AsSeconds() ) * mSampleRate * mChannelCount;

    mIsStreaming = true;

    Launch();

	// Recover old status
	if ( oldStatus == Stopped ) {
		Stop();
	} else if ( oldStatus == Paused ) {
		Pause();
	}
}

void SoundStream::Loop( const bool& Loop ) {
	mLoop = Loop;
}

bool SoundStream::Loop() const {
	return mLoop;
}

void SoundStream::Run() {
    ALCheck( alGenBuffers( BuffersCount, mBuffers ) );

	for ( int i = 0; i < BuffersCount; ++i )
		mEndBuffers[i] = false;

    // Fill the queue
    bool RequestStop = FillQueue();

	Sound::Play();

	while ( mIsStreaming ) {
		// The stream has been interrupted !
		if ( Sound::GetState() == Sound::Stopped ) {
			// User requested to stop : finish the streaming loop
			if ( !RequestStop ) {
				Sound::Play();
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
				if ( FillAndPushBuffer( bufferNum ) )
					RequestStop = true;
			}
		}

		// Leave some time for the other threads if the stream is still playing
		if ( Sound::GetState() != Sound::Stopped )
			Sys::Sleep(10);
	}

	// Stop the playback
	Sound::Stop();

	// Unqueue any buffer left in the queue
	ClearQueue();

	// Delete the buffers
	ALCheck( alSourcei( Sound::mSource, AL_BUFFER, 0 ) );
	ALCheck( alDeleteBuffers( BuffersCount, mBuffers ) );
}

bool SoundStream::FillAndPushBuffer( const unsigned int& Buffer ) {
	bool RequestStop = false;

	// Acquire audio data
	Chunk Data = {NULL, 0};

	if ( !OnGetData( Data ) ) {
		// Mark the buffer as the last one (so that we know when to reset the playing position)
		mEndBuffers[ Buffer ] = true;

		// Check if the stream must loop or stop
		if ( mLoop ) {
			// Return to the beginning of the stream source
			OnSeek( cTime::Zero );

			// If we previously had no data, try to fill the buffer once again
			if ( !Data.Samples || ( Data.SamplesCount == 0 ) ) {
				return FillAndPushBuffer( Buffer );
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

bool SoundStream::FillQueue() {
    // Fill and enqueue all the available buffers
    bool RequestStop = false;

    for ( int i = 0; (i < BuffersCount) && !RequestStop; ++i ) {
		if ( FillAndPushBuffer( i ) )
            RequestStop = true;
    }

    return RequestStop;
}

void SoundStream::ClearQueue() {
    // Get the number of buffers still in the queue
    ALint  NbQueued;
    ALCheck( alGetSourcei( Sound::mSource, AL_BUFFERS_QUEUED, &NbQueued ) );

    // Unqueue them all
    ALuint Buffer;
    for ( ALint i = 0; i < NbQueued; ++i )
        ALCheck( alSourceUnqueueBuffers( Sound::mSource, 1, &Buffer ) );
}

Sound::Status SoundStream::State() const {
	return GetState();
}

}}
