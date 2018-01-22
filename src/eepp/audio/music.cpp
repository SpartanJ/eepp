#include <eepp/audio/music.hpp>
#include <eepp/audio/soundfile.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/filesystem.hpp>

namespace EE { namespace Audio {

Music::Music( std::size_t BufferSize ) :
	mFile ( NULL ),
	mDuration( 0.f ),
	mSamples( BufferSize )
{
}

Music::~Music() {
	stop();
	eeSAFE_DELETE( mFile );
}

bool Music::openFromPack( Pack* Pack, const std::string& FilePackPath ) {
	if ( Pack->isOpen() && Pack->extractFileToMemory( FilePackPath, mData ) )
		return openFromMemory( reinterpret_cast<const char*> ( mData.data ), mData.size );

	return false;
}

bool Music::openFromFile( const std::string& Filename ) {
	if ( !FileSystem::fileExists( Filename ) ) {
		if ( PackManager::instance()->isFallbackToPacksActive() ) {
			std::string tPath( Filename );

			Pack * tPack = PackManager::instance()->exists( tPath );

			if ( NULL != tPack ) {
				return openFromPack( tPack, tPath );
			}
		}

		return false;
	}

	// Create the sound file implementation, and open it in read mode
	stop();
	eeSAFE_DELETE( mFile );

	mFile = SoundFile::createRead( Filename );

	if ( NULL == mFile ) {
		eePRINTL( "Failed to open %s for reading", Filename.c_str() );
		return false;
	}

	// Compute the duration
	mDuration = static_cast<float>( mFile->getSamplesCount() ) / mFile->getSampleRate() / mFile->getChannelCount();

	// Initialize the stream
	initialize( mFile->getChannelCount(), mFile->getSampleRate() );

	eePRINTL( "Music file %s loaded.", Filename.c_str() );

	return true;
}

bool Music::openFromMemory( const char * Data, std::size_t SizeInBytes ) {
	stop();
	eeSAFE_DELETE( mFile );

	// Create the sound file implementation, and open it in read mode
	mFile = SoundFile::createRead( Data, SizeInBytes );

	if ( NULL == mFile ) {
		eePRINTL( "Failed to open music from memory for reading" );
		return false;
	}

	mDuration = static_cast<float>( mFile->getSamplesCount() ) / mFile->getSampleRate(); // Compute the duration

	initialize( mFile->getChannelCount(), mFile->getSampleRate() ); // Initialize the stream

	eePRINTL( "Music file loaded from memory." );

	return true;
}

bool Music::onStart() {
	return NULL != mFile && mFile->restart();
}

bool Music::onGetData( SoundStream::Chunk& Data ) {
	if ( NULL != mFile ) {
		// Fill the chunk parameters
		Data.Samples   = &mSamples[0];
		Data.SamplesCount = mFile->read( &mSamples[0], mSamples.size() );

		// Check if we have reached the end of the audio file
		return Data.SamplesCount == mSamples.size();
	}

	return false;
}

Time Music::getDuration() const {
	return Seconds( mDuration );
}

void Music::onSeek( Time timeOffset ) {
	if ( NULL != mFile ) {
		mFile->seek( timeOffset );
	}
}

}}
