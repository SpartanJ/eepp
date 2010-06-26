#ifndef EE_AUDIOCSOUNDSTREAM_H
#define EE_AUDIOCSOUNDSTREAM_H

#include "base.hpp"
#include "caudiodevice.hpp"
#include "csound.hpp"

namespace EE { namespace Audio {

class EE_API cSoundStream : private cThread, private cSound {
	public:
		using cSound::Pause;
		using cSound::Pitch;
		using cSound::Volume;
		using cSound::Position;
		using cSound::MinDistance;
		using cSound::Attenuation;
		using cSound::SetRelativeToListener;
		using cSound::IsRelativeToListener;
		
		struct Chunk {
			const Int16* Samples;   ///< Pointer to the audio samples
			std::size_t  NbSamples; ///< Number of samples pointed by Samples
		};
		
		virtual ~cSoundStream();
		
		void Play();
		void Stop();
		
		unsigned int GetChannelsCount() const;
		unsigned int GetSampleRate() const;
		
		EE_SOUND_STATE GetState() const;
		EE_SOUND_STATE State() const { return GetState(); };
		
		eeFloat GetPlayingOffset() const;
		
		/** Set the stream loop state. This parameter is disabled by default
		* @param Loop True to play in loop, false to play once
		*/
		void Loop( const bool& Loop);

	
		/** Tell whether or not the stream is looping
		* @return True if the music is looping, false otherwise
		*/
		bool Loop() const;
	protected:
		cSoundStream();
		
		void Initialize(unsigned int ChannelsCount, unsigned int SampleRate);
	private :
		virtual void Run();
		
		virtual bool OnStart();
		virtual bool OnGetData(Chunk& Data) = 0;
		
		/** Fill a new buffer with audio data, and push it to the playing queue
		* @param Buffer Buffer to fill
		* @return True if the derived class wish to continue playback
		*/
		bool FillAndPushBuffer( const unsigned int& Buffer );
    	
    	/** Fill the buffers queue with all available buffers
    	* @return True if the derived class has requested to stop
    	*/
    	bool FillQueue();
    	
		void ClearQueue();
		
		enum {BuffersCount = 3};
		
		bool		  mIsStreaming;		   		///< Streaming state (true = playing, false = stopped)
		unsigned int  mBuffers[BuffersCount]; 	///< Sound buffers used to store temporary audio data
		unsigned int  mChannelsCount;		 	///< Number of channels (1 = mono, 2 = stereo, ...)
		unsigned int  mSampleRate;				///< Frequency (samples / second)
		unsigned long mFormat;					///< Format of the internal sound buffers
		bool          mLoop;                  	///< Loop flag (true to loop, false to play once)
		unsigned int  mSamplesProcessed;      	///< Number of buffers processed since beginning of the stream
};

}}

#endif
