#include <eepp/audio/soundfileogg.hpp>
#include <eepp/helper/stb_vorbis/stb_vorbis.h>

namespace EE { namespace Audio {

SoundFileOgg::SoundFileOgg() :
	mStream (NULL),
	mChannelCount(0)
{
}

SoundFileOgg::~SoundFileOgg() {
	if ( NULL != mStream )
		stb_vorbis_close( mStream );
}

bool SoundFileOgg::IsFileSupported( const std::string& Filename, bool Read ) {
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

bool SoundFileOgg::IsFileSupported( const char* Data, std::size_t SizeInBytes ) {
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

bool SoundFileOgg::OpenRead( const std::string& Filename, std::size_t& SamplesCount, unsigned int& ChannelCount, unsigned int& SampleRate ) {
	// Close the file if already opened
	if ( NULL != mStream )
		stb_vorbis_close( mStream );

	// Open the vorbis stream
	mStream = stb_vorbis_open_filename( const_cast<char*>( Filename.c_str() ), NULL, NULL );

	if ( NULL == mStream ) {
		eePRINTL( "Failed to read sound file %s (cannot open the file)", Filename.c_str() );
		return false;
	}

	// Get the music parameters
	stb_vorbis_info Infos = stb_vorbis_get_info( mStream );
	ChannelCount	= mChannelCount = Infos.channels;
	SampleRate		= Infos.sample_rate;
	SamplesCount	= static_cast<std::size_t>( stb_vorbis_stream_length_in_samples( mStream ) * ChannelCount );

	return true;
}

bool SoundFileOgg::OpenRead( const char* Data, std::size_t SizeInBytes, std::size_t& SamplesCount, unsigned int& ChannelCount, unsigned int& SampleRate ) {
	// Close the file if already opened
	if ( NULL != mStream )
		stb_vorbis_close( mStream );

	// Open the vorbis stream
	unsigned char* Buffer = reinterpret_cast<unsigned char*>( const_cast<char*>( Data ) );
	int Length = static_cast<int>( SizeInBytes );

	mStream = stb_vorbis_open_memory( Buffer, Length, NULL, NULL );

	if ( NULL == mStream ) {
		eePRINTL( "Failed to read sound file from memory (cannot open the file)" );
		return false;
	}

	// Get the music parameters
	stb_vorbis_info Infos = stb_vorbis_get_info( mStream );
	ChannelCount	= mChannelCount = Infos.channels;
	SampleRate		= Infos.sample_rate;
	SamplesCount	= static_cast<std::size_t>( stb_vorbis_stream_length_in_samples( mStream ) * ChannelCount );

	return true;
}

std::size_t SoundFileOgg::Read( Int16 * Data, std::size_t SamplesCount ) {
	if ( NULL != mStream && Data && SamplesCount ) {
		int Read = stb_vorbis_get_samples_short_interleaved( mStream, mChannelCount, Data, static_cast<int>( SamplesCount ) );

		std::size_t scount = Read * mChannelCount;

		return scount;
	}

	return 0;
}

void SoundFileOgg::Seek( Time timeOffset ) {
	if ( NULL != mStream ) {
		Uint32 frameOffset = static_cast<Uint32>( timeOffset.asSeconds() * mSampleRate / 1000 );
		stb_vorbis_seek( mStream, frameOffset );
	}
}

}}
