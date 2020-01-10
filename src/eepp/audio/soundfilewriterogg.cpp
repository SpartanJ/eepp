#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <eepp/audio/soundfilewriterogg.hpp>
#include <eepp/core/debug.hpp>

namespace EE { namespace Audio { namespace Private {

bool SoundFileWriterOgg::check( const std::string& filename ) {
	std::string extension = filename.substr( filename.find_last_of( "." ) + 1 );
	std::transform( extension.begin(), extension.end(), extension.begin(), ::tolower );

	return extension == "ogg";
}

SoundFileWriterOgg::SoundFileWriterOgg() :
	mChannelCount( 0 ), mFile(), mOgg(), mVorbis(), mState() {}

SoundFileWriterOgg::~SoundFileWriterOgg() {
	close();
}

bool SoundFileWriterOgg::open( const std::string& filename, unsigned int sampleRate,
							   unsigned int channelCount ) {
	// Save the channel count
	mChannelCount = channelCount;

	// Initialize the ogg/vorbis stream
	ogg_stream_init( &mOgg, std::rand() );
	vorbis_info_init( &mVorbis );

	// Setup the encoder: VBR, automatic bitrate management
	// Quality is in range [-1 .. 1], 0.4 gives ~128 kbps for a 44 KHz stereo sound
	int status = vorbis_encode_init_vbr( &mVorbis, channelCount, sampleRate, 0.4f );

	if ( status < 0 ) {
		eePRINTL( "Failed to write ogg/vorbis file \"%s\" (unsupported bitrate)",
				  filename.c_str() );
		close();
		return false;
	}

	vorbis_analysis_init( &mState, &mVorbis );

	// Open the file after the vorbis setup is ok
	mFile.open( filename.c_str(), std::ios::binary );

	if ( !mFile ) {
		eePRINTL( "Failed to write ogg/vorbis file \"%s\" (cannot open file)", filename.c_str() );
		close();
		return false;
	}

	// Generate header metadata (leave it empty)
	vorbis_comment comment;
	vorbis_comment_init( &comment );

	// Generate the header packets
	ogg_packet header, headerComm, headerCode;
	status = vorbis_analysis_headerout( &mState, &comment, &header, &headerComm, &headerCode );
	vorbis_comment_clear( &comment );
	if ( status < 0 ) {
		eePRINTL( "Failed to write ogg/vorbis file \"%s\" (cannot generate the headers)",
				  filename.c_str() );
		close();
		return false;
	}

	// Write the header packets to the ogg stream
	ogg_stream_packetin( &mOgg, &header );
	ogg_stream_packetin( &mOgg, &headerComm );
	ogg_stream_packetin( &mOgg, &headerCode );

	// This ensures the actual audio data will start on a new page, as per spec
	ogg_page page;
	while ( ogg_stream_flush( &mOgg, &page ) > 0 ) {
		mFile.write( reinterpret_cast<const char*>( page.header ), page.header_len );
		mFile.write( reinterpret_cast<const char*>( page.body ), page.body_len );
	}

	return true;
}

void SoundFileWriterOgg::write( const Int16* samples, Uint64 count ) {
	// Vorbis has issues with buffers that are too large, so we ask for 64K
	static const int bufferSize = 65536;

	// A frame contains a sample from each channel
	int frameCount = static_cast<int>( count / mChannelCount );

	while ( frameCount > 0 ) {
		// Prepare a buffer to hold our samples
		float** buffer = vorbis_analysis_buffer( &mState, bufferSize );
		assert( buffer );

		// Write the samples to the buffer, converted to float
		for ( int i = 0; i < std::min( frameCount, bufferSize ); ++i )
			for ( unsigned int j = 0; j < mChannelCount; ++j )
				buffer[j][i] = *samples++ / 32767.0f;

		// Tell the library how many samples we've written
		vorbis_analysis_wrote( &mState, std::min( frameCount, bufferSize ) );

		frameCount -= bufferSize;

		// Flush any produced block
		flushBlocks();
	}
}

void SoundFileWriterOgg::flushBlocks() {
	// Let the library divide uncompressed data into blocks, and process them
	vorbis_block block;
	vorbis_block_init( &mState, &block );

	while ( vorbis_analysis_blockout( &mState, &block ) == 1 ) {
		// Let the automatic bitrate management do its job
		vorbis_analysis( &block, NULL );
		vorbis_bitrate_addblock( &block );

		// Get new packets from the bitrate management engine
		ogg_packet packet;
		while ( vorbis_bitrate_flushpacket( &mState, &packet ) ) {
			// Write the packet to the ogg stream
			ogg_stream_packetin( &mOgg, &packet );

			// If the stream produced new pages, write them to the output file
			ogg_page page;
			while ( ogg_stream_flush( &mOgg, &page ) > 0 ) {
				mFile.write( reinterpret_cast<const char*>( page.header ), page.header_len );
				mFile.write( reinterpret_cast<const char*>( page.body ), page.body_len );
			}
		}
	}

	// Clear the allocated block
	vorbis_block_clear( &block );
}

void SoundFileWriterOgg::close() {
	if ( mFile.is_open() ) {
		// Submit an empty packet to mark the end of stream
		vorbis_analysis_wrote( &mState, 0 );
		flushBlocks();

		// Close the file
		mFile.close();
	}

	// Clear all the ogg/vorbis structures
	ogg_stream_clear( &mOgg );
	vorbis_dsp_clear( &mState );
	vorbis_info_clear( &mVorbis );
}

}}} // namespace EE::Audio::Private
