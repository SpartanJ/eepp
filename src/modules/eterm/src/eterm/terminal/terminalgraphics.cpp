#include <algorithm>
#include <eterm/terminal/terminalgraphics.hpp>
#include <iterator>

namespace eterm { namespace Terminal {

TerminalGraphicsUpdateQueue::TerminalGraphicsUpdateQueue( size_t maxUpdates, size_t maxBytes ) :
	mMaxUpdates( maxUpdates ), mMaxBytes( maxBytes ) {}

Uint64 TerminalGraphicsUpdateQueue::enqueue( TerminalGraphicsUpdate update ) {
	std::lock_guard<std::mutex> lock( mMutex );
	const size_t payloadBytes = update.payloadBytes();
	if ( mNeedsResync )
		return ++mNextSequence;

	auto isFullImageUpdate = []( TerminalGraphicsUpdateType type ) {
		return type == TerminalGraphicsUpdateType::CreateImage ||
			   type == TerminalGraphicsUpdateType::ReplaceImage;
	};
	if ( !mUpdates.empty() && isFullImageUpdate( update.type ) &&
		 isFullImageUpdate( mUpdates.back().type ) && mUpdates.back().imageId == update.imageId ) {
		const size_t previousBytes = mUpdates.back().payloadBytes();
		const size_t otherBytes = mQueuedBytes - previousBytes;
		if ( mUpdates.size() == 1 ||
			 ( otherBytes <= mMaxBytes && payloadBytes <= mMaxBytes - otherBytes ) ) {
			update.sequence = mUpdates.back().sequence;
			if ( mUpdates.back().type == TerminalGraphicsUpdateType::CreateImage )
				update.type = TerminalGraphicsUpdateType::CreateImage;
			mQueuedBytes = mQueuedBytes - previousBytes + payloadBytes;
			mUpdates.back() = std::move( update );
			return mUpdates.back().sequence;
		}
	}

	update.sequence = ++mNextSequence;

	// The Kitty decoder bounds individual payloads. A single large frame can make progress
	// without allowing an unbounded backlog of frames behind it.
	if ( mUpdates.size() >= mMaxUpdates ||
		 ( !mUpdates.empty() &&
		   ( mQueuedBytes > mMaxBytes || payloadBytes > mMaxBytes - mQueuedBytes ) ) ) {
		mUpdates.clear();
		mQueuedBytes = 0;
		mNeedsResync = true;
		TerminalGraphicsUpdate resync;
		resync.sequence = update.sequence;
		resync.type = TerminalGraphicsUpdateType::Resync;
		mUpdates.emplace_back( std::move( resync ) );
		return update.sequence;
	}

	mQueuedBytes += payloadBytes;
	mUpdates.emplace_back( std::move( update ) );
	return mNextSequence;
}

void TerminalGraphicsUpdateQueue::replace(
	std::vector<TerminalGraphicsUpdate>&& updates,
	const std::shared_ptr<TerminalGraphicsPresentation>& presentation ) {
	std::lock_guard<std::mutex> lock( mMutex );
	mUpdates.clear();
	mQueuedBytes = 0;
	mNeedsResync = false;
	for ( auto& update : updates )
		update.sequence = ++mNextSequence;
	presentation->requiredUpdateSequence = mNextSequence;
	// Retain references to the authoritative pixels even if later incremental updates overflow.
	if ( !updates.empty() )
		updates.front().recoveryPresentation = presentation;
	mReplacement.assign( std::make_move_iterator( updates.begin() ),
						 std::make_move_iterator( updates.end() ) );
}

std::vector<TerminalGraphicsUpdate> TerminalGraphicsUpdateQueue::drain() {
	std::vector<TerminalGraphicsUpdate> updates;
	std::lock_guard<std::mutex> lock( mMutex );
	const size_t limit = std::max( size_t{ 1 }, mMaxUpdates );
	updates.reserve( std::min( limit, mReplacement.size() + mUpdates.size() ) );
	size_t bytes = 0;
	while ( updates.size() < limit && ( !mReplacement.empty() || !mUpdates.empty() ) ) {
		const bool replaying = !mReplacement.empty();
		auto& pending = replaying ? mReplacement : mUpdates;
		const size_t payload = pending.front().payloadBytes();
		// Make progress on a large image by allowing one payload above the byte limit.
		if ( bytes && payload && ( bytes > mMaxBytes || payload > mMaxBytes - bytes ) )
			break;
		bytes += payload;
		if ( !replaying )
			mQueuedBytes -= payload;
		updates.emplace_back( std::move( pending.front() ) );
		pending.pop_front();
	}
	return updates;
}

size_t TerminalGraphicsUpdateQueue::queuedBytes() const {
	std::lock_guard<std::mutex> lock( mMutex );
	size_t bytes = mQueuedBytes;
	for ( const auto& update : mReplacement )
		bytes += update.payloadBytes();
	return bytes;
}

bool TerminalGraphicsUpdateQueue::needsResync() const {
	std::lock_guard<std::mutex> lock( mMutex );
	return mNeedsResync;
}

}} // namespace eterm::Terminal
