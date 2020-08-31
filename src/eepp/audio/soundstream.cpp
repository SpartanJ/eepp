#include <eepp/audio/alcheck.hpp>
#include <eepp/audio/audiodevice.hpp>
#include <eepp/audio/soundstream.hpp>
#include <eepp/core/debug.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/sys.hpp>

#ifdef _MSC_VER
#pragma warning( disable : 4355 ) // 'this' used in base member initializer list
#endif

namespace EE { namespace Audio {

SoundStream::SoundStream() :
	mThread( &SoundStream::streamData, this ),
	mThreadMutex(),
	mThreadStartState( Stopped ),
	mIsStreaming( false ),
	mBuffers(),
	mChannelCount( 0 ),
	mSampleRate( 0 ),
	mFormat( 0 ),
	mLoop( false ),
	mSamplesProcessed( 0 ),
	mBufferSeeks() {}

SoundStream::~SoundStream() {
	// Stop the sound if it was playing

	// Request the thread to terminate
	{
		Lock lock( mThreadMutex );
		mIsStreaming = false;
	}

	// Wait for the thread to terminate
	mThread.wait();
}

void SoundStream::initialize( unsigned int channelCount, unsigned int sampleRate ) {
	mChannelCount = channelCount;
	mSampleRate = sampleRate;
	mSamplesProcessed = 0;
	mIsStreaming = false;

	// Deduce the format from the number of channels
	mFormat = Private::AudioDevice::getFormatFromChannelCount( channelCount );

	// Check if the format is valid
	if ( mFormat == 0 ) {
		mChannelCount = 0;
		mSampleRate = 0;
		Log::error( "SoundStream: Unsupported number of channels (%d)", mChannelCount );
	}
}

void SoundStream::play() {
	// Check if the sound parameters have been set
	if ( mFormat == 0 ) {
		Log::error( "Failed to play audio stream: sound parameters have not been initialized (call "
					"initialize() first)" );
		return;
	}

	bool isStreaming = false;
	Status threadStartState = Stopped;

	{
		Lock lock( mThreadMutex );

		isStreaming = mIsStreaming;
		threadStartState = mThreadStartState;
	}

	if ( isStreaming && ( threadStartState == Paused ) ) {
		// If the sound is paused, resume it
		Lock lock( mThreadMutex );
		mThreadStartState = Playing;
		alCheck( alSourcePlay( mSource ) );
		return;
	} else if ( isStreaming && ( threadStartState == Playing ) ) {
		// If the sound is playing, stop it and continue as if it was stopped
		stop();
	}

	// Start updating the stream in a separate thread to avoid blocking the application
	mIsStreaming = true;
	mThreadStartState = Playing;
	mThread.launch();
}

void SoundStream::pause() {
	// Handle pause() being called before the thread has started
	{
		Lock lock( mThreadMutex );

		if ( !mIsStreaming )
			return;

		mThreadStartState = Paused;
	}

	alCheck( alSourcePause( mSource ) );
}

void SoundStream::stop() {
	// Request the thread to terminate
	{
		Lock lock( mThreadMutex );
		mIsStreaming = false;
	}

	// Wait for the thread to terminate
	mThread.wait();

	// Move to the beginning
	onSeek( Time::Zero );
}

unsigned int SoundStream::getChannelCount() const {
	return mChannelCount;
}

unsigned int SoundStream::getSampleRate() const {
	return mSampleRate;
}

SoundStream::Status SoundStream::getStatus() const {
	Status status = SoundSource::getStatus();

	// To compensate for the lag between play() and alSourceplay()
	if ( status == Stopped ) {
		Lock lock( mThreadMutex );

		if ( mIsStreaming )
			status = mThreadStartState;
	}

	return status;
}

void SoundStream::setPlayingOffset( Time timeOffset ) {
	// Get old playing status
	Status oldStatus = getStatus();

	// Stop the stream
	stop();

	// Let the derived class update the current position
	onSeek( timeOffset );

	// Restart streaming
	mSamplesProcessed = static_cast<Uint64>( timeOffset.asSeconds() * mSampleRate * mChannelCount );

	if ( oldStatus == Stopped )
		return;

	mIsStreaming = true;
	mThreadStartState = oldStatus;
	mThread.launch();
}

Time SoundStream::getPlayingOffset() const {
	if ( mSampleRate && mChannelCount ) {
		ALfloat secs = 0.f;
		alCheck( alGetSourcef( mSource, AL_SEC_OFFSET, &secs ) );

		return Seconds( secs +
						static_cast<float>( mSamplesProcessed ) / mSampleRate / mChannelCount );
	} else {
		return Time::Zero;
	}
}

void SoundStream::setLoop( bool loop ) {
	mLoop = loop;
}

bool SoundStream::getLoop() const {
	return mLoop;
}

Int64 SoundStream::onLoop() {
	onSeek( Time::Zero );
	return 0;
}

void SoundStream::streamData() {
	bool requestStop = false;

	{
		Lock lock( mThreadMutex );

		// Check if the thread was launched Stopped
		if ( mThreadStartState == Stopped ) {
			mIsStreaming = false;
			return;
		}
	}

	// Create the buffers
	alCheck( alGenBuffers( BufferCount, mBuffers ) );
	for ( int i = 0; i < BufferCount; ++i )
		mBufferSeeks[i] = NoLoop;

	// Fill the queue
	requestStop = fillQueue();

	// Play the sound
	alCheck( alSourcePlay( mSource ) );

	{
		Lock lock( mThreadMutex );

		// Check if the thread was launched Paused
		if ( mThreadStartState == Paused )
			alCheck( alSourcePause( mSource ) );
	}

	for ( ;; ) {
		{
			Lock lock( mThreadMutex );
			if ( !mIsStreaming )
				break;
		}

		// The stream has been interrupted!
		if ( SoundSource::getStatus() == Stopped ) {
			if ( !requestStop ) {
				// Just continue
				alCheck( alSourcePlay( mSource ) );
			} else {
				// End streaming
				Lock lock( mThreadMutex );
				mIsStreaming = false;
			}
		}

		// Get the number of buffers that have been processed (i.e. ready for reuse)
		ALint nbProcessed = 0;
		alCheck( alGetSourcei( mSource, AL_BUFFERS_PROCESSED, &nbProcessed ) );

		while ( nbProcessed-- ) {
			// Pop the first unused buffer from the queue
			ALuint buffer;
			alCheck( alSourceUnqueueBuffers( mSource, 1, &buffer ) );

			// Find its number
			unsigned int bufferNum = 0;
			for ( int i = 0; i < BufferCount; ++i )
				if ( mBuffers[i] == buffer ) {
					bufferNum = i;
					break;
				}

			// Retrieve its size and add it to the samples count
			if ( mBufferSeeks[bufferNum] != NoLoop ) {
				// This was the last buffer before EOF or Loop End: reset the sample count
				mSamplesProcessed = mBufferSeeks[bufferNum];
				mBufferSeeks[bufferNum] = NoLoop;
			} else {
				ALint size, bits;
				alCheck( alGetBufferi( buffer, AL_SIZE, &size ) );
				alCheck( alGetBufferi( buffer, AL_BITS, &bits ) );

				// Bits can be 0 if the format or parameters are corrupt, avoid division by zero
				if ( bits == 0 ) {
					Log::warning(
						"SoundStream: Bits in sound stream are 0: make sure that the "
						"audio format is not corrupt and initialize() has been called correctly." );

					// Abort streaming (exit main loop)
					Lock lock( mThreadMutex );
					mIsStreaming = false;
					requestStop = true;
					break;
				} else {
					mSamplesProcessed += size / ( bits / 8 );
				}
			}

			// Fill it and push it back into the playing queue
			if ( !requestStop ) {
				if ( fillAndPushBuffer( bufferNum ) )
					requestStop = true;
			}
		}

		// Leave some time for the other threads if the stream is still playing
		if ( SoundSource::getStatus() != Stopped )
			Sys::sleep( Milliseconds( 10 ) );
	}

	// Stop the playback
	alCheck( alSourceStop( mSource ) );

	// Dequeue any buffer left in the queue
	clearQueue();

	// Reset the playing position
	mSamplesProcessed = 0;

	// Delete the buffers
	alCheck( alSourcei( mSource, AL_BUFFER, 0 ) );
	alCheck( alDeleteBuffers( BufferCount, mBuffers ) );
}

bool SoundStream::fillAndPushBuffer( unsigned int bufferNum, bool immediateLoop ) {
	bool requestStop = false;

	// Acquire audio data, also address EOF and error cases if they occur
	Chunk data = {NULL, 0};
	for ( Uint32 retryCount = 0; !onGetData( data ) && ( retryCount < BufferRetries );
		  ++retryCount ) {
		// Check if the stream must loop or stop
		if ( !mLoop ) {
			// Not looping: Mark this buffer as ending with 0 and request stop
			if ( data.samples != NULL && data.sampleCount != 0 )
				mBufferSeeks[bufferNum] = 0;
			requestStop = true;
			break;
		}

		// Return to the beginning or loop-start of the stream source using onLoop(), and store the
		// result in the buffer seek array This marks the buffer as the "last" one (so that we know
		// where to reset the playing position)
		mBufferSeeks[bufferNum] = onLoop();

		// If we got data, break and process it, else try to fill the buffer once again
		if ( data.samples != NULL && data.sampleCount != 0 )
			break;

		// If immediateLoop is specified, we have to immediately adjust the sample count
		if ( immediateLoop && ( mBufferSeeks[bufferNum] != NoLoop ) ) {
			// We just tried to begin preloading at EOF or Loop End: reset the sample count
			mSamplesProcessed = mBufferSeeks[bufferNum];
			mBufferSeeks[bufferNum] = NoLoop;
		}

		// We're a looping sound that got no data, so we retry onGetData()
	}

	// Fill the buffer if some data was returned
	if ( data.samples && data.sampleCount ) {
		unsigned int buffer = mBuffers[bufferNum];

		// Fill the buffer
		ALsizei size = static_cast<ALsizei>( data.sampleCount ) * sizeof( Int16 );
		alCheck( alBufferData( buffer, mFormat, data.samples, size, mSampleRate ) );

		// Push it into the sound queue
		alCheck( alSourceQueueBuffers( mSource, 1, &buffer ) );
	} else {
		// If we get here, we most likely ran out of retries
		requestStop = true;
	}

	return requestStop;
}

bool SoundStream::fillQueue() {
	// Fill and enqueue all the available buffers
	bool requestStop = false;
	for ( int i = 0; ( i < BufferCount ) && !requestStop; ++i ) {
		// Since no sound has been loaded yet, we can't schedule loop seeks preemptively,
		// So if we start on EOF or Loop End, we let fillAndPushBuffer() adjust the sample count
		if ( fillAndPushBuffer( i, ( i == 0 ) ) )
			requestStop = true;
	}

	return requestStop;
}

void SoundStream::clearQueue() {
	// Get the number of buffers still in the queue
	ALint nbQueued;
	alCheck( alGetSourcei( mSource, AL_BUFFERS_QUEUED, &nbQueued ) );

	// Dequeue them all
	ALuint buffer;
	for ( ALint i = 0; i < nbQueued; ++i )
		alCheck( alSourceUnqueueBuffers( mSource, 1, &buffer ) );
}

}} // namespace EE::Audio
