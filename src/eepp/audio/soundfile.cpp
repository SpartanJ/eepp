#include <eepp/audio/soundfile.hpp>
#ifdef EE_LIBSNDFILE_ENABLED
#include <eepp/audio/soundfiledefault.hpp>
#endif
#include <eepp/audio/soundfileogg.hpp>

namespace EE { namespace Audio {

SoundFile::SoundFile() :
	mSamplesCount (0),
	mChannelCount(0),
	mSampleRate (0)
{
}

SoundFile::~SoundFile()
{
}

SoundFile * SoundFile::createRead( const std::string& Filename ) {
	// Create the file according to its type
	SoundFile * File = NULL;

	if 			( SoundFileOgg::isFileSupported( Filename, true ) )	 	File = eeNew( SoundFileOgg, () );
	#ifdef EE_LIBSNDFILE_ENABLED
	else if 	( SoundFileDefault::isFileSupported( Filename, true ) ) 	File = eeNew( SoundFileDefault, () );
	#endif

	// Open it for reading
	if ( NULL != File ) {
		std::size_t  SamplesCount;
		unsigned int ChannelCount;
		unsigned int SampleRate;

		if ( File->openRead( Filename, SamplesCount, ChannelCount, SampleRate ) ) {
			File->mFilename			= Filename;
			File->mData				= NULL;
			File->mSize				= 0;
			File->mSamplesCount		= SamplesCount;
			File->mChannelCount		= ChannelCount;
			File->mSampleRate		= SampleRate;
		} else {
			eeDelete( File );
			File = NULL;
		}
	}

	return File;
}

SoundFile * SoundFile::createRead( const char* Data, std::size_t SizeInMemory ) {
	// Create the file according to its type
	SoundFile * File = NULL;

	if			( SoundFileOgg::isFileSupported( Data, SizeInMemory ) )	 	File = eeNew( SoundFileOgg, () );
	#ifdef EE_LIBSNDFILE_ENABLED
	else if 	( SoundFileDefault::isFileSupported( Data, SizeInMemory ) ) 	File = eeNew( SoundFileDefault, () );
	#endif

	// Open it for reading
	if ( NULL != File ) {
		std::size_t  SamplesCount;
		unsigned int ChannelCount;
		unsigned int SampleRate;

		if ( File->openRead( Data, SizeInMemory, SamplesCount, ChannelCount, SampleRate ) ) {
			File->mFilename			= "";
			File->mData				= Data;
			File->mSize				= SizeInMemory;
			File->mSamplesCount		= SamplesCount;
			File->mChannelCount		= ChannelCount;
			File->mSampleRate		= SampleRate;
		} else {
			eeDelete( File );
			File = NULL;
		}
	}

	return File;
}

SoundFile * SoundFile::createWrite( const std::string& Filename, unsigned int ChannelCount, unsigned int SampleRate ) {
	// Create the file according to its type
	SoundFile * File = NULL;

	if		( SoundFileOgg::isFileSupported( Filename, false ) )		File = eeNew( SoundFileOgg, () );
	#ifdef EE_LIBSNDFILE_ENABLED
	else if	( SoundFileDefault::isFileSupported( Filename, false ) )	File = eeNew( SoundFileDefault, () );
	#endif

	// Open it for writing
	if ( NULL != File ) {
		if ( File->openWrite( Filename, ChannelCount, SampleRate ) ) {
			File->mFilename			= "";
			File->mData				= NULL;
			File->mSize				= 0;
			File->mSamplesCount		= 0;
			File->mChannelCount		= ChannelCount;
			File->mSampleRate		= SampleRate;
		} else {
			eeDelete( File );
			File = NULL;
		}
	}

	return File;
}

std::size_t SoundFile::getSamplesCount() const
{
	return mSamplesCount;
}

unsigned int SoundFile::getChannelCount() const {
	return mChannelCount;
}

unsigned int SoundFile::getSampleRate() const {
	return mSampleRate;
}

bool SoundFile::restart() {
	if ( mData ) {
		// Reopen from memory
		return openRead( mData, mSize, mSamplesCount, mChannelCount, mSampleRate );
	} else if ( mFilename != "" ) {
		// Reopen from file
		return openRead( mFilename, mSamplesCount, mChannelCount, mSampleRate );
	} else {
		eePRINTL( "Warning : trying to restart a sound opened in write mode, which is not allowed" );
		return false;
	}
}

bool SoundFile::openRead(const std::string& Filename, std::size_t&, unsigned int&, unsigned int&) {
	eePRINTL( "Failed to open sound file %s, format is not supported by eepp", Filename.c_str() );
	return false;
}

bool SoundFile::openRead(const char*, std::size_t, std::size_t&, unsigned int&, unsigned int&) {
	eePRINTL( "Failed to open sound file from memory, format is not supported by eepp" );
	return false;
}

bool SoundFile::openWrite(const std::string& Filename, unsigned int, unsigned int) {
	eePRINTL( "Failed to open sound file %s, format is not supported by eepp", Filename.c_str() );
	return false;
}

std::size_t SoundFile::read(Int16*, std::size_t) {
	eePRINTL( "Failed to read from sound file (not supported)" );
	return 0;
}

void SoundFile::write(const Int16*, std::size_t) {
	eePRINTL( "Failed to write to sound file (not supported)" );
}

void SoundFile::seek( Time timeOffset ) {
	eePRINTL( "Trying to seek a file that doesn't support seeking." );
}

}}
