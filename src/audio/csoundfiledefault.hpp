#ifndef EE_AUDIOCSOUNDFILEDEFAULT_H
#define EE_AUDIOCSOUNDFILEDEFAULT_H

#include "base.hpp"
#include "csoundfile.hpp"

namespace EE { namespace Audio {

class cSoundFileDefault : public cSoundFile {
	public :
		cSoundFileDefault();
		~cSoundFileDefault();
		
		/** Check if a given file is supported by this loader. */
		static bool IsFileSupported(const std::string& Filename, bool Read);
		
		/** Check if a given file in memory is supported by this loader. */
		static bool IsFileSupported(const char* Data, std::size_t SizeInBytes);
	
		virtual std::size_t Read(Int16* Data, std::size_t NbSamples);
		virtual void Write(const Int16* Data, std::size_t NbSamples);
	private :
		virtual bool OpenRead(const std::string& Filename, std::size_t& NbSamples, unsigned int& ChannelsCount, unsigned int& SampleRate);
		virtual bool OpenRead(const char* Data, std::size_t SizeInBytes, std::size_t& NbSamples, unsigned int& ChannelsCount, unsigned int& SampleRate);
		virtual bool OpenWrite(const std::string& Filename, unsigned int ChannelsCount, unsigned int SampleRate);
		
		static int GetFormatFromFilename(const std::string& Filename);
	
		static sf_count_t MemoryGetLength(void* UserData);
		static sf_count_t MemoryRead(void* Ptr, sf_count_t Count, void* UserData);
		static sf_count_t MemorySeek(sf_count_t Offset, int Whence, void* UserData);
		static sf_count_t MemoryTell(void* UserData);
		static sf_count_t MemoryWrite(const void* Ptr, sf_count_t Count, void* UserData);
	
		struct MemoryInfos {
			const char* DataStart; ///< Pointer to the begining of the data
			const char* DataPtr;   ///< Pointer to the current read / write position
			sf_count_t  TotalSize; ///< Total size of the data, in bytes
		};
	
		SNDFILE*	myFile;   ///< File descriptor
		MemoryInfos myMemory; ///< Memory read / write data
};

}}

#endif
