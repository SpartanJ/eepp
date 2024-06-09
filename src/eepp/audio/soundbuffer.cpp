#include <eepp/audio/alcheck.hpp>
#include <eepp/audio/audiodevice.hpp>
#include <eepp/audio/inputsoundfile.hpp>
#include <eepp/audio/outputsoundfile.hpp>
#include <eepp/audio/sound.hpp>
#include <eepp/audio/soundbuffer.hpp>
#include <eepp/core/debug.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/log.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/system/scopedbuffer.hpp>

namespace EE { namespace Audio {

SoundBuffer::SoundBuffer() : mBuffer( 0 ), mDuration() {
	// Create the buffer
	alCheck( alGenBuffers( 1, &mBuffer ) );
}

SoundBuffer::SoundBuffer( const SoundBuffer& copy ) :
	mBuffer( 0 ),
	mSamples( copy.mSamples ),
	mDuration( copy.mDuration ),
	mSounds() // don't copy the attached sounds
{
	// Create the buffer
	alCheck( alGenBuffers( 1, &mBuffer ) );

	// Update the internal buffer with the new samples
	update( copy.getChannelCount(), copy.getSampleRate() );
}

SoundBuffer::~SoundBuffer() {
	// To prevent the iterator from becoming invalid, move the entire buffer to another
	// container. Otherwise calling resetBuffer would result in detachSound being
	// called which removes the sound from the internal list.
	SoundList sounds;
	sounds.swap( mSounds );

	// Detach the buffer from the sounds that use it (to avoid OpenAL errors)
	for ( SoundList::const_iterator it = sounds.begin(); it != sounds.end(); ++it )
		( *it )->resetBuffer();

	// Destroy the buffer
	if ( mBuffer )
		alCheck( alDeleteBuffers( 1, &mBuffer ) );
}

bool SoundBuffer::loadFromFile( const std::string& filename ) {
	if ( !FileSystem::fileExists( filename ) ) {
		if ( PackManager::instance()->isFallbackToPacksActive() ) {
			std::string tPath( filename );

			Pack* tPack = PackManager::instance()->exists( tPath );

			if ( NULL != tPack ) {
				return loadFromPack( tPack, tPath );
			}
		}

		return false;
	}

	InputSoundFile file;
	if ( file.openFromFile( filename ) )
		return initialize( file );
	else
		return false;
}

bool SoundBuffer::loadFromMemory( const void* data, std::size_t sizeInBytes ) {
	InputSoundFile file;
	if ( file.openFromMemory( data, sizeInBytes ) )
		return initialize( file );
	else
		return false;
}

bool SoundBuffer::loadFromStream( IOStream& stream ) {
	InputSoundFile file;
	if ( file.openFromStream( stream ) )
		return initialize( file );
	else
		return false;
}

bool SoundBuffer::loadFromSamples( const Int16* samples, Uint64 sampleCount,
								   unsigned int channelCount, unsigned int sampleRate ) {
	if ( samples && sampleCount && channelCount && sampleRate ) {
		// Copy the new audio samples
		mSamples.assign( samples, samples + sampleCount );

		// Update the internal buffer with the new samples
		return update( channelCount, sampleRate );
	} else {
		// Error...
		Log::error( "Failed to load sound buffer from samples (array: %d, count: %d, channels: %d, "
					"samplerate: %d)",
					samples, sampleCount, channelCount, sampleRate );
		return false;
	}
}

bool SoundBuffer::loadFromPack( Pack* pack, std::string filePackPath ) {
	bool Ret = false;
	ScopedBuffer buffer;

	if ( pack->isOpen() && pack->extractFileToMemory( filePackPath, buffer ) )
		Ret = loadFromMemory( reinterpret_cast<const char*>( buffer.get() ), buffer.length() );

	return Ret;
}

bool SoundBuffer::saveToFile( const std::string& filename ) const {
	// Create the sound file in write mode
	OutputSoundFile file;
	if ( file.openFromFile( filename, getSampleRate(), getChannelCount() ) ) {
		// Write the samples to the opened file
		file.write( &mSamples[0], mSamples.size() );

		return true;
	} else {
		return false;
	}
}

const Int16* SoundBuffer::getSamples() const {
	return mSamples.empty() ? NULL : &mSamples[0];
}

Uint64 SoundBuffer::getSampleCount() const {
	return mSamples.size();
}

unsigned int SoundBuffer::getSampleRate() const {
	ALint sampleRate;
	alCheck( alGetBufferi( mBuffer, AL_FREQUENCY, &sampleRate ) );
	return sampleRate;
}

unsigned int SoundBuffer::getChannelCount() const {
	ALint channelCount;
	alCheck( alGetBufferi( mBuffer, AL_CHANNELS, &channelCount ) );
	return channelCount;
}

Time SoundBuffer::getDuration() const {
	return mDuration;
}

SoundBuffer& SoundBuffer::operator=( const SoundBuffer& right ) {
	SoundBuffer temp( right );

	std::swap( mSamples, temp.mSamples );
	std::swap( mBuffer, temp.mBuffer );
	std::swap( mDuration, temp.mDuration );
	std::swap( mSounds,
			   temp.mSounds ); // swap sounds too, so that they are detached when temp is destroyed

	return *this;
}

bool SoundBuffer::initialize( InputSoundFile& file ) {
	// Retrieve the sound parameters
	Uint64 sampleCount = file.getSampleCount();
	unsigned int channelCount = file.getChannelCount();
	unsigned int sampleRate = file.getSampleRate();

	// Read the samples from the provided file
	mSamples.resize( static_cast<std::size_t>( sampleCount ) );
	if ( file.read( &mSamples[0], sampleCount ) == sampleCount ) {
		// Update the internal buffer with the new samples
		return update( channelCount, sampleRate );
	} else {
		return false;
	}
}

bool SoundBuffer::update( unsigned int channelCount, unsigned int sampleRate ) {
	// Check parameters
	if ( !channelCount || !sampleRate || mSamples.empty() )
		return false;

	// Find the good format according to the number of channels
	ALenum format = Private::AudioDevice::getFormatFromChannelCount( channelCount );

	// Check if the format is valid
	if ( format == 0 ) {
		Log::error( "Failed to load sound buffer (unsupported number of channels: %d)",
					channelCount );
		return false;
	}

	// First make a copy of the list of sounds so we can reattach later
	SoundList sounds( mSounds );

	// Detach the buffer from the sounds that use it (to avoid OpenAL errors)
	for ( SoundList::const_iterator it = sounds.begin(); it != sounds.end(); ++it )
		( *it )->resetBuffer();

	// Fill the buffer
	ALsizei size = static_cast<ALsizei>( mSamples.size() ) * sizeof( Int16 );
	alCheck( alBufferData( mBuffer, format, &mSamples[0], size, sampleRate ) );

	// Compute the duration
	mDuration = Seconds( static_cast<float>( mSamples.size() ) / sampleRate / channelCount );

	// Now reattach the buffer to the sounds that use it
	for ( SoundList::const_iterator it = sounds.begin(); it != sounds.end(); ++it )
		( *it )->setBuffer( *this );

	return true;
}

void SoundBuffer::attachSound( Sound* sound ) const {
	mSounds.insert( sound );
}

void SoundBuffer::detachSound( Sound* sound ) const {
	mSounds.erase( sound );
}

}} // namespace EE::Audio
