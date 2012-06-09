#ifndef EE_AUDIOCSOUNDFILE_H
#define EE_AUDIOCSOUNDFILE_H

#include <eepp/audio/base.hpp>

namespace EE { namespace Audio {

class EE_API cSoundFile {
	public:
		static cSoundFile * CreateRead(const std::string& Filename);

		static cSoundFile * CreateRead(const char* Data, std::size_t SizeInBytes);

		static cSoundFile * CreateWrite(const std::string& Filename, unsigned int ChannelsCount, unsigned int SampleRate);

		virtual ~cSoundFile();

		std::size_t GetSamplesCount() const;

		unsigned int GetChannelsCount() const;

		unsigned int GetSampleRate() const;

		bool Restart();

		virtual std::size_t Read(Int16* Data, std::size_t NbSamples);

		virtual void Write(const Int16* Data, std::size_t NbSamples);

		virtual void Seek( Uint32 timeOffset );
	protected :
		cSoundFile();

		virtual bool OpenRead(const std::string& Filename, std::size_t& NbSamples, unsigned int& ChannelsCount, unsigned int& SampleRate);

		virtual bool OpenRead(const char* Data, std::size_t SizeInBytes, std::size_t& NbSamples, unsigned int& ChannelsCount, unsigned int& SampleRate);

		virtual bool OpenWrite(const std::string& Filename, unsigned int ChannelsCount, unsigned int SampleRate);

		std::size_t		mNbSamples;
		unsigned int	mChannelsCount;
		unsigned int	mSampleRate;
		std::string		mFilename;
		const char *	mData;
		std::size_t		mSize;
};

}}

#endif
