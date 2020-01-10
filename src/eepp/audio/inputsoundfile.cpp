#include <eepp/audio/inputsoundfile.hpp>
#include <eepp/audio/soundfilefactory.hpp>
#include <eepp/audio/soundfilereader.hpp>
#include <eepp/core/debug.hpp>
#include <eepp/system/iostream.hpp>
#include <eepp/system/iostreamfile.hpp>
#include <eepp/system/iostreammemory.hpp>

namespace EE { namespace Audio {

InputSoundFile::InputSoundFile() :
	mReader( NULL ),
	mStream( NULL ),
	mStreamOwned( false ),
	mSampleOffset( 0 ),
	mSampleCount( 0 ),
	mChannelCount( 0 ),
	mSampleRate( 0 ) {}

InputSoundFile::~InputSoundFile() {
	// Close the file in case it was open
	close();
}

bool InputSoundFile::openFromFile( const std::string& filename ) {
	// If the file is already open, first close it
	close();

	// Find a suitable reader for the file type
	mReader = SoundFileFactory::createReaderFromFilename( filename );
	if ( !mReader )
		return false;

	// Wrap the file into a stream
	IOStreamFile* file = new IOStreamFile( filename );
	mStream = file;
	mStreamOwned = true;

	// Open it
	if ( !file->isOpen() ) {
		close();
		return false;
	}

	// Pass the stream to the reader
	SoundFileReader::Info info;
	if ( !mReader->open( *file, info ) ) {
		close();
		return false;
	}

	// Retrieve the attributes of the open sound file
	mSampleCount = info.sampleCount;
	mChannelCount = info.channelCount;
	mSampleRate = info.sampleRate;

	return true;
}

bool InputSoundFile::openFromMemory( const void* data, std::size_t sizeInBytes ) {
	// If the file is already open, first close it
	close();

	// Find a suitable reader for the file type
	mReader = SoundFileFactory::createReaderFromMemory( data, sizeInBytes );
	if ( !mReader )
		return false;

	// Wrap the memory file into a stream
	IOStreamMemory* memory = new IOStreamMemory( (const char*)data, sizeInBytes );
	mStream = memory;
	mStreamOwned = true;

	// Pass the stream to the reader
	SoundFileReader::Info info;
	if ( !mReader->open( *memory, info ) ) {
		close();
		return false;
	}

	// Retrieve the attributes of the open sound file
	mSampleCount = info.sampleCount;
	mChannelCount = info.channelCount;
	mSampleRate = info.sampleRate;

	return true;
}

bool InputSoundFile::openFromStream( IOStream& stream ) {
	// If the file is already open, first close it
	close();

	// Find a suitable reader for the file type
	mReader = SoundFileFactory::createReaderFromStream( stream );
	if ( !mReader )
		return false;

	// store the stream
	mStream = &stream;
	mStreamOwned = false;

	// Don't forget to reset the stream to its beginning before re-opening it
	if ( stream.seek( 0 ) != 0 ) {
		eePRINTL( "Failed to open sound file from stream (cannot restart stream)" );
		return false;
	}

	// Pass the stream to the reader
	SoundFileReader::Info info;
	if ( !mReader->open( stream, info ) ) {
		close();
		return false;
	}

	// Retrieve the attributes of the open sound file
	mSampleCount = info.sampleCount;
	mChannelCount = info.channelCount;
	mSampleRate = info.sampleRate;

	return true;
}

Uint64 InputSoundFile::getSampleCount() const {
	return mSampleCount;
}

unsigned int InputSoundFile::getChannelCount() const {
	return mChannelCount;
}

unsigned int InputSoundFile::getSampleRate() const {
	return mSampleRate;
}

Time InputSoundFile::getDuration() const {
	// Make sure we don't divide by 0
	if ( mChannelCount == 0 || mSampleRate == 0 ) {
		return Time::Zero;
	}

	return Seconds( static_cast<float>( mSampleCount ) / mChannelCount / mSampleRate );
}

Time InputSoundFile::getTimeOffset() const {
	// Make sure we don't divide by 0
	if ( mChannelCount == 0 || mSampleRate == 0 ) {
		return Time::Zero;
	}

	return Seconds( static_cast<float>( mSampleOffset ) / mChannelCount / mSampleRate );
}

Uint64 InputSoundFile::getSampleOffset() const {
	return mSampleOffset;
}

void InputSoundFile::seek( Uint64 sampleOffset ) {
	if ( mReader ) {
		// The reader handles an overrun gracefully, but we
		// pre-check to keep our known position consistent
		mSampleOffset = std::min( sampleOffset, mSampleCount );
		mReader->seek( mSampleOffset );
	}
}

void InputSoundFile::seek( Time timeOffset ) {
	seek( static_cast<Uint64>( timeOffset.asSeconds() * mSampleRate * mChannelCount ) );
}

Uint64 InputSoundFile::read( Int16* samples, Uint64 maxCount ) {
	Uint64 readSamples = 0;
	if ( mReader && samples && maxCount )
		readSamples = mReader->read( samples, maxCount );
	mSampleOffset += readSamples;
	return readSamples;
}

void InputSoundFile::close() {
	// Destroy the reader
	delete mReader;
	mReader = NULL;

	// Destroy the stream if we own it
	if ( mStreamOwned ) {
		delete mStream;
		mStreamOwned = false;
	}
	mStream = NULL;
	mSampleOffset = 0;

	// Reset the sound file attributes
	mSampleCount = 0;
	mChannelCount = 0;
	mSampleRate = 0;
}

}} // namespace EE::Audio
