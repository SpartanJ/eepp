#include <algorithm>
#include <cassert>
#include <cctype>
#include <eepp/audio/soundfilereaderogg.hpp>
#include <eepp/core/debug.hpp>
#include <eepp/system/iostreammemory.hpp>

namespace {
size_t read( void* ptr, size_t size, size_t nmemb, void* data ) {
	IOStream* stream = static_cast<IOStream*>( data );
	return static_cast<std::size_t>( stream->read( (char*)ptr, size * nmemb ) );
}

int seek( void* data, ogg_int64_t offset, int whence ) {
	IOStream* stream = static_cast<IOStream*>( data );
	switch ( whence ) {
		case SEEK_SET:
			break;
		case SEEK_CUR:
			offset += stream->tell();
			break;
		case SEEK_END:
			offset = stream->getSize() - offset;
	}
	return static_cast<int>( stream->seek( offset ) );
}

long tell( void* data ) {
	IOStream* stream = static_cast<IOStream*>( data );
	return static_cast<long>( stream->tell() );
}

static ov_callbacks callbacks = {&read, &seek, NULL, &tell};
} // namespace

namespace EE { namespace Audio { namespace Private {

bool SoundFileReaderOgg::check( IOStream& stream ) {
	OggVorbis_File file;
	if ( ov_test_callbacks( &stream, &file, NULL, 0, callbacks ) == 0 ) {
		ov_clear( &file );
		return true;
	} else {
		return false;
	}
}

SoundFileReaderOgg::SoundFileReaderOgg() : mVorbis(), mChannelCount( 0 ) {
	mVorbis.datasource = NULL;
}

SoundFileReaderOgg::~SoundFileReaderOgg() {
	close();
}

bool SoundFileReaderOgg::open( IOStream& stream, Info& info ) {
	// Open the Vorbis stream
	int status = ov_open_callbacks( &stream, &mVorbis, NULL, 0, callbacks );

	if ( status < 0 ) {
		eePRINTL( "Failed to open Vorbis file for reading" );
		return false;
	}

	// Retrieve the music attributes
	vorbis_info* vorbisInfo = ov_info( &mVorbis, -1 );
	info.channelCount = vorbisInfo->channels;
	info.sampleRate = vorbisInfo->rate;
	info.sampleCount =
		static_cast<std::size_t>( ov_pcm_total( &mVorbis, -1 ) * vorbisInfo->channels );

	// We must keep the channel count for the seek function
	mChannelCount = info.channelCount;

	return true;
}

void SoundFileReaderOgg::seek( Uint64 sampleOffset ) {
	assert( mVorbis.datasource );

	ov_pcm_seek( &mVorbis, sampleOffset / mChannelCount );
}

Uint64 SoundFileReaderOgg::read( Int16* samples, Uint64 maxCount ) {
	assert( mVorbis.datasource );

	// Try to read the requested number of samples, stop only on error or end of file
	Uint64 count = 0;
	while ( count < maxCount ) {
		int bytesToRead = static_cast<int>( maxCount - count ) * sizeof( Int16 );
		long bytesRead =
			ov_read( &mVorbis, reinterpret_cast<char*>( samples ), bytesToRead, 0, 2, 1, NULL );

		if ( bytesRead > 0 ) {
			long samplesRead = bytesRead / sizeof( Int16 );
			count += samplesRead;
			samples += samplesRead;
		} else {
			// error or end of file
			break;
		}
	}

	return count;
}

void SoundFileReaderOgg::close() {
	if ( mVorbis.datasource ) {
		ov_clear( &mVorbis );
		mVorbis.datasource = NULL;
		mChannelCount = 0;
	}
}

}}} // namespace EE::Audio::Private
