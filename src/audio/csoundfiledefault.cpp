#include "csoundfiledefault.hpp"

namespace EE { namespace Audio {

cSoundFileDefault::cSoundFileDefault() : myFile(NULL) {}

cSoundFileDefault::~cSoundFileDefault() {
	if (myFile)
		sf_close(myFile);
}

bool cSoundFileDefault::IsFileSupported(const std::string& Filename, bool Read) {
	if (Read) {
		// Open the sound file
		SF_INFO FileInfos;
		SNDFILE* File = sf_open(Filename.c_str(), SFM_READ, &FileInfos);

		if (File) {
			sf_close(File);
			return true;
		} else {
			return false;
		}
	} else {
		// Check the extension
		return GetFormatFromFilename(Filename) != -1;
	}
}

bool cSoundFileDefault::IsFileSupported(const char* Data, std::size_t SizeInBytes) {
	// Define the I/O custom functions for reading from memory
	SF_VIRTUAL_IO VirtualIO;
	VirtualIO.get_filelen = &cSoundFileDefault::MemoryGetLength;
	VirtualIO.read		= &cSoundFileDefault::MemoryRead;
	VirtualIO.seek		= &cSoundFileDefault::MemorySeek;
	VirtualIO.tell		= &cSoundFileDefault::MemoryTell;
	VirtualIO.write	   = &cSoundFileDefault::MemoryWrite;

	// Initialize the memory data
	MemoryInfos Memory;
	Memory.DataStart = Data;
	Memory.DataPtr   = Data;
	Memory.TotalSize = SizeInBytes;

	// Open the sound file
	SF_INFO FileInfos;
	SNDFILE* File = sf_open_virtual(&VirtualIO, SFM_READ, &FileInfos, &Memory);

	if (File) {
		sf_close(File);
		return true;
	} else
		return false;
}

bool cSoundFileDefault::OpenRead(const std::string& Filename, std::size_t& NbSamples, unsigned int& ChannelsCount, unsigned int& SampleRate) {
	// If the file is already opened, first close it
	if (myFile)
		sf_close(myFile);

	// Open the sound file
	SF_INFO FileInfos;
	myFile = sf_open(Filename.c_str(), SFM_READ, &FileInfos);
	if (!myFile) {
		cLog::instance()->Write( "Failed to read sound file \"" + Filename + "\"" );
		return false;
	}

	// Set the sound parameters
	ChannelsCount = FileInfos.channels;
	SampleRate	= FileInfos.samplerate;
	NbSamples	 = static_cast<std::size_t>(FileInfos.frames) * ChannelsCount;

	return true;
}

bool cSoundFileDefault::OpenRead(const char* Data, std::size_t SizeInBytes, std::size_t& NbSamples, unsigned int& ChannelsCount, unsigned int& SampleRate) {
	// If the file is already opened, first close it
	if (myFile)
		sf_close(myFile);

	// Define the I/O custom functions for reading from memory
	SF_VIRTUAL_IO VirtualIO;
	VirtualIO.get_filelen = &cSoundFileDefault::MemoryGetLength;
	VirtualIO.read		= &cSoundFileDefault::MemoryRead;
	VirtualIO.seek		= &cSoundFileDefault::MemorySeek;
	VirtualIO.tell		= &cSoundFileDefault::MemoryTell;
	VirtualIO.write	   = &cSoundFileDefault::MemoryWrite;

	// Initialize the memory data
	myMemory.DataStart = Data;
	myMemory.DataPtr   = Data;
	myMemory.TotalSize = SizeInBytes;

	// Open the sound file
	SF_INFO FileInfos;
	myFile = sf_open_virtual(&VirtualIO, SFM_READ, &FileInfos, &myMemory);
	if (!myFile) {
		cLog::instance()->Write( "Failed to read sound file from memory" );
		return false;
	}

	// Set the sound parameters
	ChannelsCount = FileInfos.channels;
	SampleRate	= FileInfos.samplerate;
	NbSamples	 = static_cast<std::size_t>(FileInfos.frames) * ChannelsCount;

	return true;
}

bool cSoundFileDefault::OpenWrite(const std::string& Filename, unsigned int ChannelsCount, unsigned int SampleRate) {
	// If the file is already opened, first close it
	if (myFile)
		sf_close(myFile);

	// Find the right format according to the file extension
	int Format = GetFormatFromFilename(Filename);
	if (Format == -1) {
		// Error : unrecognized extension
		cLog::instance()->Write( "Failed to create sound file \"" + Filename + "\" : unknown format" );
		return false;
	}

	// Fill the sound infos with parameters
	SF_INFO FileInfos;
	FileInfos.channels   = ChannelsCount;
	FileInfos.samplerate = SampleRate;
	FileInfos.format	 = Format | SF_FORMAT_PCM_16;

	// Open the sound file for writing
	myFile = sf_open(Filename.c_str(), SFM_WRITE, &FileInfos);
	if (!myFile) {
		cLog::instance()->Write( "Failed to create sound file \"" + Filename + "\"" );
		return false;
	}

	return true;
}

std::size_t cSoundFileDefault::Read(Int16* Data, std::size_t NbSamples) {
	if (myFile && Data && NbSamples)
		return static_cast<std::size_t>(sf_read_short(myFile, Data, NbSamples));
	else
		return 0;
}

void cSoundFileDefault::Write(const Int16* Data, std::size_t NbSamples) {
	if (myFile && Data && NbSamples)
		sf_write_short(myFile, Data, NbSamples);
}

int cSoundFileDefault::GetFormatFromFilename(const std::string& Filename) {
	// Extract the extension
	std::string Ext = "wav";
	std::string::size_type Pos = Filename.find_last_of(".");
	if (Pos != std::string::npos)
		Ext = Filename.substr(Pos + 1);

	// Match every supported extension with its format constant
	if (Ext == "wav"  || Ext == "WAV" ) return SF_FORMAT_WAV;
	if (Ext == "aif"  || Ext == "AIF" ) return SF_FORMAT_AIFF;
	if (Ext == "aiff" || Ext == "AIFF") return SF_FORMAT_AIFF;
	if (Ext == "au"   || Ext == "AU"  ) return SF_FORMAT_AU;
	if (Ext == "raw"  || Ext == "RAW" ) return SF_FORMAT_RAW;
	if (Ext == "paf"  || Ext == "PAF" ) return SF_FORMAT_PAF;
	if (Ext == "svx"  || Ext == "SVX" ) return SF_FORMAT_SVX;
	if (Ext == "voc"  || Ext == "VOC" ) return SF_FORMAT_VOC;
	if (Ext == "sf"   || Ext == "SF"  ) return SF_FORMAT_IRCAM;
	if (Ext == "w64"  || Ext == "W64" ) return SF_FORMAT_W64;
	if (Ext == "mat4" || Ext == "MAT4") return SF_FORMAT_MAT4;
	if (Ext == "mat5" || Ext == "MAT5") return SF_FORMAT_MAT5;
	if (Ext == "pvf"  || Ext == "PVF" ) return SF_FORMAT_PVF;
	if (Ext == "htk"  || Ext == "HTK" ) return SF_FORMAT_HTK;
	if (Ext == "caf"  || Ext == "CAF" ) return SF_FORMAT_CAF;
	if (Ext == "nist" || Ext == "NIST") return SF_FORMAT_NIST; // SUPPORTED ?
	if (Ext == "sds"  || Ext == "SDS" ) return SF_FORMAT_SDS;  // SUPPORTED ?
	if (Ext == "avr"  || Ext == "AVR" ) return SF_FORMAT_AVR;  // SUPPORTED ?
	if (Ext == "sd2"  || Ext == "SD2" ) return SF_FORMAT_SD2;  // SUPPORTED ?
	if (Ext == "flac" || Ext == "FLAC") return SF_FORMAT_FLAC; // SUPPORTED ?

	return -1;
}

sf_count_t cSoundFileDefault::MemoryGetLength(void* UserData) {
	MemoryInfos* Memory = static_cast<MemoryInfos*>(UserData);

	return Memory->TotalSize;
}

sf_count_t cSoundFileDefault::MemoryRead(void* Ptr, sf_count_t Count, void* UserData) {
	MemoryInfos* Memory = static_cast<MemoryInfos*>(UserData);

	sf_count_t Position = Memory->DataPtr - Memory->DataStart;
	if (Position + Count >= Memory->TotalSize)
		Count = Memory->TotalSize - Position;

	memcpy(Ptr, Memory->DataPtr, static_cast<std::size_t>(Count));

	Memory->DataPtr += Count;

	return Count;
}

sf_count_t cSoundFileDefault::MemorySeek(sf_count_t Offset, int Whence, void* UserData) {
	MemoryInfos* Memory = static_cast<MemoryInfos*>(UserData);

	sf_count_t Position = 0;
	switch (Whence) {
		case SEEK_SET :
			Position = Offset;
			break;
		case SEEK_CUR :
			Position = Memory->DataPtr - Memory->DataStart + Offset;
			break;
		case SEEK_END :
			Position = Memory->TotalSize - Offset;
			break;
		default :
			Position = 0;
			break;
	}

	if (Position >= Memory->TotalSize)
		Position = Memory->TotalSize - 1;
	else if (Position < 0)
		Position = 0;

	Memory->DataPtr = Memory->DataStart + Position;

	return Position;
}

sf_count_t cSoundFileDefault::MemoryTell(void* UserData) {
	MemoryInfos* Memory = static_cast<MemoryInfos*>(UserData);

	return Memory->DataPtr - Memory->DataStart;
}

sf_count_t cSoundFileDefault::MemoryWrite(const void*, sf_count_t, void*) {
	return 0;
}

}}
