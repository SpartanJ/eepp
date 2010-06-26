#ifndef EE_AUDIOCSOUNDBUFFER_H
#define EE_AUDIOCSOUNDBUFFER_H

#include "base.hpp"
#include "caudiodevice.hpp"
#include "caudioresource.hpp"
#include "csoundfile.hpp"

namespace EE { namespace Audio {

class EE_API cSoundBuffer : public cAudioResource {
	public :
		cSoundBuffer();
		~cSoundBuffer();
		
		/** Copy constructor */
		cSoundBuffer(const 	cSoundBuffer& Copy);
		
		/** Load the Sound Buffer from a file */
		bool LoadFromFile( const std::string& Filename );
		
		/** Load the Sound Buffer from a file inside a pack file*/
		bool LoadFromPack( cPack* Pack, const std::string& FilePackPath );
		
		/** Load the Sound Buffer from a file in memory */
		bool LoadFromMemory( const char* Data, std::size_t SizeInBytes );
		
		/** Load the Sound Buffer from an array of samples. Assumed format for samples is 16 bits signed integer */
		bool LoadFromSamples( const Int16* Samples, std::size_t SamplesCount, unsigned int ChannelsCount, unsigned int SampleRate );
		
		/** Save the Sound Buffer to a file */
		bool SaveToFile( const std::string& Filename ) const;
		
		/** @return The Sound Samples */
		const Int16* GetSamples() const;
		
		/** @return The Samples Count */
		std::size_t GetSamplesCount() const;
		
		/** Get the Sample Rate */
		unsigned int GetSampleRate() const;
		
		/** Return the number of Channels */
		unsigned int GetChannelsCount() const;
		
		/** Get the Sound Duration */
		eeFloat GetDuration() const;
		
		/** Assignment operator */
		cSoundBuffer& operator =(const 	cSoundBuffer& Other);
	private :
		friend class cSound;
		
		/** Update the internal buffer with the audio samples */
		bool Update(unsigned int ChannelsCount, unsigned int SampleRate);
			
		unsigned int myBuffer;   ///< OpenAL buffer identifier
		std::vector<Int16> mySamples;  ///< Samples buffer
		eeFloat myDuration; ///< Sound duration, in seconds
};

}}

#endif
