#ifndef EE_AUDIOCSOUNDBUFFER_H
#define EE_AUDIOCSOUNDBUFFER_H

#include <set>
#include <eepp/system/time.hpp>
#include <eepp/system/pack.hpp>
using namespace EE::System;


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
		bool loadFromFile( const std::string& Filename );

		/** Load the Sound Buffer from a file inside a pack file*/
		bool loadFromPack( Pack* Pack, const std::string& FilePackPath );

		/** Load the Sound Buffer from a file in memory */
		bool loadFromMemory( const char* Data, std::size_t SizeInBytes );

		/** Load the Sound Buffer from an array of samples. Assumed format for samples is 16 bits signed integer */
		bool loadFromSamples( const Int16* Samples, std::size_t SamplesCount, unsigned int ChannelCount, unsigned int SampleRate );

		/** Save the Sound Buffer to a file */
		bool saveToFile( const std::string& Filename ) const;

		/** @return The Sound Samples */
		const Int16* getSamples() const;

		/** @return The Samples Count */
		std::size_t getSamplesCount() const;

		/** Get the Sample Rate */
		unsigned int getSampleRate() const;

		/** Return the number of Channels */
		unsigned int getChannelCount() const;

		/** Get the Sound Duration */
		Time getDuration() const;

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
		bool update( unsigned int ChannelCount, unsigned int SampleRate );

		void attachSound( Sound* sound ) const;

		void detachSound( Sound* sound ) const;

};

}}

#endif
