#include "csoundfileogg.hpp"

namespace EE { namespace Audio {

cSoundFileOgg::cSoundFileOgg() : myStream (NULL), myChannelsCount(0) {}

cSoundFileOgg::~cSoundFileOgg() {
	if (myStream)
		stb_vorbis_close(myStream);
}

bool cSoundFileOgg::IsFileSupported(const std::string& Filename, bool Read) {
	if (Read) {
		// Open the vorbis stream
		stb_vorbis* Stream = stb_vorbis_open_filename(const_cast<char*>(Filename.c_str()), NULL, NULL);

		if (Stream) {
			stb_vorbis_close(Stream);
			return true;
		} else
			return false;
	} else // No support for writing ogg files yet...
		return false;
}

bool cSoundFileOgg::IsFileSupported(const char* Data, std::size_t SizeInBytes) {
	// Open the vorbis stream
	unsigned char* Buffer = reinterpret_cast<unsigned char*>(const_cast<char*>(Data));
	int Length = static_cast<int>(SizeInBytes);
	stb_vorbis* Stream = stb_vorbis_open_memory(Buffer, Length, NULL, NULL);

	if (Stream) {
		stb_vorbis_close(Stream);
		return true;
	} else
		return false;

}

bool cSoundFileOgg::OpenRead(const std::string& Filename, std::size_t& NbSamples, unsigned int& ChannelsCount, unsigned int& SampleRate) {
	// Close the file if already opened
	if (myStream)
		stb_vorbis_close(myStream);
	
	// Open the vorbis stream
	myStream = stb_vorbis_open_filename(const_cast<char*>(Filename.c_str()), NULL, NULL);
	if (myStream == NULL) {
		cLog::instance()->Write( "Failed to read sound file \"" + Filename + "\" (cannot open the file)" );
		return false;
	}

	// Get the music parameters
	stb_vorbis_info Infos = stb_vorbis_get_info(myStream);
	ChannelsCount = myChannelsCount = Infos.channels;
	SampleRate	= Infos.sample_rate;
	NbSamples	 = static_cast<std::size_t>(stb_vorbis_stream_length_in_samples(myStream) * ChannelsCount);

	return true;
}

bool cSoundFileOgg::OpenRead(const char* Data, std::size_t SizeInBytes, std::size_t& NbSamples, unsigned int& ChannelsCount, unsigned int& SampleRate) {
	// Close the file if already opened
	if (myStream)
		stb_vorbis_close(myStream);

	// Open the vorbis stream
	unsigned char* Buffer = reinterpret_cast<unsigned char*>(const_cast<char*>(Data));
	int Length = static_cast<int>(SizeInBytes);
	myStream = stb_vorbis_open_memory(Buffer, Length, NULL, NULL);
	if (myStream == NULL) {
		cLog::instance()->Write( "Failed to read sound file from memory (cannot open the file)" );
		return false;
	}

	// Get the music parameters
	stb_vorbis_info Infos = stb_vorbis_get_info(myStream);
	ChannelsCount = myChannelsCount = Infos.channels;
	SampleRate	= Infos.sample_rate;
	NbSamples	 = static_cast<std::size_t>(stb_vorbis_stream_length_in_samples(myStream));

	return true;
}

std::size_t cSoundFileOgg::Read(Int16* Data, std::size_t NbSamples) {
	if (myStream && Data && NbSamples) {
		int Read = stb_vorbis_get_samples_short_interleaved(myStream, myChannelsCount, Data, static_cast<int>(NbSamples));
		return static_cast<std::size_t>(Read * myChannelsCount);
	} else
		return 0;
}

}}
