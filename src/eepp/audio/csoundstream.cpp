#include <eepp/audio/csoundstream.hpp>
#include <eepp/audio/caudiodevice.hpp>
#include <eepp/system/sys.hpp>
using namespace EE::System;

namespace EE { namespace Audio {

cSoundStream::cSoundStream() :
	mIsStreaming(false),
	mChannelsCount(0),
	mSampleRate (0),
	mFormat (0),
	mLoop(false),
	mSamplesProcessed(0)
{
}

cSoundStream::~cSoundStream() {
	Stop(); // Stop the sound if it was playing
}

void cSoundStream::Initialize(unsigned int ChannelsCount, unsigned int SampleRate) {
	mChannelsCount	= ChannelsCount;
	mSampleRate		= SampleRate;

	// Deduce the format from the number of channels
	mFormat = cAudioDevice::GetFormatFromChannelsCount(ChannelsCount);

	if ( mFormat == 0 ) { // Check if the format is valid
		mChannelsCount = 0;
		mSampleRate	= 0;
		cLog::instance()->Write( "Unsupported number of channels." );
	}
}

void cSoundStream::Play() {
	if ( mFormat == 0 ) { // Check if the sound parameters have been set
		cLog::instance()->Write( "Failed to play audio stream : sound parameters have not been initialized (call Initialize first)." );
		return;
	}

	if ( mIsStreaming ) { // If the sound is already playing (probably paused), just resume it
		cSound::Play();
		return;
	}

	OnSeek( 0 );

	mSamplesProcessed = 0;
	mIsStreaming = true; // Start updating the stream in a separate thread to avoid blocking the application
	Launch();
}

void cSoundStream::Pause() {
	ALCheck( alSourcePause( mSource ) );
}

void cSoundStream::Stop() {
	mIsStreaming = false; // Wait for the thread to terminate
	Wait();
}

unsigned int cSoundStream::GetChannelsCount() const {
	return mChannelsCount;
}

unsigned int cSoundStream::GetSampleRate() const {
	return mSampleRate;
}

EE_SOUND_STATE cSoundStream::GetState() const {
	EE_SOUND_STATE Status = cSound::GetState();

	if ( ( Status == SOUND_STOPPED ) && mIsStreaming ) // To compensate for the lag between Play() and alSourcePlay()
		Status = SOUND_PLAYING;

	return Status;
}

Uint32 cSoundStream::PlayingOffset() const {
	return static_cast<Uint32>( cSound::PlayingOffset() ) * 1000 + 1000 * mSamplesProcessed / mSampleRate / mChannelsCount;
}

void cSoundStream::PlayingOffset( const Uint32& timeOffset ) {
    // Stop the stream
    Stop();

    // Let the derived class update the current position
    OnSeek( timeOffset );

    // Restart streaming
	mSamplesProcessed = static_cast<Uint32>( timeOffset ) * mSampleRate * mChannelsCount / 1000;

    mIsStreaming = true;

    Launch();
}

void cSoundStream::Loop( const bool& Loop ) {
	mLoop = Loop;
}

bool cSoundStream::Loop() const {
	return mLoop;
}

void cSoundStream::Run() {
    ALCheck( alGenBuffers( BuffersCount, mBuffers ) );

	for ( int i = 0; i < BuffersCount; ++i )
		mEndBuffers[i] = false;

    // Fill the queue
    bool RequestStop = FillQueue();

	cSound::Play();

	while ( mIsStreaming ) {
		// The stream has been interrupted !
		if ( cSound::GetState() == SOUND_STOPPED ) {
			// User requested to stop : finish the streaming loop
			if ( !RequestStop ) {
				cSound::Play();
			} else {
				// Streaming is not completed : restart the sound
				mIsStreaming = false;
			}
		}

		// Get the number of buffers that have been processed (ie. ready for reuse)
		ALint NbProcessed = 0;
		ALCheck( alGetSourcei( cSound::mSource, AL_BUFFERS_PROCESSED, &NbProcessed ) );

		while ( NbProcessed-- ) {
			// Pop the first unused buffer from the queue
			ALuint Buffer;
			ALCheck( alSourceUnqueueBuffers( cSound::mSource, 1, &Buffer ) );

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
		if ( cSound::GetState() != SOUND_STOPPED )
			Sys::Sleep(10);
	}

	// Stop the playback
	cSound::Stop();

	// Unqueue any buffer left in the queue
	ClearQueue();

	// Delete the buffers
	ALCheck( alSourcei( cSound::mSource, AL_BUFFER, 0 ) );
	ALCheck( alDeleteBuffers( BuffersCount, mBuffers ) );
}

bool cSoundStream::FillAndPushBuffer( const unsigned int& Buffer ) {
	bool RequestStop = false;

	// Acquire audio data
	Chunk Data = {NULL, 0};

	if ( !OnGetData( Data ) ) {
		// Mark the buffer as the last one (so that we know when to reset the playing position)
		mEndBuffers[ Buffer ] = true;

		// Check if the stream must loop or stop
		if ( mLoop ) {
			// Return to the beginning of the stream source
			OnSeek( 0 );

			// If we previously had no data, try to fill the buffer once again
			if ( !Data.Samples || ( Data.NbSamples == 0 ) ) {
				return FillAndPushBuffer( Buffer );
			}
		} else {
			// Not looping: request stop
			RequestStop = true;
		}
	}

	// Create and fill the buffer, and push it to the queue
	if ( Data.Samples && Data.NbSamples ) {
		Uint32 buffer = mBuffers[ Buffer ];

		// Fill the buffer
		ALsizei Size = static_cast<ALsizei>( Data.NbSamples ) * sizeof(Int16);

		ALCheck( alBufferData( buffer, mFormat, Data.Samples, Size, mSampleRate ) );

		// Push it into the sound queue
		ALCheck( alSourceQueueBuffers( cSound::mSource, 1, &buffer ) );
	}

	return RequestStop;
}

bool cSoundStream::FillQueue() {
    // Fill and enqueue all the available buffers
    bool RequestStop = false;

    for ( int i = 0; (i < BuffersCount) && !RequestStop; ++i ) {
		if ( FillAndPushBuffer( i ) )
            RequestStop = true;
    }

    return RequestStop;
}

void cSoundStream::ClearQueue() {
    // Get the number of buffers still in the queue
    ALint  NbQueued;
    ALCheck( alGetSourcei( cSound::mSource, AL_BUFFERS_QUEUED, &NbQueued ) );

    // Unqueue them all
    ALuint Buffer;
    for ( ALint i = 0; i < NbQueued; ++i )
        ALCheck( alSourceUnqueueBuffers( cSound::mSource, 1, &Buffer ) );
}

EE_SOUND_STATE cSoundStream::State() const {
	return GetState();
}

}}
