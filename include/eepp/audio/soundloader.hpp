#ifndef EE_AUDIOTSOUNDLOADER_HPP
#define EE_AUDIOTSOUNDLOADER_HPP

#include <eepp/audio/base.hpp>
#include <eepp/audio/soundmanager.hpp>
#include <eepp/system/objectloader.hpp>

namespace EE { namespace Audio {

#define SND_LT_PATH 	(1)
#define SND_LT_MEM 		(2)
#define SND_LT_PACK 	(3)
#define SND_LT_SAMPLES	(4)

/** @brief A helper template to load sounds in synchronous or asynchronous mode.
**	@see ObjectLoader */
template <typename T>
class tSoundLoader : public ObjectLoader {
	public:
		/** @brief Load the sound from file */
		tSoundLoader( tSoundManager<T> * SndMngr, const T& id, const std::string& filepath );

		/** @brief Load the sound from memory */
		tSoundLoader( tSoundManager<T> * SndMngr, const T& id, const char* Data, std::size_t SizeInBytes );

		/** @brief Load the sound from an array of samples */
		tSoundLoader( tSoundManager<T> * SndMngr, const T& id, const Int16* Samples, std::size_t SamplesCount, unsigned int ChannelCount, unsigned int SampleRate );

		/** @brief Load the sound from the Pack file */
		tSoundLoader( tSoundManager<T> * SndMngr, const T& id, Pack* Pack, const std::string& FilePackPath );

		~tSoundLoader();

		/** Unload the sound if was already loaded. */
		void					unload();

		/** @return The sound id */
		const T&				id() const;
	protected:
		Uint32					mLoadType;
		tSoundManager<T> *		mSndMngr;
		T						mId;
		std::string 			mFilepath;
		const char * 			mData;
		Uint32					mDataSize;
		const Int16 *			mSamples;
		Uint32					mSamplesCount;
		Uint32					mChannelCount;
		Uint32					mSampleRate;
		Pack *					mPack;

		void 					start();
	private:
		void 					loadFromPath();
		void					loadFromMemory();
		void					loadFromPack();
		void 					loadFromSamples();
};

template <typename T>
tSoundLoader<T>::tSoundLoader( tSoundManager<T> * SndMngr,
	const T& id,
	const std::string& filepath
) : ObjectLoader( ObjectLoader::SoundLoaderType ),
	mLoadType(SND_LT_PATH),
	mSndMngr(SndMngr),
	mId(id),
	mFilepath(filepath)
{
}

template <typename T>
tSoundLoader<T>::tSoundLoader( tSoundManager<T> * SndMngr,
	const T& id,
	const char * Data,
	std::size_t SizeInBytes
) : ObjectLoader( ObjectLoader::SoundLoaderType ),
	mLoadType(SND_LT_MEM),
	mSndMngr(SndMngr),
	mId(id),
	mData(Data),
	mDataSize(SizeInBytes)
{
}

template <typename T>
tSoundLoader<T>::tSoundLoader( tSoundManager<T> * SndMngr,
	const T& id,
	const Int16* Samples,
	std::size_t SamplesCount,
	unsigned int ChannelCount,
	unsigned int SampleRate
) : ObjectLoader( ObjectLoader::SoundLoaderType ),
	mLoadType(SND_LT_SAMPLES),
	mSndMngr(SndMngr),
	mId(id),
	mSamples(Samples),
	mSamplesCount(SamplesCount),
	mChannelCount(ChannelCount),
	mSampleRate(SampleRate)
{
}

template <typename T>
tSoundLoader<T>::tSoundLoader( tSoundManager<T> * SndMngr,
	const T& id, Pack* Pack,
	const std::string& FilePackPath
) : ObjectLoader( ObjectLoader::SoundLoaderType ),
	mLoadType(SND_LT_PACK),
	mSndMngr(SndMngr),
	mId(id),
	mFilepath(FilePackPath),
	mPack(Pack)
{
}

template <typename T>
tSoundLoader<T>::~tSoundLoader() {
}

template <typename T>
void tSoundLoader<T>::start() {
	if ( NULL != mSndMngr ) {
		ObjectLoader::start();

		if ( SND_LT_PATH == mLoadType )
			loadFromPath();
		else if ( SND_LT_MEM == mLoadType )
			loadFromMemory();
		else if ( SND_LT_PACK == mLoadType )
			loadFromPack();
		else if ( SND_LT_SAMPLES == mLoadType )
			loadFromSamples();

		setLoaded();
	}
}

template <typename T>
void tSoundLoader<T>::loadFromPath() {
	mSndMngr->loadFromFile( mId, mFilepath );
}

template <typename T>
void tSoundLoader<T>::loadFromMemory() {
	mSndMngr->loadFromMemory( mId, mData, mDataSize );
}

template <typename T>
void tSoundLoader<T>::loadFromPack() {
	mSndMngr->loadFromPack( mId, mPack, mFilepath );
}

template <typename T>
void tSoundLoader<T>::loadFromSamples() {
	mSndMngr->loadFromSamples( mId, mSamples, mSamplesCount, mChannelCount, mSampleRate );
}

template <typename T>
const T& tSoundLoader<T>::id() const {
	return mId;
}

template <typename T>
void tSoundLoader<T>::unload() {
	if ( mLoaded ) {
		mSndMngr->remove( mId );

		reset();
	}
}

typedef tSoundLoader<std::string>	SoundLoader;

}}

#endif
