#ifndef EE_AUDIOCSOUNDFILEDEFAULT_H
#define EE_AUDIOCSOUNDFILEDEFAULT_H

#include <eepp/audio/base.hpp>

#ifdef EE_LIBSNDFILE_ENABLED

#include <sndfile.h>
#include <eepp/audio/soundfile.hpp>

namespace EE { namespace Audio {

class EE_API SoundFileDefault : public SoundFile {
	public :
		SoundFileDefault();

		~SoundFileDefault();

		/** Check if a given file is supported by this loader. */
		static bool isFileSupported(const std::string& Filename, bool read);

		/** Check if a given file in memory is supported by this loader. */
		static bool isFileSupported(const char* Data, std::size_t SizeInBytes);

		virtual std::size_t read(Int16* Data, std::size_t SamplesCount);

		virtual void write(const Int16* Data, std::size_t SamplesCount);

		virtual void seek( Time timeOffset );
	private :
		virtual bool openRead( const std::string& Filename, std::size_t& SamplesCount, unsigned int& ChannelCount, unsigned int& SampleRate );

		virtual bool openRead( const char* Data, std::size_t SizeInBytes, std::size_t& SamplesCount, unsigned int& ChannelCount, unsigned int& SampleRate );

		virtual bool openWrite( const std::string& Filename, unsigned int ChannelCount, unsigned int SampleRate );

		static int getFormatFromFilename(const std::string& Filename);

		class MemoryIO {
			public:
				SF_VIRTUAL_IO prepare(const void* data, std::size_t sizeInBytes);
			private:
				static sf_count_t getLength(void* UserData);
				static sf_count_t read(void* Ptr, sf_count_t Count, void* UserData);
				static sf_count_t seek(sf_count_t Offset, int Whence, void* UserData);
				static sf_count_t tell(void* UserData);
				static sf_count_t write(const void* Ptr, sf_count_t Count, void* UserData);

			const char *	mDataStart; ///< Pointer to the begining of the data
			const char *	mDataPtr;   ///< Pointer to the current read / write position
			sf_count_t		mTotalSize; ///< Total size of the data, in bytes
		};

		SNDFILE *	mFile;   ///< File descriptor
		MemoryIO 	mMemoryIO; ///< Memory read / write data
};

}}

#endif

#endif
