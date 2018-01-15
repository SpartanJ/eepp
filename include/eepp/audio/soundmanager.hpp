#ifndef EE_AUDIOTSOUNDMANAGER_H
#define EE_AUDIOTSOUNDMANAGER_H

#include <eepp/audio/base.hpp>
#include <eepp/audio/sound.hpp>
#include <eepp/audio/soundbuffer.hpp>

namespace EE { namespace Audio {

/** @brief A basic template to hold sounds and the respective buffer. */
template <typename T>
class tSoundManager {
	public:
		/** @brief Load the sound from file
		**	@param id The sound Id
		**	@param filepath The sound path
		*/
		bool loadFromFile( const T& id, const std::string& filepath );

		/** @brief Load the sound from memory
		**	@param id The sound id
		**	@param Data The pointer to the data
		**	@param SizeInBytes The size of the file to load */
		bool loadFromMemory( const T& id, const char* Data, std::size_t SizeInBytes );

		/**	@brief Load the sound from an array of samples.
		**	@param id The sound id
		**	@param Samples Pointer to the array of samples in memory
		**	@param SamplesCount Number of samples in the array
		**	@param ChannelCount Number of channels (1 = mono, 2 = stereo, ...)
		**	@param SampleRate Sample rate (number of samples to play per second) */
		bool loadFromSamples( const T& id, const Int16* Samples, std::size_t SamplesCount, unsigned int ChannelCount, unsigned int SampleRate );

		/**	@brief Load the sound from a Pack file
		**	@param id The id to use to identificate the sound
		**	@param Pack The Pack file
		**	@param FilePackPath The pack file path */
		bool loadFromPack( const T& id, Pack* Pack, const std::string& FilePackPath );

		/** @return The sound buffer of the sound id */
		SoundBuffer& getBuffer( const T& id );

		/** Play the sound. This method will open a new channel if the channel seted for the sound is already playing.
		**	@param id The sound id to play */
		void play( const T& id );

		/** Remove a sound from the sound manager.
		**	@param id The sound id to remove */
		bool remove( const T& id );

		/** @return The sound id if exists */
		Sound& operator[] ( const T& id );

		/** @brief Search for the sound id, and return a sound that is not playing, if all the sounds are playing, creates a new sound.
		**	@return The sound */
		Sound& getFreeSound( const T& id );

		~tSoundManager();
	private:
		typedef struct sSound {
			SoundBuffer Buf;
			std::vector<Sound> Snd;
		} sSound;
		std::map<T, sSound> tSounds;
};

template <typename T>
bool tSoundManager<T>::loadFromPack( const T& id, Pack* Pack, const std::string& FilePackPath ) {
	if ( tSounds.find( id ) == tSounds.end() ) { // if id doesn't exists
		sSound * tSound = &tSounds[id];

		if ( tSound->Buf.loadFromPack( Pack, FilePackPath ) ) {
			tSound->Snd.push_back( Sound( tSound->Buf ) );
			return true;
		}
	}

	return false;
}

template <typename T>
bool tSoundManager<T>::loadFromFile( const T& id, const std::string& filepath ) {
	if ( tSounds.find( id ) == tSounds.end() ) { // if id doesn't exists
		sSound * tSound = &tSounds[id];

		if ( tSound->Buf.loadFromFile( filepath ) ) {
			tSound->Snd.push_back( Sound( tSound->Buf ) );
			return true;
		}
	}

	return false;
}

template <typename T>
bool tSoundManager<T>::loadFromMemory( const T& id, const char* Data, std::size_t SizeInBytes ) {
	if ( tSounds.find( id ) == tSounds.end() ) { // if id doesn't exists
		sSound * tSound = &tSounds[id];

		if ( tSound->Buf.loadFromMemory( Data, SizeInBytes ) ) {
			tSound->Snd.push_back( Sound( tSound->Buf ) );
			return true;
		}
	}

	return false;
}

template <typename T>
bool tSoundManager<T>::loadFromSamples( const T& id, const Int16* Samples, std::size_t SamplesCount, unsigned int ChannelCount, unsigned int SampleRate ) {
	if ( tSounds.find( id ) == tSounds.end() ) { // if id doesn't exists
		sSound * tSound = &tSounds[id];

		if ( tSound->Buf.loadFromSamples( Samples, SamplesCount, ChannelCount, SampleRate ) ) {
			tSound->Snd.push_back( Sound( tSound->Buf ) );
			return true;
		}
	}

	return false;
}

template <typename T>
SoundBuffer& tSoundManager<T>::getBuffer( const T& id ) {
	if ( tSounds.find( id ) != tSounds.end() )
		return tSounds[id].Buf;

	static SoundBuffer soundBuf;
	return soundBuf;
}

template <typename T>
Sound& tSoundManager<T>::getFreeSound( const T& id ) {
	typename std::map<T, sSound>::iterator it = tSounds.find( id );

	if ( it != tSounds.end() ) {
		sSound * tSound = &it->second;
		Uint32 tSize = (Uint32)tSound->Snd.size();

		for ( Uint32 i = 0; i < tSize; i++ ) {
			// If there is a free slot, use it.
			if ( tSound->Snd[i].getState() != Sound::Playing ) {
				return tSound->Snd[i];
			}
		}

		tSound->Snd.push_back( Sound( tSound->Buf ) );

		return tSound->Snd[ tSound->Snd.size() - 1 ];
	}

	return tSounds[0].Snd[0];
}

template <typename T>
Sound& tSoundManager<T>::operator[] ( const T& id ) {
	if ( tSounds.find( id ) != tSounds.end() )
		return tSounds[id].Snd[0];

	return tSounds[0].Snd[0];
}

template <typename T>
void tSoundManager<T>::play( const T& id ) {
	typename std::map<T, sSound>::iterator it = tSounds.find( id );

	if ( it != tSounds.end() ) {
		sSound * tSound = &it->second;
		Uint32 tSize = (Uint32)tSound->Snd.size();

		for ( Uint32 i = 0; i < tSize; i++ ) {
			// If there is a free slot, use it.
			if ( tSound->Snd[i].getState() != Sound::Playing ) {
				tSound->Snd[i].play();
				return;
			}
		}

		// Otherwise create a new one and play it.
		tSound->Snd.push_back( Sound( tSound->Buf ) );
		tSound->Snd[ tSize ].play();
	}
}

template <typename T>
tSoundManager<T>::~tSoundManager() {
	typename std::map<T, sSound>::iterator itr;

	for (itr = tSounds.begin(); itr != tSounds.end(); itr++)
		itr->second.Snd.clear();

	tSounds.clear();
}

template <typename T>
bool tSoundManager<T>::remove( const T& id ) {
	if ( tSounds.find( id ) != tSounds.end() ) {
		tSounds.erase( id );
		return true;
	}

	return false;
}

typedef tSoundManager<std::string> SoundManager;

}}

#endif
