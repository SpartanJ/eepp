#include "csoundfile.hpp"
#include "csoundfiledefault.hpp"
#include "csoundfileogg.hpp"

namespace EE { namespace Audio {

cSoundFile* cSoundFile::CreateRead(const std::string& Filename) {
	// Create the file according to its type
	cSoundFile* File = NULL;
	if	  (cSoundFileOgg::IsFileSupported(Filename, true))	 File = new cSoundFileOgg;
	else if (cSoundFileDefault::IsFileSupported(Filename, true)) File = new cSoundFileDefault;

	// Open it for reading
	if (File) {
		std::size_t  SamplesCount;
		unsigned int ChannelsCount;
		unsigned int SampleRate;

		if (File->OpenRead(Filename, SamplesCount, ChannelsCount, SampleRate)) {
			File->myFilename	  = Filename;
			File->myData		  = NULL;
			File->mySize		  = 0;
			File->myNbSamples	 = SamplesCount;
			File->myChannelsCount = ChannelsCount;
			File->mySampleRate	= SampleRate;
		} else {
			delete File;
			File = NULL;
		}
	}
	return File;
}

cSoundFile* cSoundFile::CreateRead(const char* Data, std::size_t SizeInMemory) {
	// Create the file according to its type
	cSoundFile* File = NULL;
	if	  (cSoundFileOgg::IsFileSupported(Data, SizeInMemory))	 File = new cSoundFileOgg;
	else if (cSoundFileDefault::IsFileSupported(Data, SizeInMemory)) File = new cSoundFileDefault;

	// Open it for reading
	if (File) {
		std::size_t  SamplesCount;
		unsigned int ChannelsCount;
		unsigned int SampleRate;

		if (File->OpenRead(Data, SizeInMemory, SamplesCount, ChannelsCount, SampleRate)) {
			File->myFilename	  = "";
			File->myData		  = Data;
			File->mySize		  = SizeInMemory;
			File->myNbSamples	 = SamplesCount;
			File->myChannelsCount = ChannelsCount;
			File->mySampleRate	= SampleRate;
		} else {
			delete File;
			File = NULL;
		}
	}
	return File;
}

cSoundFile* cSoundFile::CreateWrite(const std::string& Filename, unsigned int ChannelsCount, unsigned int SampleRate) {
	// Create the file according to its type
	cSoundFile* File = NULL;
	if	  (cSoundFileOgg::IsFileSupported(Filename, false))	 File = new cSoundFileOgg;
	else if (cSoundFileDefault::IsFileSupported(Filename, false)) File = new cSoundFileDefault;

	// Open it for writing
	if (File) {
		if (File->OpenWrite(Filename, ChannelsCount, SampleRate)) {
			File->myFilename	  = "";
			File->myData		  = NULL;
			File->mySize		  = 0;
			File->myNbSamples	 = 0;
			File->myChannelsCount = ChannelsCount;
			File->mySampleRate	= SampleRate;
		} else {
			delete File;
			File = NULL;
		}
	}
	return File;
}

cSoundFile::cSoundFile() : myNbSamples (0), myChannelsCount(0), mySampleRate (0) {}
cSoundFile::~cSoundFile() {}

std::size_t cSoundFile::GetSamplesCount() const
{
	return myNbSamples;
}

unsigned int cSoundFile::GetChannelsCount() const {
	return myChannelsCount;
}

unsigned int cSoundFile::GetSampleRate() const {
	return mySampleRate;
}

bool cSoundFile::Restart() {
	if (myData) {
		// Reopen from memory
		return OpenRead(myData, mySize, myNbSamples, myChannelsCount, mySampleRate);
	} else if (myFilename != "") {
		// Reopen from file
		return OpenRead(myFilename, myNbSamples, myChannelsCount, mySampleRate);
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

}}
