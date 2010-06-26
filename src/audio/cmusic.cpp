#include "cmusic.hpp"

namespace EE { namespace Audio {

cMusic::cMusic(std::size_t BufferSize) : myFile (NULL), myDuration(0.f), mySamples (BufferSize) {}

cMusic::~cMusic() {
	Stop();
	delete myFile;
}

bool cMusic::OpenFromPack( cPack* Pack, const std::string& FilePackPath ) {
	if ( Pack->IsOpen() && Pack->ExtractFileToMemory( FilePackPath, myData ) )
		return OpenFromMemory( reinterpret_cast<const char*> (&myData[0]), myData.size() );
	
	return false;
}

bool cMusic::OpenFromFile(const std::string& Filename) {
	// Create the sound file implementation, and open it in read mode
	Stop();
	delete myFile;
	
	myFile = cSoundFile::CreateRead(Filename);
	if (!myFile) {
		cLog::instance()->Write( "Failed to open \"" + Filename + "\" for reading" );
		return false;
	}
	
	// Compute the duration
	myDuration = static_cast<eeFloat>(myFile->GetSamplesCount()) / myFile->GetSampleRate() / myFile->GetChannelsCount();
	
	// Initialize the stream
	Initialize(myFile->GetChannelsCount(), myFile->GetSampleRate());
	
	cLog::instance()->Write( "Music file " + Filename + " loaded." );
	return true;
}

bool cMusic::OpenFromMemory(const char* Data, std::size_t SizeInBytes) {
	Stop();
	delete myFile;
	
	// Create the sound file implementation, and open it in read mode
	myFile = cSoundFile::CreateRead(Data, SizeInBytes);
	if (!myFile) {
		cLog::instance()->Write( "Failed to open music from memory for reading" );
		return false;
	}
	
	myDuration = static_cast<eeFloat>(myFile->GetSamplesCount()) / myFile->GetSampleRate(); // Compute the duration

	Initialize(myFile->GetChannelsCount(), myFile->GetSampleRate()); // Initialize the stream
	
	cLog::instance()->Write( "Music file loaded from memory." );
	return true;
}

bool cMusic::OnStart() {
	return myFile && myFile->Restart();
}

bool cMusic::OnGetData(cSoundStream::Chunk& Data) {
	if ( myFile ) {
		// Fill the chunk parameters
		Data.Samples   = &mySamples[0];
		Data.NbSamples = myFile->Read(&mySamples[0], mySamples.size());
		
		// Check if we have reached the end of the audio file
		return Data.NbSamples == mySamples.size();
	}
	return false;
}

eeFloat cMusic::GetDuration() const {
	return myDuration;
}

}}
