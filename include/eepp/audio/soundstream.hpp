#ifndef EE_AUDIOCSOUNDSTREAM_H
#define EE_AUDIOCSOUNDSTREAM_H

#include <eepp/audio/sound.hpp>
#include <eepp/system/thread.hpp>
using namespace EE::System;

namespace EE { namespace Audio {

/** @brief Abstract base class for streamed audio sources */
class EE_API SoundStream : private Thread, public Sound {
	public:
		using Sound::setPitch;
		using Sound::getPitch;
		using Sound::setVolume;
		using Sound::getVolume;
		using Sound::setPosition;
		using Sound::getPosition;
		using Sound::setMinDistance;
		using Sound::getMinDistance;
		using Sound::setAttenuation;
		using Sound::getAttenuation;
		using Sound::setRelativeToListener;
		using Sound::isRelativeToListener;

		/** @brief Structure defining a chunk of audio data to stream */
		struct Chunk {
			const Int16* Samples;   ///< Pointer to the audio samples
			std::size_t  SamplesCount; ///< Number of samples pointed by Samples
		};

		virtual ~SoundStream();

		/** \brief Start or resume playing the audio stream
		**	This function starts the stream if it was stopped, resumes
		**	it if it was paused, and restarts it from beginning if it
		**	was it already playing.
		**	This function uses its own thread so that it doesn't block
		**	the rest of the program while the stream is played.
		**	@see Pause, Stop */
		void play();

		/** @brief Pause the audio stream
		**	This function pauses the stream if it was playing,
		**	otherwise (stream already paused or stopped) it has no effect.
		**	@see Play, Stop */
		void pause();

		/** @brief Stop playing the audio stream
		**	This function stops the stream if it was playing or paused,
		**	and does nothing if it was already stopped.
		**	It also resets the playing position (unlike pause()).
		**	@see Play, Pause */
		void stop();

		/** @brief Return the number of channels of the stream
		**	1 channel means a mono sound, 2 means stereo, etc.
		**	@return Number of channels */
		unsigned int getChannelCount() const;

		/** @brief Get the stream sample rate of the stream
		**	The sample rate is the number of audio samples played per
		**	second. The higher, the better the quality.
		**	@return Sample rate, in number of samples per second */
		unsigned int getSampleRate() const;

		/** @brief Get the current status of the stream (stopped, paused, playing)
		**	@return Current status */
		Status 	getState() const;

		/**	@brief Get the current playing position of the stream
		**	@return Current playing position, from the beginning of the stream. */
		Time getPlayingOffset() const;

		/**	@brief Change the current playing position of the stream
		**	The playing position can be changed when the stream is either paused or playing.
		**	@param timeOffset New playing position, from the beginning of the stream. */
		void setPlayingOffset( const Time &timeOffset );

		/** Set the stream loop state. This parameter is disabled by default
		* @param Loop True to play in loop, false to play once
		*/
		void setLoop( const bool& getLoop);

		/** Tell whether or not the stream is looping
		* @return True if the music is looping, false otherwise
		*/
		bool getLoop() const;
	protected:
		SoundStream();

		void initialize(unsigned int ChannelCount, unsigned int SampleRate);
	private :
		virtual void run();

		virtual bool onGetData( Chunk& Data ) = 0;

		virtual void onSeek( Time timeOffset ) = 0;

		/** Fill a new buffer with audio data, and push it to the playing queue
		* @param Buffer Buffer to fill
		* @return True if the derived class wish to continue playback
		*/
		bool fillAndPushBuffer( const unsigned int& buffer );

		/** Fill the buffers queue with all available buffers
		* @return True if the derived class has requested to stop
		*/
		bool fillQueue();

		void clearQueue();

		enum {
			BuffersCount = 3
		};

		bool			mIsStreaming;				///< Streaming state (true = playing, false = stopped)
		unsigned int	mBuffers[BuffersCount];		///< Sound buffers used to store temporary audio data
		unsigned int	mChannelCount;				///< Number of channels (1 = mono, 2 = stereo, ...)
		unsigned int	mSampleRate;				///< Frequency (samples / second)
		unsigned long	mFormat;					///< Format of the internal sound buffers
		bool			mLoop;						///< Loop flag (true to loop, false to play once)
		Uint32			mSamplesProcessed;			///< Number of buffers processed since beginning of the stream
		bool			mEndBuffers[BuffersCount];	///< Each buffer is marked as "end buffer" or not, for proper duration calculation
};

}}

#endif
