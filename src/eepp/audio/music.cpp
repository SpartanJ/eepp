#include <eepp/audio/music.hpp>
#include <eepp/audio/soundfile.hpp>
#include <eepp/system/packmanager.hpp>

namespace EE { namespace Audio {

Music::Music( std::size_t BufferSize ) :
	mFile ( NULL ),
	mDuration( 0.f ),
	mSamples( BufferSize )
{
}

Music::~Music() {
	Stop();
	eeSAFE_DELETE( mFile );
}

bool Music::OpenFromPack( Pack* Pack, const std::string& FilePackPath ) {
	if ( Pack->isOpen() && Pack->extractFileToMemory( FilePackPath, mData ) )
		return OpenFromMemory( reinterpret_cast<const char*> ( mData.Data ), mData.DataSize );

	return false;
}

bool Music::OpenFromFile( const std::string& Filename ) {
	if ( !FileSystem::fileExists( Filename ) ) {
		if ( PackManager::instance()->fallbackToPacks() ) {
			std::string tPath( Filename );

			Pack * tPack = PackManager::instance()->exists( tPath );

			if ( NULL != tPack ) {
				return OpenFromPack( tPack, tPath );
			}
		}

		return false;
	}

	// Create the sound file implementation, and open it in read mode
	Stop();
	eeSAFE_DELETE( mFile );

	mFile = SoundFile::CreateRead( Filename );

	if ( NULL == mFile ) {
		eePRINTL( "Failed to open %s for reading", Filename.c_str() );
		return false;
	}

	// Compute the duration
	mDuration = static_cast<float>( mFile->GetSamplesCount() ) / mFile->GetSampleRate() / mFile->GetChannelCount();

	// Initialize the stream
	Initialize( mFile->GetChannelCount(), mFile->GetSampleRate() );

	eePRINTL( "Music file %s loaded.", Filename.c_str() );

	return true;
}

bool Music::OpenFromMemory( const char * Data, std::size_t SizeInBytes ) {
	Stop();
	eeSAFE_DELETE( mFile );

	// Create the sound file implementation, and open it in read mode
	mFile = SoundFile::CreateRead( Data, SizeInBytes );

	if ( NULL == mFile ) {
		eePRINTL( "Failed to open music from memory for reading" );
		return false;
	}

	mDuration = static_cast<float>( mFile->GetSamplesCount() ) / mFile->GetSampleRate(); // Compute the duration

	Initialize( mFile->GetChannelCount(), mFile->GetSampleRate() ); // Initialize the stream

	eePRINTL( "Music file loaded from memory." );

	return true;
}

bool Music::OnStart() {
	return NULL != mFile && mFile->Restart();
}

bool Music::OnGetData( SoundStream::Chunk& Data ) {
	if ( NULL != mFile ) {
		// Fill the chunk parameters
		Data.Samples   = &mSamples[0];
		Data.SamplesCount = mFile->Read( &mSamples[0], mSamples.size() );

		// Check if we have reached the end of the audio file
		return Data.SamplesCount == mSamples.size();
	}

	return false;
}

Time Music::GetDuration() const {
	return Seconds( mDuration );
}

void Music::OnSeek( Time timeOffset ) {
	if ( NULL != mFile ) {
		mFile->Seek( timeOffset );
	}
}

}}
