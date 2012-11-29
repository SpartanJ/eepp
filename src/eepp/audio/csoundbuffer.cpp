#include <eepp/audio/csoundbuffer.hpp>
#include <eepp/audio/csound.hpp>
#include <eepp/audio/caudiodevice.hpp>
#include <eepp/system/cpackmanager.hpp>
#include <memory>

namespace EE { namespace Audio {

cSoundBuffer::cSoundBuffer() :
	mBuffer(0),
	mDuration(0.f)
{
	EnsureALInit();

	ALCheck( alGenBuffers( 1, &mBuffer ) );
}

cSoundBuffer::cSoundBuffer(const cSoundBuffer& Copy) :
	cAudioResource(Copy),
	mBuffer(0),
	mSamples(Copy.mSamples),
	mDuration(Copy.mDuration),
	mSounds()
{
	EnsureALInit();

	ALCheck( alGenBuffers( 1, &mBuffer ) );
	Update( Copy.GetChannelsCount(), Copy.GetSampleRate() );
}

cSoundBuffer::~cSoundBuffer() {
	for ( SoundList::const_iterator it = mSounds.begin(); it != mSounds.end(); ++it )
		(*it)->ResetBuffer();

	if ( mBuffer )
		ALCheck( alDeleteBuffers( 1, &mBuffer ) );
}

bool cSoundBuffer::LoadFromFile(const std::string& Filename) {
	if ( !FileSystem::FileExists( Filename ) ) {
		if ( cPackManager::instance()->FallbackToPacks() ) {
			std::string tPath( Filename );

			cPack * tPack = cPackManager::instance()->Exists( tPath );

			if ( NULL != tPack ) {
				return LoadFromPack( tPack, tPath );
			}
		}

		cLog::instance()->Write( "Failed to load sound buffer from file \"" + Filename + "\"" );
		return false;
	}

	// Create the sound file
	cSoundFile * File = cSoundFile::CreateRead( Filename );

	// Open the sound file
	if ( NULL != File ) {
		// Get the sound parameters
		std::size_t  NbSamples		= File->GetSamplesCount();
		unsigned int ChannelsCount	= File->GetChannelsCount();
		unsigned int SampleRate	= File->GetSampleRate();

		// Read the samples from the opened file
		mSamples.resize( NbSamples );

		if ( File->Read( &mSamples[0], NbSamples ) == NbSamples ) {
			cLog::instance()->Write( "Sound file " + Filename + " loaded." );

			// Update the internal buffer with the new samples
			eeDelete( File );

			return Update( ChannelsCount, SampleRate );
		} else {
			cLog::instance()->Write( "Failed to read audio data from file \"" + Filename + "\"" );

			eeDelete( File );

			return false;
		}
	} else {
		cLog::instance()->Write( "Failed to load sound buffer from file \"" + Filename + "\"" );

		return false;
	}
}

bool cSoundBuffer::LoadFromPack( cPack* Pack, const std::string& FilePackPath ) {
	bool Ret = false;
	SafeDataPointer PData;

	if ( Pack->IsOpen() && Pack->ExtractFileToMemory( FilePackPath, PData ) )
		Ret = LoadFromMemory( reinterpret_cast<const char*> ( PData.Data ), PData.DataSize );

	return Ret;
}

bool cSoundBuffer::LoadFromMemory( const char* Data, std::size_t SizeInBytes ) {
	// Create the sound file
	cSoundFile * File = cSoundFile::CreateRead( Data, SizeInBytes );

	// Open the sound file
	if ( NULL != File ) {
		// Get the sound parameters
		std::size_t  NbSamples		= File->GetSamplesCount();
		unsigned int ChannelsCount	= File->GetChannelsCount();
		unsigned int SampleRate	= File->GetSampleRate();

		// Read the samples from the opened file
		mSamples.resize( NbSamples );

		if ( File->Read( &mSamples[0], NbSamples ) == NbSamples ) {
			cLog::instance()->Write( "Sound file loaded from memory." );

			// Update the internal buffer with the new samples
			eeDelete( File );

			return Update( ChannelsCount, SampleRate );
		} else {
			cLog::instance()->Write( "Failed to read audio data from file in memory" );

			eeDelete( File );

			return false;
		}
	} else {
		cLog::instance()->Write( "Failed to load sound buffer from file in memory" );
		return false;
	}
}

bool cSoundBuffer::LoadFromSamples( const Int16 * Samples, std::size_t SamplesCount, unsigned int ChannelsCount, unsigned int SampleRate ) {
	if ( Samples && SamplesCount && ChannelsCount && SampleRate ) {
		// Copy the new audio samples
		mSamples.assign( Samples, Samples + SamplesCount );

		cLog::instance()->Write( "Sound file loaded from memory samples." );

		// Update the internal buffer with the new samples
		return Update( ChannelsCount, SampleRate );
	} else {
		// Error...
		cLog::instance()->Write( "Failed to load sound buffer from memory Samples" );
		return false;
	}
}

bool cSoundBuffer::SaveToFile(const std::string& Filename) const {
	// Create the sound file in write mode
	std::auto_ptr<cSoundFile> File( cSoundFile::CreateWrite( Filename, GetChannelsCount(), GetSampleRate() ) );

	if ( File.get() ) {
		// Write the samples to the opened file
		File->Write( &mSamples[0], mSamples.size() );
		return true;
	} else {
		// Error...
		cLog::instance()->Write( "Failed to save sound buffer to file \"" + Filename + "\"" );
		return false;
	}
}

const Int16* cSoundBuffer::GetSamples() const {
	return mSamples.empty() ? NULL : &mSamples[0];
}

std::size_t cSoundBuffer::GetSamplesCount() const {
	return mSamples.size();
}

unsigned int cSoundBuffer::GetSampleRate() const {
	ALint SampleRate;
	ALCheck( alGetBufferi( mBuffer, AL_FREQUENCY, &SampleRate ) );
	return SampleRate;
}

unsigned int cSoundBuffer::GetChannelsCount() const {
	ALint ChannelsCount;
	ALCheck( alGetBufferi( mBuffer, AL_CHANNELS, &ChannelsCount ) );
	return ChannelsCount;
}

Uint32 cSoundBuffer::GetDuration() const {
	return mDuration;
}

cSoundBuffer& cSoundBuffer::operator =( const cSoundBuffer& Other ) {
	cSoundBuffer Temp( Other );

	std::swap( mSamples	,	Temp.mSamples );
	std::swap( mBuffer	,	Temp.mBuffer );
	std::swap( mDuration,	Temp.mDuration );
	std::swap( mSounds	,	Temp.mSounds );

	return *this;
}

bool cSoundBuffer::Update( unsigned int ChannelsCount, unsigned int SampleRate ) {
	// Check parameters
	if ( !SampleRate || !ChannelsCount || mSamples.empty() )
		return false;

	// Find the good format according to the number of channels
	ALenum Format = cAudioDevice::GetFormatFromChannelsCount( ChannelsCount );

	// Check if the format is valid
	if ( Format == 0 ) {
		cLog::instance()->Write( "Unsupported number of channels." );
		return false;
	}

	// Fill the buffer
	ALsizei Size = static_cast<ALsizei>( mSamples.size() ) * sizeof(Int16);
	ALCheck( alBufferData( mBuffer, Format, &mSamples[0], Size, SampleRate ) );

	// Compute the duration
	mDuration = static_cast<Uint32>( 1000 * mSamples.size() ) / SampleRate / ChannelsCount;

	return true;
}

void cSoundBuffer::AttachSound( cSound* sound ) const {
	mSounds.insert(sound);
}

void cSoundBuffer::DetachSound( cSound* sound ) const {
	mSounds.erase(sound);
}

}}
