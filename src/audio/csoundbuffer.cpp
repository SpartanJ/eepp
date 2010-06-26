#include "csoundbuffer.hpp"

namespace EE { namespace Audio {

cSoundBuffer::cSoundBuffer() : myBuffer (0), myDuration(0.f) {
	ALCheck(alGenBuffers(1, &myBuffer));
}

cSoundBuffer::cSoundBuffer(const cSoundBuffer& Copy) : cAudioResource(Copy), myBuffer (0), mySamples (Copy.mySamples), myDuration (Copy.myDuration) {
	ALCheck(alGenBuffers(1, &myBuffer));
	Update(Copy.GetChannelsCount(), Copy.GetSampleRate());
}

cSoundBuffer::~cSoundBuffer() {
	if (myBuffer)
		ALCheck(alDeleteBuffers(1, &myBuffer));
}

bool cSoundBuffer::LoadFromFile(const std::string& Filename) {
	// Create the sound file
	std::auto_ptr<cSoundFile> File(cSoundFile::CreateRead(Filename));
	
	// Open the sound file
	if ( File.get() ) {
		// Get the sound parameters
		std::size_t  NbSamples	 = File->GetSamplesCount();
		unsigned int ChannelsCount = File->GetChannelsCount();
		unsigned int SampleRate	= File->GetSampleRate();
		
		// Read the samples from the opened file
		mySamples.resize(NbSamples);
		if (File->Read(&mySamples[0], NbSamples) == NbSamples) {
			cLog::instance()->Write( "Sound file " + Filename + " loaded." );
			
			// Update the internal buffer with the new samples
			return Update(ChannelsCount, SampleRate);
		} else {
			cLog::instance()->Write( "Failed to read audio data from file \"" + Filename + "\"" );
			return false;
		}
	} else {
		cLog::instance()->Write( "Failed to load sound buffer from file \"" + Filename + "\"" );
		return false;
	}
}

bool cSoundBuffer::LoadFromPack( cPack* Pack, const std::string& FilePackPath ) {
	std::vector<Uint8> TmpData;
	
	if ( Pack->IsOpen() && Pack->ExtractFileToMemory( FilePackPath, TmpData ) )
		return LoadFromMemory( reinterpret_cast<const char*> (&TmpData[0]), TmpData.size() );
	
	return false;
}

bool cSoundBuffer::LoadFromMemory(const char* Data, std::size_t SizeInBytes) {
	// Create the sound file
	std::auto_ptr<cSoundFile> File(cSoundFile::CreateRead(Data, SizeInBytes));

	// Open the sound file
	if (File.get()) {
		// Get the sound parameters
		std::size_t  NbSamples	 = File->GetSamplesCount();
		unsigned int ChannelsCount = File->GetChannelsCount();
		unsigned int SampleRate	= File->GetSampleRate();

		// Read the samples from the opened file
		mySamples.resize(NbSamples);
		if (File->Read(&mySamples[0], NbSamples) == NbSamples) {
			cLog::instance()->Write( "Sound file loaded from memory." );
			
			// Update the internal buffer with the new samples
			return Update(ChannelsCount, SampleRate);
		} else {
			cLog::instance()->Write( "Failed to read audio data from file in memory" );
			return false;
		}
	}else {
		cLog::instance()->Write( "Failed to load sound buffer from file in memory" );
		return false;
	}
}

bool cSoundBuffer::LoadFromSamples(const Int16* Samples, std::size_t SamplesCount, unsigned int ChannelsCount, unsigned int SampleRate) {
	if (Samples && SamplesCount && ChannelsCount && SampleRate) {
		// Copy the new audio samples
		mySamples.assign(Samples, Samples + SamplesCount);
		
		cLog::instance()->Write( "Sound file loaded from memory samples." );
		
		// Update the internal buffer with the new samples
		return Update(ChannelsCount, SampleRate);
	} else {
		// Error...
		cLog::instance()->Write( "Failed to load sound buffer from memory Samples" );
		return false;
	}
}

bool cSoundBuffer::SaveToFile(const std::string& Filename) const {
	// Create the sound file in write mode
	std::auto_ptr<cSoundFile> File(cSoundFile::CreateWrite(Filename, GetChannelsCount(), GetSampleRate()));
	if (File.get()) {
		// Write the samples to the opened file
		File->Write(&mySamples[0], mySamples.size());
		return true;
	} else {
		// Error...
		cLog::instance()->Write( "Failed to save sound buffer to file \"" + Filename + "\"" );
		return false;
	}
}

const Int16* cSoundBuffer::GetSamples() const {
	return mySamples.empty() ? NULL : &mySamples[0];
}

std::size_t cSoundBuffer::GetSamplesCount() const {
	return mySamples.size();
}

unsigned int cSoundBuffer::GetSampleRate() const {
	ALint SampleRate;
	ALCheck(alGetBufferi(myBuffer, AL_FREQUENCY, &SampleRate));
	return SampleRate;
}

unsigned int cSoundBuffer::GetChannelsCount() const {
	ALint ChannelsCount;
	ALCheck(alGetBufferi(myBuffer, AL_CHANNELS, &ChannelsCount));
	return ChannelsCount;
}

eeFloat cSoundBuffer::GetDuration() const {
	return myDuration;
}

cSoundBuffer& cSoundBuffer::operator =(const cSoundBuffer& Other) {
	cSoundBuffer Temp(Other);

	mySamples.swap(Temp.mySamples);
	std::swap(myBuffer,   Temp.myBuffer);
	std::swap(myDuration, Temp.myDuration);
	return *this;
}

bool cSoundBuffer::Update(unsigned int ChannelsCount, unsigned int SampleRate) {
	// Check parameters
	if (!SampleRate || !ChannelsCount || mySamples.empty())
		return false;

	// Find the good format according to the number of channels
	ALenum Format = cAudioDevice::instance().GetFormatFromChannelsCount(ChannelsCount);

	// Check if the format is valid
	if (Format == 0) {
		cLog::instance()->Write( "Unsupported number of channels." );
		return false;
	}

	// Fill the buffer
	ALsizei Size = static_cast<ALsizei>(mySamples.size()) * sizeof(Int16);
	ALCheck(alBufferData(myBuffer, Format, &mySamples[0], Size, SampleRate));

	// Compute the duration
	myDuration = static_cast<eeFloat>(mySamples.size()) / SampleRate / ChannelsCount;

	return true;
}


}}
