#ifndef EE_AUDIOTSOUNDLOADER_HPP
#define EE_AUDIOTSOUNDLOADER_HPP

#include "base.hpp"
#include "tsoundmanager.hpp"
#include "../system/cobjectloader.hpp"

namespace EE { namespace Audio {

#define SND_LT_PATH 	(1)
#define SND_LT_MEM 		(2)
#define SND_LT_PACK 	(3)
#define SND_LT_SAMPLES	(4)

template <typename T>
class EE_API tSoundLoader : public cObjectLoader {
	public:
		tSoundLoader( tSoundManager<T> * SndMngr, const T& id, const std::string& filepath );

		tSoundLoader( tSoundManager<T> * SndMngr, const T& id, const char* Data, std::size_t SizeInBytes );

		tSoundLoader( tSoundManager<T> * SndMngr, const T& id, const Int16* Samples, std::size_t SamplesCount, unsigned int ChannelsCount, unsigned int SampleRate );

		tSoundLoader( tSoundManager<T> * SndMngr, const T& id, cPack* Pack, const std::string& FilePackPath );

		~tSoundLoader();

		void 					Update();

		void					Unload();

		const T&				Id() const;
	protected:
		Uint32					mLoadType;
		tSoundManager<T> *		mSndMngr;
		T						mId;
		std::string 			mFilepath;
		const char * 			mData;
		Uint32					mDataSize;
		const Int16 *			mSamples;
		Uint32					mSamplesCount;
		Uint32					mChannelsCount;
		Uint32					mSampleRate;
		cPack *					mPack;

		void 					Start();
	private:
		void 					LoadFromPath();
		void					LoadFromMemory();
		void					LoadFromPack();
		void 					LoadFromSamples();
};

template <typename T>
tSoundLoader<T>::tSoundLoader( tSoundManager<T> * SndMngr,
	const T& id,
	const std::string& filepath
) : cObjectLoader( cObjectLoader::SoundLoader ),
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
) : cObjectLoader( cObjectLoader::SoundLoader ),
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
	unsigned int ChannelsCount,
	unsigned int SampleRate
) : cObjectLoader( cObjectLoader::SoundLoader ),
	mLoadType(SND_LT_SAMPLES),
	mSndMngr(SndMngr),
	mId(id),
	mSamples(Samples),
	mSamplesCount(SamplesCount),
	mChannelsCount(ChannelsCount),
	mSampleRate(SampleRate)
{
}

template <typename T>
tSoundLoader<T>::tSoundLoader( tSoundManager<T> * SndMngr,
	const T& id, cPack* Pack,
	const std::string& FilePackPath
) : cObjectLoader( cObjectLoader::SoundLoader ),
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
void tSoundLoader<T>::Update() {

}

template <typename T>
void tSoundLoader<T>::Start() {
	if ( NULL != mSndMngr ) {
		cObjectLoader::Start();

		if ( SND_LT_PATH == mLoadType )
			LoadFromPath();
		else if ( SND_LT_MEM == mLoadType )
			LoadFromMemory();
		else if ( SND_LT_PACK == mLoadType )
			LoadFromPack();
		else if ( SND_LT_SAMPLES == mLoadType )
			LoadFromSamples();

		SetLoaded();
	}
}

template <typename T>
void tSoundLoader<T>::LoadFromPath() {
	mSndMngr->LoadFromFile( mId, mFilepath );
}

template <typename T>
void tSoundLoader<T>::LoadFromMemory() {
	mSndMngr->LoadFromMemory( mId, mData, mDataSize );
}

template <typename T>
void tSoundLoader<T>::LoadFromPack() {
	mSndMngr->LoadFromPack( mId, mPack, mFilepath );
}

template <typename T>
void tSoundLoader<T>::LoadFromSamples() {
	mSndMngr->LoadFromSamples( mId, mSamples, mSamplesCount, mChannelsCount, mSampleRate );
}

template <typename T>
const T& tSoundLoader<T>::Id() const {
	return mId;
}

template <typename T>
void tSoundLoader<T>::Unload() {
	if ( mLoaded ) {
		mSndMngr->Remove( mId );

		Reset();
	}
}

typedef tSoundLoader<std::string>	cSoundLoader;

}}

#endif
