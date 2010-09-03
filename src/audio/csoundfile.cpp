#include "csoundfile.hpp"
#ifndef EE_NO_SNDFILE
#include "csoundfiledefault.hpp"
#endif
#include "csoundfileogg.hpp"

namespace EE { namespace Audio {

cSoundFile::cSoundFile() :
	mNbSamples (0),
	mChannelsCount(0),
	mSampleRate (0)
{
}

cSoundFile::~cSoundFile()
{
}

cSoundFile * cSoundFile::CreateRead( const std::string& Filename ) {
	// Create the file according to its type
	cSoundFile * File = NULL;

	if 			( cSoundFileOgg::IsFileSupported(Filename, true) )	 	File = eeNew( cSoundFileOgg, () );
	#ifndef EE_NO_SNDFILE
	else if 	( cSoundFileDefault::IsFileSupported(Filename, true) ) 	File = eeNew( cSoundFileDefault, () );
	#endif

	// Open it for reading
	if ( NULL != File ) {
		std::size_t  SamplesCount;
		unsigned int ChannelsCount;
		unsigned int SampleRate;

		if ( File->OpenRead( Filename, SamplesCount, ChannelsCount, SampleRate ) ) {
			File->mFilename			= Filename;
			File->mData				= NULL;
			File->mSize				= 0;
			File->mNbSamples		= SamplesCount;
			File->mChannelsCount	= ChannelsCount;
			File->mSampleRate		= SampleRate;
		} else {
			eeDelete( File );
			File = NULL;
		}
	}

	return File;
}

cSoundFile * cSoundFile::CreateRead( const char* Data, std::size_t SizeInMemory ) {
	// Create the file according to its type
	cSoundFile * File = NULL;

	if			( cSoundFileOgg::IsFileSupported(Data, SizeInMemory))	 	File = eeNew( cSoundFileOgg, () );
	#ifndef EE_NO_SNDFILE
	else if 	( cSoundFileDefault::IsFileSupported(Data, SizeInMemory) ) 	File = eeNew( cSoundFileDefault, () );
	#endif

	// Open it for reading
	if ( NULL != File ) {
		std::size_t  SamplesCount;
		unsigned int ChannelsCount;
		unsigned int SampleRate;

		if ( File->OpenRead(Data, SizeInMemory, SamplesCount, ChannelsCount, SampleRate) ) {
			File->mFilename	  = "";
			File->mData		  = Data;
			File->mSize		  = SizeInMemory;
			File->mNbSamples	 = SamplesCount;
			File->mChannelsCount = ChannelsCount;
			File->mSampleRate	= SampleRate;
		} else {
			eeDelete( File );
			File = NULL;
		}
	}

	return File;
}

cSoundFile * cSoundFile::CreateWrite( const std::string& Filename, unsigned int ChannelsCount, unsigned int SampleRate ) {
	// Create the file according to its type
	cSoundFile * File = NULL;

	if			( cSoundFileOgg::IsFileSupported(Filename, false) )		File = eeNew( cSoundFileOgg, () );
	#ifndef EE_NO_SNDFILE
	else if	( cSoundFileDefault::IsFileSupported(Filename, false) )	File = eeNew( cSoundFileDefault, () );
	#endif

	// Open it for writing
	if ( NULL != File ) {
		if ( File->OpenWrite(Filename, ChannelsCount, SampleRate) ) {
			File->mFilename	  = "";
			File->mData		  = NULL;
			File->mSize		  = 0;
			File->mNbSamples	 = 0;
			File->mChannelsCount = ChannelsCount;
			File->mSampleRate	= SampleRate;
		} else {
			eeDelete( File );
			File = NULL;
		}
	}

	return File;
}

std::size_t cSoundFile::GetSamplesCount() const
{
	return mNbSamples;
}

unsigned int cSoundFile::GetChannelsCount() const {
	return mChannelsCount;
}

unsigned int cSoundFile::GetSampleRate() const {
	return mSampleRate;
}

bool cSoundFile::Restart() {
	if ( mData ) {
		// Reopen from memory
		return OpenRead(mData, mSize, mNbSamples, mChannelsCount, mSampleRate);
	} else if ( mFilename != "" ) {
		// Reopen from file
		return OpenRead(mFilename, mNbSamples, mChannelsCount, mSampleRate);
	} else {
		cLog::instance()->Write( "Warning : trying to restart a sound opened in write mode, which is not allowed" );
		return false;
	}
}

bool cSoundFile::OpenRead(const std::string& Filename, std::size_t&, unsigned int&, unsigned int&) {
	cLog::instance()->Write( "Failed to open sound file \"" + Filename + "\", format is not supported by EE" );
	return false;
}

bool cSoundFile::OpenRead(const char*, std::size_t, std::size_t&, unsigned int&, unsigned int&) {
	cLog::instance()->Write( "Failed to open sound file from memory, format is not supported by EE" );
	return false;
}

bool cSoundFile::OpenWrite(const std::string& Filename, unsigned int, unsigned int) {
	cLog::instance()->Write( "Failed to open sound file \"" + Filename + "\", format is not supported by EE");
	return false;
}

std::size_t cSoundFile::Read(Int16*, std::size_t) {
	cLog::instance()->Write( "Failed to read from sound file (not supported)" );
	return 0;
}

void cSoundFile::Write(const Int16*, std::size_t) {
	cLog::instance()->Write( "Failed to write to sound file (not supported)" );
}

void cSoundFile::Seek( float timeOffset ) {
	cLog::instance()->Write( "Trying to seek a file that doesn't support seeking." );
}

}}
