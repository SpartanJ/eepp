#include "cmusic.hpp"

namespace EE { namespace Audio {

cMusic::cMusic(std::size_t BufferSize) :
	mFile (NULL),
	mDuration(0.f),
	mSamples (BufferSize)
{
}

cMusic::~cMusic() {
	Stop();
	eeSAFE_DELETE( mFile );
}

bool cMusic::OpenFromPack( cPack* Pack, const std::string& FilePackPath ) {
	if ( Pack->IsOpen() && Pack->ExtractFileToMemory( FilePackPath, mData ) )
		return OpenFromMemory( reinterpret_cast<const char*> (&mData[0]), mData.size() );

	return false;
}

bool cMusic::OpenFromFile(const std::string& Filename) {
	// Create the sound file implementation, and open it in read mode
	Stop();
	eeSAFE_DELETE( mFile );

	mFile = cSoundFile::CreateRead( Filename );

	if ( NULL == mFile ) {
		cLog::instance()->Write( "Failed to open \"" + Filename + "\" for reading" );
		return false;
	}

	// Compute the duration
	mDuration = static_cast<eeFloat>(mFile->GetSamplesCount()) / mFile->GetSampleRate() / mFile->GetChannelsCount();

	// Initialize the stream
	Initialize(mFile->GetChannelsCount(), mFile->GetSampleRate());

	cLog::instance()->Write( "Music file " + Filename + " loaded." );
	return true;
}

bool cMusic::OpenFromMemory(const char* Data, std::size_t SizeInBytes) {
	Stop();
	eeSAFE_DELETE( mFile );

	// Create the sound file implementation, and open it in read mode
	mFile = cSoundFile::CreateRead( Data, SizeInBytes );

	if ( NULL == mFile ) {
		cLog::instance()->Write( "Failed to open music from memory for reading" );
		return false;
	}

	mDuration = static_cast<eeFloat>(mFile->GetSamplesCount()) / mFile->GetSampleRate(); // Compute the duration

	Initialize(mFile->GetChannelsCount(), mFile->GetSampleRate()); // Initialize the stream

	cLog::instance()->Write( "Music file loaded from memory." );
	return true;
}

bool cMusic::OnStart() {
	return NULL != mFile && mFile->Restart();
}

bool cMusic::OnGetData( cSoundStream::Chunk& Data ) {
	if ( NULL != mFile ) {
		// Fill the chunk parameters
		Data.Samples   = &mSamples[0];
		Data.NbSamples = mFile->Read( &mSamples[0], mSamples.size() );

		// Check if we have reached the end of the audio file
		return Data.NbSamples == mSamples.size();
	}

	return false;
}

eeFloat cMusic::GetDuration() const {
	return mDuration;
}

void cMusic::OnSeek( float timeOffset ) {
	if ( NULL != mFile ) {
		mFile->Seek( timeOffset );
	}
}

}}
