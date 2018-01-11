#include <eepp/audio/soundbuffer.hpp>
#include <eepp/audio/sound.hpp>
#include <eepp/audio/soundfile.hpp>
#include <eepp/audio/audiodevice.hpp>
#include <eepp/audio/openal.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/filesystem.hpp>
#include <memory>

namespace EE { namespace Audio {

SoundBuffer::SoundBuffer() :
	mBuffer(0),
	mDuration()
{
	EnsureALInit();

	ALCheck( alGenBuffers( 1, &mBuffer ) );
}

SoundBuffer::SoundBuffer(const SoundBuffer& Copy) :
	mBuffer(0),
	mSamples(Copy.mSamples),
	mDuration(Copy.mDuration),
	mSounds()
{
	EnsureALInit();

	ALCheck( alGenBuffers( 1, &mBuffer ) );
	update( Copy.getChannelCount(), Copy.getSampleRate() );
}

SoundBuffer::~SoundBuffer() {
	for ( SoundList::const_iterator it = mSounds.begin(); it != mSounds.end(); ++it )
		(*it)->resetBuffer();

	if ( mBuffer )
		ALCheck( alDeleteBuffers( 1, &mBuffer ) );
}

bool SoundBuffer::loadFromFile(const std::string& Filename) {
	if ( !FileSystem::fileExists( Filename ) ) {
		if ( PackManager::instance()->isFallbackToPacksActive() ) {
			std::string tPath( Filename );

			Pack * tPack = PackManager::instance()->exists( tPath );

			if ( NULL != tPack ) {
				return loadFromPack( tPack, tPath );
			}
		}

		eePRINTL( "Failed to load sound buffer from file %s", Filename.c_str() );
		return false;
	}

	// Create the sound file
	SoundFile * File = SoundFile::createRead( Filename );

	// Open the sound file
	if ( NULL != File ) {
		// Get the sound parameters
		std::size_t  SamplesCount	= File->getSamplesCount();
		unsigned int ChannelCount	= File->getChannelCount();
		unsigned int SampleRate		= File->getSampleRate();

		// Read the samples from the opened file
		mSamples.resize( SamplesCount );

		if ( File->read( &mSamples[0], SamplesCount ) == SamplesCount ) {
			eePRINTL( "Sound file %s loaded.", Filename.c_str() );

			// Update the internal buffer with the new samples
			eeDelete( File );

			return update( ChannelCount, SampleRate );
		} else {
			eePRINTL( "Failed to read audio data from file %s", Filename.c_str() );

			eeDelete( File );

			return false;
		}
	} else {
		eePRINTL( "Failed to load sound buffer from file %s", Filename.c_str() );

		return false;
	}
}

bool SoundBuffer::loadFromPack( Pack* Pack, const std::string& FilePackPath ) {
	bool Ret = false;
	SafeDataPointer PData;

	if ( Pack->isOpen() && Pack->extractFileToMemory( FilePackPath, PData ) )
		Ret = loadFromMemory( reinterpret_cast<const char*> ( PData.Data ), PData.DataSize );

	return Ret;
}

bool SoundBuffer::loadFromMemory( const char* Data, std::size_t SizeInBytes ) {
	// Create the sound file
	SoundFile * File = SoundFile::createRead( Data, SizeInBytes );

	// Open the sound file
	if ( NULL != File ) {
		// Get the sound parameters
		std::size_t  SamplesCount	= File->getSamplesCount();
		unsigned int ChannelCount	= File->getChannelCount();
		unsigned int SampleRate		= File->getSampleRate();

		// Read the samples from the opened file
		mSamples.resize( SamplesCount );

		if ( File->read( &mSamples[0], SamplesCount ) == SamplesCount ) {
			eePRINTL( "Sound file loaded from memory." );

			// Update the internal buffer with the new samples
			eeDelete( File );

			return update( ChannelCount, SampleRate );
		} else {
			eePRINTL( "Failed to read audio data from file in memory" );

			eeDelete( File );

			return false;
		}
	} else {
		eePRINTL( "Failed to load sound buffer from file in memory" );
		return false;
	}
}

bool SoundBuffer::loadFromSamples( const Int16 * Samples, std::size_t SamplesCount, unsigned int ChannelCount, unsigned int SampleRate ) {
	if ( Samples && SamplesCount && ChannelCount && SampleRate ) {
		// Copy the new audio samples
		mSamples.assign( Samples, Samples + SamplesCount );

		eePRINTL( "Sound file loaded from memory samples." );

		// Update the internal buffer with the new samples
		return update( ChannelCount, SampleRate );
	} else {
		// Error...
		eePRINTL( "Failed to load sound buffer from memory Samples" );
		return false;
	}
}

bool SoundBuffer::saveToFile(const std::string& Filename) const {
	// Create the sound file in write mode
	SoundFile * File = SoundFile::createWrite( Filename, getChannelCount(), getSampleRate() );

	if ( NULL != File ) {
		// Write the samples to the opened file
		File->write( &mSamples[0], mSamples.size() );

		eeSAFE_DELETE( File );

		return true;
	} else {
		// Error...
		eePRINTL( "Failed to save sound buffer to file %s", Filename.c_str() );
		return false;
	}
}

const Int16* SoundBuffer::getSamples() const {
	return mSamples.empty() ? NULL : &mSamples[0];
}

std::size_t SoundBuffer::getSamplesCount() const {
	return mSamples.size();
}

unsigned int SoundBuffer::getSampleRate() const {
	ALint SampleRate;
	ALCheck( alGetBufferi( mBuffer, AL_FREQUENCY, &SampleRate ) );
	return SampleRate;
}

unsigned int SoundBuffer::getChannelCount() const {
	ALint ChannelCount;
	ALCheck( alGetBufferi( mBuffer, AL_CHANNELS, &ChannelCount ) );
	return ChannelCount;
}

Time SoundBuffer::getDuration() const {
	return mDuration;
}

SoundBuffer& SoundBuffer::operator =( const SoundBuffer& Other ) {
	SoundBuffer Temp( Other );

	std::swap( mSamples	,	Temp.mSamples );
	std::swap( mBuffer	,	Temp.mBuffer );
	std::swap( mDuration,	Temp.mDuration );
	std::swap( mSounds	,	Temp.mSounds );

	return *this;
}

bool SoundBuffer::update( unsigned int ChannelCount, unsigned int SampleRate ) {
	// Check parameters
	if ( !SampleRate || !ChannelCount || mSamples.empty() )
		return false;

	// Find the good format according to the number of channels
	ALenum Format = AudioDevice::getFormatFromChannelCount( ChannelCount );

	// Check if the format is valid
	if ( Format == 0 ) {
		eePRINTL( "Unsupported number of channels." );
		return false;
	}

	// First make a copy of the list of sounds so we can reattach later
	SoundList sounds( mSounds );

	// Detach the buffer from the sounds that use it (to avoid OpenAL errors)
	for (SoundList::const_iterator it = sounds.begin(); it != sounds.end(); ++it)
		(*it)->resetBuffer();

	// Fill the buffer
	ALsizei Size = static_cast<ALsizei>( mSamples.size() ) * sizeof(Int16);
	ALCheck( alBufferData( mBuffer, Format, &mSamples[0], Size, SampleRate ) );

	// Compute the duration
	mDuration = Seconds(static_cast<float>(mSamples.size()) / SampleRate / ChannelCount);

	return true;
}

void SoundBuffer::attachSound( Sound* sound ) const {
	mSounds.insert(sound);
}

void SoundBuffer::detachSound( Sound* sound ) const {
	mSounds.erase(sound);
}

}}
