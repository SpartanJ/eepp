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

SoundFile * SoundFile::CreateRead( const std::string& Filename ) {
	// Create the file according to its type
	SoundFile * File = NULL;

	if 			( SoundFileOgg::IsFileSupported( Filename, true ) )	 	File = eeNew( SoundFileOgg, () );
	#ifdef EE_LIBSNDFILE_ENABLED
	else if 	( SoundFileDefault::IsFileSupported( Filename, true ) ) 	File = eeNew( SoundFileDefault, () );
	#endif

	// Open it for reading
	if ( NULL != File ) {
		std::size_t  SamplesCount;
		unsigned int ChannelCount;
		unsigned int SampleRate;

		if ( File->OpenRead( Filename, SamplesCount, ChannelCount, SampleRate ) ) {
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

SoundFile * SoundFile::CreateRead( const char* Data, std::size_t SizeInMemory ) {
	// Create the file according to its type
	SoundFile * File = NULL;

	if			( SoundFileOgg::IsFileSupported( Data, SizeInMemory ) )	 	File = eeNew( SoundFileOgg, () );
	#ifdef EE_LIBSNDFILE_ENABLED
	else if 	( SoundFileDefault::IsFileSupported( Data, SizeInMemory ) ) 	File = eeNew( SoundFileDefault, () );
	#endif

	// Open it for reading
	if ( NULL != File ) {
		std::size_t  SamplesCount;
		unsigned int ChannelCount;
		unsigned int SampleRate;

		if ( File->OpenRead( Data, SizeInMemory, SamplesCount, ChannelCount, SampleRate ) ) {
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

SoundFile * SoundFile::CreateWrite( const std::string& Filename, unsigned int ChannelCount, unsigned int SampleRate ) {
	// Create the file according to its type
	SoundFile * File = NULL;

	if		( SoundFileOgg::IsFileSupported( Filename, false ) )		File = eeNew( SoundFileOgg, () );
	#ifdef EE_LIBSNDFILE_ENABLED
	else if	( SoundFileDefault::IsFileSupported( Filename, false ) )	File = eeNew( SoundFileDefault, () );
	#endif

	// Open it for writing
	if ( NULL != File ) {
		if ( File->OpenWrite( Filename, ChannelCount, SampleRate ) ) {
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

std::size_t SoundFile::GetSamplesCount() const
{
	return mSamplesCount;
}

unsigned int SoundFile::GetChannelCount() const {
	return mChannelCount;
}

unsigned int SoundFile::GetSampleRate() const {
	return mSampleRate;
}

bool SoundFile::Restart() {
	if ( mData ) {
		// Reopen from memory
		return OpenRead( mData, mSize, mSamplesCount, mChannelCount, mSampleRate );
	} else if ( mFilename != "" ) {
		// Reopen from file
		return OpenRead( mFilename, mSamplesCount, mChannelCount, mSampleRate );
	} else {
		eePRINTL( "Warning : trying to restart a sound opened in write mode, which is not allowed" );
		return false;
	}
}

bool SoundFile::OpenRead(const std::string& Filename, std::size_t&, unsigned int&, unsigned int&) {
	eePRINTL( "Failed to open sound file %s, format is not supported by eepp", Filename.c_str() );
	return false;
}

bool SoundFile::OpenRead(const char*, std::size_t, std::size_t&, unsigned int&, unsigned int&) {
	eePRINTL( "Failed to open sound file from memory, format is not supported by eepp" );
	return false;
}

bool SoundFile::OpenWrite(const std::string& Filename, unsigned int, unsigned int) {
	eePRINTL( "Failed to open sound file %s, format is not supported by eepp", Filename.c_str() );
	return false;
}

std::size_t SoundFile::Read(Int16*, std::size_t) {
	eePRINTL( "Failed to read from sound file (not supported)" );
	return 0;
}

void SoundFile::Write(const Int16*, std::size_t) {
	eePRINTL( "Failed to write to sound file (not supported)" );
}

void SoundFile::Seek( cTime timeOffset ) {
	eePRINTL( "Trying to seek a file that doesn't support seeking." );
}

}}
