#ifndef EE_AUDIOCSOUNDFILEDEFAULT_H
#define EE_AUDIOCSOUNDFILEDEFAULT_H

#include <eepp/audio/base.hpp>

#ifdef EE_LIBSNDFILE_ENABLED

#include <sndfile.h>
#include <eepp/audio/csoundfile.hpp>

namespace EE { namespace Audio {

class EE_API cSoundFileDefault : public cSoundFile {
	public :
		cSoundFileDefault();

		~cSoundFileDefault();

		/** Check if a given file is supported by this loader. */
		static bool IsFileSupported(const std::string& Filename, bool Read);

		/** Check if a given file in memory is supported by this loader. */
		static bool IsFileSupported(const char* Data, std::size_t SizeInBytes);

		virtual std::size_t Read(Int16* Data, std::size_t SamplesCount);

		virtual void Write(const Int16* Data, std::size_t SamplesCount);

		virtual void Seek( cTime timeOffset );
	private :
		virtual bool OpenRead( const std::string& Filename, std::size_t& SamplesCount, unsigned int& ChannelCount, unsigned int& SampleRate );

		virtual bool OpenRead( const char* Data, std::size_t SizeInBytes, std::size_t& SamplesCount, unsigned int& ChannelCount, unsigned int& SampleRate );

		virtual bool OpenWrite( const std::string& Filename, unsigned int ChannelCount, unsigned int SampleRate );

		static int GetFormatFromFilename(const std::string& Filename);

		class MemoryIO {
			public:
				SF_VIRTUAL_IO Prepare(const void* data, std::size_t sizeInBytes);
			private:
				static sf_count_t GetLength(void* UserData);
				static sf_count_t Read(void* Ptr, sf_count_t Count, void* UserData);
				static sf_count_t Seek(sf_count_t Offset, int Whence, void* UserData);
				static sf_count_t Tell(void* UserData);
				static sf_count_t Write(const void* Ptr, sf_count_t Count, void* UserData);

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
