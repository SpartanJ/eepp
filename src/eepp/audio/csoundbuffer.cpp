#include <eepp/audio/csoundbuffer.hpp>
#include <eepp/audio/csound.hpp>
#include <eepp/audio/csoundfile.hpp>
#include <eepp/audio/caudiodevice.hpp>
#include <eepp/system/cpackmanager.hpp>
#include <eepp/audio/openal.hpp>
#include <memory>

namespace EE { namespace Audio {

cSoundBuffer::cSoundBuffer() :
	mBuffer(0),
	mDuration()
{
	EnsureALInit();

	ALCheck( alGenBuffers( 1, &mBuffer ) );
}

cSoundBuffer::cSoundBuffer(const cSoundBuffer& Copy) :
	mBuffer(0),
	mSamples(Copy.mSamples),
	mDuration(Copy.mDuration),
	mSounds()
{
	EnsureALInit();

	ALCheck( alGenBuffers( 1, &mBuffer ) );
	Update( Copy.GetChannelCount(), Copy.GetSampleRate() );
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

		eePRINTL( "Failed to load sound buffer from file %s", Filename.c_str() );
		return false;
	}

	// Create the sound file
	cSoundFile * File = cSoundFile::CreateRead( Filename );

	// Open the sound file
	if ( NULL != File ) {
		// Get the sound parameters
		std::size_t  SamplesCount	= File->GetSamplesCount();
		unsigned int ChannelCount	= File->GetChannelCount();
		unsigned int SampleRate		= File->GetSampleRate();

		// Read the samples from the opened file
		mSamples.resize( SamplesCount );

		if ( File->Read( &mSamples[0], SamplesCount ) == SamplesCount ) {
			eePRINTL( "Sound file %s loaded.", Filename.c_str() );

			// Update the internal buffer with the new samples
			eeDelete( File );

			return Update( ChannelCount, SampleRate );
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
		std::size_t  SamplesCount	= File->GetSamplesCount();
		unsigned int ChannelCount	= File->GetChannelCount();
		unsigned int SampleRate		= File->GetSampleRate();

		// Read the samples from the opened file
		mSamples.resize( SamplesCount );

		if ( File->Read( &mSamples[0], SamplesCount ) == SamplesCount ) {
			eePRINTL( "Sound file loaded from memory." );

			// Update the internal buffer with the new samples
			eeDelete( File );

			return Update( ChannelCount, SampleRate );
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

bool cSoundBuffer::LoadFromSamples( const Int16 * Samples, std::size_t SamplesCount, unsigned int ChannelCount, unsigned int SampleRate ) {
	if ( Samples && SamplesCount && ChannelCount && SampleRate ) {
		// Copy the new audio samples
		mSamples.assign( Samples, Samples + SamplesCount );

		eePRINTL( "Sound file loaded from memory samples." );

		// Update the internal buffer with the new samples
		return Update( ChannelCount, SampleRate );
	} else {
		// Error...
		eePRINTL( "Failed to load sound buffer from memory Samples" );
		return false;
	}
}

bool cSoundBuffer::SaveToFile(const std::string& Filename) const {
	// Create the sound file in write mode
	std::auto_ptr<cSoundFile> File( cSoundFile::CreateWrite( Filename, GetChannelCount(), GetSampleRate() ) );

	if ( File.get() ) {
		// Write the samples to the opened file
		File->Write( &mSamples[0], mSamples.size() );
		return true;
	} else {
		// Error...
		eePRINTL( "Failed to save sound buffer to file %s", Filename.c_str() );
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

unsigned int cSoundBuffer::GetChannelCount() const {
	ALint ChannelCount;
	ALCheck( alGetBufferi( mBuffer, AL_CHANNELS, &ChannelCount ) );
	return ChannelCount;
}

cTime cSoundBuffer::GetDuration() const {
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

bool cSoundBuffer::Update( unsigned int ChannelCount, unsigned int SampleRate ) {
	// Check parameters
	if ( !SampleRate || !ChannelCount || mSamples.empty() )
		return false;

	// Find the good format according to the number of channels
	ALenum Format = cAudioDevice::GetFormatFromChannelCount( ChannelCount );

	// Check if the format is valid
	if ( Format == 0 ) {
		eePRINTL( "Unsupported number of channels." );
		return false;
	}

	// First make a copy of the list of sounds so we can reattach later
	SoundList sounds( mSounds );

	// Detach the buffer from the sounds that use it (to avoid OpenAL errors)
	for (SoundList::const_iterator it = sounds.begin(); it != sounds.end(); ++it)
		(*it)->ResetBuffer();

	// Fill the buffer
	ALsizei Size = static_cast<ALsizei>( mSamples.size() ) * sizeof(Int16);
	ALCheck( alBufferData( mBuffer, Format, &mSamples[0], Size, SampleRate ) );

	// Compute the duration
	mDuration = Seconds(static_cast<float>(mSamples.size()) / SampleRate / ChannelCount);

	return true;
}

void cSoundBuffer::AttachSound( cSound* sound ) const {
	mSounds.insert(sound);
}

void cSoundBuffer::DetachSound( cSound* sound ) const {
	mSounds.erase(sound);
}

}}
