#ifndef EE_AUDIOCSOUNDBUFFER_H
#define EE_AUDIOCSOUNDBUFFER_H

#include <eepp/audio/base.hpp>
#include <set>

namespace EE { namespace Audio {

class Sound;

/** @brief Storage for audio samples defining a sound */
class EE_API SoundBuffer {
	public :
		SoundBuffer();

		~SoundBuffer();

		/** Copy constructor */
		SoundBuffer(const 	SoundBuffer& Copy);

		/** Load the Sound Buffer from a file */
		bool LoadFromFile( const std::string& Filename );

		/** Load the Sound Buffer from a file inside a pack file*/
		bool LoadFromPack( Pack* Pack, const std::string& FilePackPath );

		/** Load the Sound Buffer from a file in memory */
		bool LoadFromMemory( const char* Data, std::size_t SizeInBytes );

		/** Load the Sound Buffer from an array of samples. Assumed format for samples is 16 bits signed integer */
		bool LoadFromSamples( const Int16* Samples, std::size_t SamplesCount, unsigned int ChannelCount, unsigned int SampleRate );

		/** Save the Sound Buffer to a file */
		bool SaveToFile( const std::string& Filename ) const;

		/** @return The Sound Samples */
		const Int16* GetSamples() const;

		/** @return The Samples Count */
		std::size_t GetSamplesCount() const;

		/** Get the Sample Rate */
		unsigned int GetSampleRate() const;

		/** Return the number of Channels */
		unsigned int GetChannelCount() const;

		/** Get the Sound Duration */
		Time GetDuration() const;

		/** Assignment operator */
		SoundBuffer& operator =(const 	SoundBuffer& Other);
	private :
		friend class Sound;

		unsigned int		mBuffer;   ///< OpenAL buffer identifier
		std::vector<Int16>	mSamples;  ///< Samples buffer
		Time				mDuration; ///< Sound duration, in seconds

		typedef std::set<Sound*> SoundList;
		mutable SoundList	mSounds;

		/** Update the internal buffer with the audio samples */
		bool Update( unsigned int ChannelCount, unsigned int SampleRate );

		void AttachSound( Sound* sound ) const;

		void DetachSound( Sound* sound ) const;

};

}}

#endif
