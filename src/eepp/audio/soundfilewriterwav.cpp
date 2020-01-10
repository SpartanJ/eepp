#include <algorithm>
#include <cassert>
#include <cctype>
#include <eepp/audio/soundfilewriterwav.hpp>
#include <eepp/core/debug.hpp>

namespace {
using namespace EE;

// The following functions takes integers in host byte order
// and writes them to a stream as little endian

void encode( std::ostream& stream, Int16 value ) {
	unsigned char bytes[] = {static_cast<unsigned char>( value & 0xFF ),
							 static_cast<unsigned char>( value >> 8 )};
	stream.write( reinterpret_cast<const char*>( bytes ), sizeof( bytes ) );
}

void encode( std::ostream& stream, Uint16 value ) {
	unsigned char bytes[] = {static_cast<unsigned char>( value & 0xFF ),
							 static_cast<unsigned char>( value >> 8 )};
	stream.write( reinterpret_cast<const char*>( bytes ), sizeof( bytes ) );
}

void encode( std::ostream& stream, Uint32 value ) {
	unsigned char bytes[] = {
		static_cast<unsigned char>( value & 0x000000FF ),
		static_cast<unsigned char>( ( value & 0x0000FF00 ) >> 8 ),
		static_cast<unsigned char>( ( value & 0x00FF0000 ) >> 16 ),
		static_cast<unsigned char>( ( value & 0xFF000000 ) >> 24 ),
	};
	stream.write( reinterpret_cast<const char*>( bytes ), sizeof( bytes ) );
}
} // namespace

namespace EE { namespace Audio { namespace Private {

bool SoundFileWriterWav::check( const std::string& filename ) {
	std::string extension = filename.substr( filename.find_last_of( "." ) + 1 );
	std::transform( extension.begin(), extension.end(), extension.begin(), ::tolower );
	return extension == "wav";
}

SoundFileWriterWav::SoundFileWriterWav() : mFile() {}

SoundFileWriterWav::~SoundFileWriterWav() {
	close();
}

bool SoundFileWriterWav::open( const std::string& filename, unsigned int sampleRate,
							   unsigned int channelCount ) {
	// Open the file
	mFile.open( filename.c_str(), std::ios::binary );

	if ( !mFile ) {
		eePRINTL( "Failed to open WAV sound file \"%s\" for writing", filename.c_str() );
		return false;
	}

	// Write the header
	if ( !writeHeader( sampleRate, channelCount ) ) {
		eePRINTL( "Failed to write header of WAV sound file \"%s\"", filename.c_str() );
		return false;
	}

	return true;
}

void SoundFileWriterWav::write( const Int16* samples, Uint64 count ) {
	assert( mFile.good() );

	while ( count-- )
		encode( mFile, *samples++ );
}

bool SoundFileWriterWav::writeHeader( unsigned int sampleRate, unsigned int channelCount ) {
	assert( mFile.good() );

	// Write the main chunk ID
	char mainChunkId[4] = {'R', 'I', 'F', 'F'};
	mFile.write( mainChunkId, sizeof( mainChunkId ) );

	// Write the main chunk header
	Uint32 mainChunkSize = 0; // placeholder, will be written later
	encode( mFile, mainChunkSize );
	char mainChunkFormat[4] = {'W', 'A', 'V', 'E'};
	mFile.write( mainChunkFormat, sizeof( mainChunkFormat ) );

	// Write the sub-chunk 1 ("format") id and size
	char fmtChunkId[4] = {'f', 'm', 't', ' '};
	mFile.write( fmtChunkId, sizeof( fmtChunkId ) );
	Uint32 fmtChunkSize = 16;
	encode( mFile, fmtChunkSize );

	// Write the format (PCM)
	Uint16 format = 1;
	encode( mFile, format );

	// Write the sound attributes
	encode( mFile, static_cast<Uint16>( channelCount ) );
	encode( mFile, static_cast<Uint32>( sampleRate ) );
	Uint32 byteRate = sampleRate * channelCount * 2;
	encode( mFile, byteRate );
	Uint16 blockAlign = channelCount * 2;
	encode( mFile, blockAlign );
	Uint16 bitsPerSample = 16;
	encode( mFile, bitsPerSample );

	// Write the sub-chunk 2 ("data") id and size
	char dataChunkId[4] = {'d', 'a', 't', 'a'};
	mFile.write( dataChunkId, sizeof( dataChunkId ) );
	Uint32 dataChunkSize = 0; // placeholder, will be written later
	encode( mFile, dataChunkSize );

	return true;
}

void SoundFileWriterWav::close() {
	// If the file is open, finalize the header and close it
	if ( mFile.is_open() ) {
		mFile.flush();

		// Update the main chunk size and data sub-chunk size
		Uint32 fileSize = static_cast<Uint32>( mFile.tellp() );
		Uint32 mainChunkSize = fileSize - 8;  // 8 bytes RIFF header
		Uint32 dataChunkSize = fileSize - 44; // 44 bytes RIFF + WAVE headers
		mFile.seekp( 4 );
		encode( mFile, mainChunkSize );
		mFile.seekp( 40 );
		encode( mFile, dataChunkSize );

		mFile.close();
	}
}

}}} // namespace EE::Audio::Private
