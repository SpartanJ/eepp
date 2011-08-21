#include "csoundfileogg.hpp"

namespace EE { namespace Audio {

cSoundFileOgg::cSoundFileOgg() :
	mStream (NULL),
	mChannelsCount(0)
{
}

cSoundFileOgg::~cSoundFileOgg() {
	if ( NULL != mStream )
		stb_vorbis_close( mStream );
}

bool cSoundFileOgg::IsFileSupported( const std::string& Filename, bool Read ) {
	if ( Read ) {
		// Open the vorbis stream
		stb_vorbis* Stream = stb_vorbis_open_filename( const_cast<char*>( Filename.c_str() ), NULL, NULL );

		if ( NULL != Stream ) {
			stb_vorbis_close( Stream );
			return true;
		} else
			return false;
	} else // No support for writing ogg files yet...
		return false;
}

bool cSoundFileOgg::IsFileSupported( const char* Data, std::size_t SizeInBytes ) {
	// Open the vorbis stream
	unsigned char* Buffer = reinterpret_cast<unsigned char*>( const_cast<char*>( Data ) );
	int Length = static_cast<int>( SizeInBytes );

	stb_vorbis * Stream = stb_vorbis_open_memory( Buffer, Length, NULL, NULL );

	if ( NULL != Stream ) {
		stb_vorbis_close(Stream);
		return true;
	} else
		return false;
}

bool cSoundFileOgg::OpenRead( const std::string& Filename, std::size_t& NbSamples, unsigned int& ChannelsCount, unsigned int& SampleRate ) {
	// Close the file if already opened
	if ( NULL != mStream )
		stb_vorbis_close( mStream );

	// Open the vorbis stream
	mStream = stb_vorbis_open_filename( const_cast<char*>( Filename.c_str() ), NULL, NULL );

	if ( NULL == mStream ) {
		cLog::instance()->Write( "Failed to read sound file \"" + Filename + "\" (cannot open the file)" );
		return false;
	}

	// Get the music parameters
	stb_vorbis_info Infos = stb_vorbis_get_info( mStream );
	ChannelsCount	= mChannelsCount = Infos.channels;
	SampleRate		= Infos.sample_rate;
	NbSamples		= static_cast<std::size_t>(stb_vorbis_stream_length_in_samples(mStream) * ChannelsCount);

	return true;
}

bool cSoundFileOgg::OpenRead( const char* Data, std::size_t SizeInBytes, std::size_t& NbSamples, unsigned int& ChannelsCount, unsigned int& SampleRate ) {
	// Close the file if already opened
	if ( NULL != mStream )
		stb_vorbis_close( mStream );

	// Open the vorbis stream
	unsigned char* Buffer = reinterpret_cast<unsigned char*>( const_cast<char*>( Data ) );
	int Length = static_cast<int>( SizeInBytes );

	mStream = stb_vorbis_open_memory( Buffer, Length, NULL, NULL );

	if ( NULL == mStream ) {
		cLog::instance()->Write( "Failed to read sound file from memory (cannot open the file)" );
		return false;
	}

	// Get the music parameters
	stb_vorbis_info Infos = stb_vorbis_get_info( mStream );
	ChannelsCount	= mChannelsCount = Infos.channels;
	SampleRate		= Infos.sample_rate;
	NbSamples		= static_cast<std::size_t>( stb_vorbis_stream_length_in_samples( mStream ) );

	return true;
}

std::size_t cSoundFileOgg::Read( Int16 * Data, std::size_t NbSamples ) {
	if ( NULL != mStream && Data && NbSamples ) {
		int Read = stb_vorbis_get_samples_short_interleaved( mStream, mChannelsCount, Data, static_cast<int>( NbSamples ) );
		return static_cast<std::size_t>( Read * mChannelsCount );
	} else
		return 0;
}

void cSoundFileOgg::Seek( Uint32 timeOffset ) {
    if ( NULL != mStream ) {
		Uint32 frameOffset = static_cast<Uint32>( timeOffset * mSampleRate / 1000 );
        stb_vorbis_seek( mStream, frameOffset );
    }
}

}}
