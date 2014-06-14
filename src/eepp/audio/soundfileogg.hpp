#ifndef EE_AUDIOCSOUNDFILEOGG_H
#define EE_AUDIOCSOUNDFILEOGG_H

#include <eepp/audio/base.hpp>
#include <eepp/audio/soundfile.hpp>

struct stb_vorbis;

namespace EE { namespace Audio {

class EE_API SoundFileOgg : public SoundFile {
	public:
		SoundFileOgg();
		~SoundFileOgg();

		/** Check if a given file is supported by this loader. */
		static bool IsFileSupported(const std::string& Filename, bool Read);

		/** Check if a given file in memory is supported by this loader. */
		static bool IsFileSupported(const char* Data, std::size_t SizeInBytes);

		virtual std::size_t Read(Int16* Data, std::size_t SamplesCount);

		virtual void Seek( Time timeOffset );
	private :
		virtual bool OpenRead(const std::string& Filename, std::size_t& SamplesCount, unsigned int& ChannelCount, unsigned int& SampleRate);

		virtual bool OpenRead(const char* Data, std::size_t SizeInBytes, std::size_t& SamplesCount, unsigned int& ChannelCount, unsigned int& SampleRate);

		stb_vorbis *	mStream;			///< Vorbis stream
		unsigned int	mChannelCount;		///< Number of channels (1 = mono, 2 = stereo)
};

}}

#endif
