#ifndef EE_AUDIOCSOUNDBUFFER_H
#define EE_AUDIOCSOUNDBUFFER_H

#include <eepp/audio/base.hpp>
#include <set>

namespace EE { namespace Audio {

class cSound;

/** @brief Storage for audio samples defining a sound */
class EE_API cSoundBuffer {
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

		/** Get the Sound Duration in seconds */
		float GetDuration() const;

		/** Assignment operator */
		cSoundBuffer& operator =(const 	cSoundBuffer& Other);
	private :
		friend class cSound;

		unsigned int		mBuffer;   ///< OpenAL buffer identifier
		std::vector<Int16>	mSamples;  ///< Samples buffer
		float				mDuration; ///< Sound duration, in seconds

		typedef std::set<cSound*> SoundList;
		mutable SoundList	mSounds;

		/** Update the internal buffer with the audio samples */
		bool Update( unsigned int ChannelCount, unsigned int SampleRate );

		void AttachSound( cSound* sound ) const;

		void DetachSound( cSound* sound ) const;

};

}}

#endif
