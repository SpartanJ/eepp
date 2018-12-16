#include <eepp/audio/music.hpp>
#include <eepp/audio/alcheck.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/pack.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/packmanager.hpp>
#include <eepp/core/debug.hpp>
#include <fstream>

namespace EE { namespace Audio {

Music::Music() :
	mFile	  (),
	mLoopSpan  (0, 0)
{}

Music::~Music() {
	// We must stop before destroying the file
	stop();
}

bool Music::openFromFile(const std::string& filename) {
	// First stop the music if it was already running
	stop();

	if ( !FileSystem::fileExists( filename ) ) {
		if ( PackManager::instance()->isFallbackToPacksActive() ) {
			std::string tPath( filename );

			Pack * tPack = PackManager::instance()->exists( tPath );

			if ( NULL != tPack ) {
				return openFromPack( tPack, tPath );
			}
		}

		return false;
	}

	// Open the underlying sound file
	if (!mFile.openFromFile(filename))
		return false;

	// Perform common initializations
	initialize();

	return true;
}

bool Music::openFromMemory(const void* data, std::size_t sizeInBytes) {
	// First stop the music if it was already running
	stop();

	// Open the underlying sound file
	if (!mFile.openFromMemory(data, sizeInBytes))
		return false;

	// Perform common initializations
	initialize();

	return true;
}

bool Music::openFromStream(IOStream& stream) {
	// First stop the music if it was already running
	stop();

	// Open the underlying sound file
	if (!mFile.openFromStream(stream))
		return false;

	// Perform common initializations
	initialize();

	return true;
}

bool Music::openFromPack(Pack * pack, const std::string & filePackPath) {
	if ( pack->isOpen() && pack->extractFileToMemory( filePackPath, mData ) )
		return openFromMemory( reinterpret_cast<const char*> ( mData.data ), mData.size );

	return false;
}

Time Music::getDuration() const {
	return mFile.getDuration();
}

Music::TimeSpan Music::getLoopPoints() const {
	return TimeSpan(samplesToTime(mLoopSpan.offset), samplesToTime(mLoopSpan.length));
}

void Music::setLoopPoints(TimeSpan timePoints) {
	Span<Uint64> samplePoints(timeToSamples(timePoints.offset), timeToSamples(timePoints.length));

	// Check our state. This averts a divide-by-zero. GetChannelCount() is cheap enough to use often
	if (getChannelCount() == 0 || mFile.getSampleCount() == 0) {
		eePRINTL( "Music is not in a valid state to assign Loop Points." );
		return;
	}

	// Round up to the next even sample if needed
	samplePoints.offset += (getChannelCount() - 1);
	samplePoints.offset -= (samplePoints.offset % getChannelCount());
	samplePoints.length += (getChannelCount() - 1);
	samplePoints.length -= (samplePoints.length % getChannelCount());

	// Validate
	if (samplePoints.offset >= mFile.getSampleCount()) {
		eePRINTL( "LoopPoints offset val must be in range [0, Duration)." );
		return;
	}

	if (samplePoints.length == 0) {
		eePRINTL( "LoopPoints length val must be nonzero." );
		return;
	}

	// Clamp End Point
	samplePoints.length = std::min(samplePoints.length, mFile.getSampleCount() - samplePoints.offset);

	// If this change has no effect, we can return without touching anything
	if (samplePoints.offset == mLoopSpan.offset && samplePoints.length == mLoopSpan.length)
		return;

	// When we apply this change, we need to "reset" this instance and its buffer

	// Get old playing status and position
	Status oldStatus = getStatus();
	Time oldPos = getPlayingOffset();

	// Unload
	stop();

	// Set
	mLoopSpan = samplePoints;

	// Restore
	if (oldPos != Time::Zero)
		setPlayingOffset(oldPos);

	// Resume
	if (oldStatus == Playing)
		play();
}

bool Music::onGetData(SoundStream::Chunk& data) {
	Lock lock(mMutex);

	std::size_t toFill = mSamples.size();
	Uint64 currentOffset = mFile.getSampleOffset();
	Uint64 loopEnd = mLoopSpan.offset + mLoopSpan.length;

	// If the loop end is enabled and imminent, request less data.
	// This will trip an "onLoop()" call from the underlying SoundStream,
	// and we can then take action.
	if (getLoop() && (mLoopSpan.length != 0) && (currentOffset <= loopEnd) && (currentOffset + toFill > loopEnd))
		toFill = static_cast<std::size_t>(loopEnd - currentOffset);

	// Fill the chunk parameters
	data.samples = &mSamples[0];
	data.sampleCount = static_cast<std::size_t>(mFile.read(&mSamples[0], toFill));
	currentOffset += data.sampleCount;

	// Check if we have stopped obtaining samples or reached either the EOF or the loop end point
	return (data.sampleCount != 0) && (currentOffset < mFile.getSampleCount()) && !(currentOffset == loopEnd && mLoopSpan.length != 0);
}

void Music::onSeek(Time timeOffset) {
	Lock lock(mMutex);
	mFile.seek(timeOffset);
}

Int64 Music::onLoop() {
	// Called by underlying SoundStream so we can determine where to loop.
	Lock lock(mMutex);
	Uint64 currentOffset = mFile.getSampleOffset();

	if (getLoop() && (mLoopSpan.length != 0) && (currentOffset == mLoopSpan.offset + mLoopSpan.length)) {
		// Looping is enabled, and either we're at the loop end, or we're at the EOF
		// when it's equivalent to the loop end (loop end takes priority). Send us to loop begin
		mFile.seek(mLoopSpan.offset);
		return mFile.getSampleOffset();
	} else if (getLoop() && (currentOffset >= mFile.getSampleCount())) {
		// If we're at the EOF, reset to 0
		mFile.seek(0);
		return 0;
	}
	return NoLoop;
}

void Music::initialize() {
	// Compute the music positions
	mLoopSpan.offset = 0;
	mLoopSpan.length = mFile.getSampleCount();

	// Resize the internal buffer so that it can contain 1 second of audio samples
	mSamples.resize(mFile.getSampleRate() * mFile.getChannelCount());

	// Initialize the stream
	SoundStream::initialize(mFile.getChannelCount(), mFile.getSampleRate());
}

Uint64 Music::timeToSamples(Time position) const {
	// Always ROUND, no unchecked truncation, hence the addition in the numerator.
	// This avoids most precision errors arising from "samples => Time => samples" conversions
	// Original rounding calculation is ((Micros * Freq * Channels) / 1000000) + 0.5
	// We refactor it to keep Int64 as the data type throughout the whole operation.
	return ((position.asMicroseconds() * getSampleRate() * getChannelCount()) + 500000) / 1000000;
}

Time Music::samplesToTime(Uint64 samples) const {
	Time position = Time::Zero;

	// Make sure we don't divide by 0
	if (getSampleRate() != 0 && getChannelCount() != 0)
		position = Microseconds((samples * 1000000) / (getChannelCount() * getSampleRate()));

	return position;
}

}}
