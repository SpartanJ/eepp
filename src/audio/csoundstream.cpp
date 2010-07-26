#include "csoundstream.hpp"

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
	mChannelsCount = ChannelsCount;
	mSampleRate	= SampleRate;

	// Deduce the format from the number of channels
	mFormat = cAudioDevice::instance()->GetFormatFromChannelsCount(ChannelsCount);

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

	if ( OnStart() ) { // Notify the derived class
		mIsStreaming = true; // Start updating the stream in a separate thread to avoid blocking the application
		Launch();
	}
}

void cSoundStream::Stop() {
	mIsStreaming = false; // Wait for the thread to terminate
	mSamplesProcessed = 0;
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

eeFloat cSoundStream::GetPlayingOffset() const {
    return cSound::PlayingOffset() + static_cast<eeFloat>(mSamplesProcessed) / mSampleRate / mChannelsCount;
}

void cSoundStream::SetPlayingOffset( float timeOffset ) {
    // Stop the stream
    Stop();

    // Let the derived class update the current position
    OnSeek( timeOffset );

    // Restart streaming
    mSamplesProcessed = static_cast<unsigned int>( timeOffset * mSampleRate * mChannelsCount );

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
	if ( !cAudioDevice::instance()->isCreated() )
		return;

    ALCheck( alGenBuffers( BuffersCount, mBuffers ) );
	unsigned int EndBuffer = 0xFFFF;

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
		ALint NbProcessed;
		ALCheck( alGetSourcei( cSound::mSource, AL_BUFFERS_PROCESSED, &NbProcessed ) );

		while ( NbProcessed-- ) {
			// Pop the first unused buffer from the queue
			ALuint Buffer;
			ALCheck( alSourceUnqueueBuffers( cSound::mSource, 1, &Buffer ) );

			// Retrieve its size and add it to the samples count
			if ( Buffer == EndBuffer ) {
				// This was the last buffer: reset the sample count
				mSamplesProcessed = 0;
				EndBuffer = 0xFFFF;
			} else {
				ALint Size;
				ALCheck( alGetBufferi( Buffer, AL_SIZE, &Size ) );
				mSamplesProcessed += Size / sizeof(Int16);
			}

			// Fill it and push it back into the playing queue
			if ( !RequestStop ) {
				if ( FillAndPushBuffer( Buffer ) ) {
					// User requested to stop: check if we must loop or really stop
					if ( mLoop && OnStart() ) {
						// Looping: mark the current buffer as the last one
						// (to know when to reset the sample count)
						EndBuffer = Buffer;
					} else {
						// Not looping or restart failed: request stop
						RequestStop = true;
					}
				}
			}
		}

		// Leave some time for the other threads if the stream is still playing
		if ( cSound::GetState() != SOUND_STOPPED )
			eeSleep(100);
	}

	// Stop the playback
	cSound::Stop();

	// Unqueue any buffer left in the queue
	ClearQueue();

	// Delete the buffers
	ALCheck( alSourcei( cSound::mSource, AL_BUFFER, 0 ) );
	ALCheck( alDeleteBuffers( BuffersCount, mBuffers ) );
}

bool cSoundStream::FillAndPushBuffer( const unsigned int& Buffer) {
	bool RequestStop = false;

	// Acquire audio data
	Chunk Data = {NULL, 0};

	if ( !OnGetData( Data ) )
		RequestStop = true;

	// Create and fill the buffer, and push it to the queue
	if ( Data.Samples && Data.NbSamples ) {
		// Fill the buffer
		ALsizei Size = static_cast<ALsizei>( Data.NbSamples ) * sizeof(Int16);

		ALCheck( alBufferData( Buffer, mFormat, Data.Samples, Size, mSampleRate ) );

		// Push it into the sound queue
		ALCheck( alSourceQueueBuffers( cSound::mSource, 1, &Buffer ) );
	}

	return RequestStop;
}

bool cSoundStream::FillQueue() {
    // Fill and enqueue all the available buffers
    bool RequestStop = false;

    for ( int i = 0; (i < BuffersCount) && !RequestStop; ++i ) {
        if ( FillAndPushBuffer( mBuffers[i] ) )
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

bool cSoundStream::OnStart() { // Called when the sound restarts
	return true; // Does nothing by default
}

}}
