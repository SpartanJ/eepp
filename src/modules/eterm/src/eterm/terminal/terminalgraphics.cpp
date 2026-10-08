#include <eterm/terminal/terminalgraphics.hpp>

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
		if ( payloadBytes <= mMaxBytes - ( mQueuedBytes - previousBytes ) ) {
			update.sequence = mUpdates.back().sequence;
			if ( mUpdates.back().type == TerminalGraphicsUpdateType::CreateImage )
				update.type = TerminalGraphicsUpdateType::CreateImage;
			mQueuedBytes = mQueuedBytes - previousBytes + payloadBytes;
			mUpdates.back() = std::move( update );
			return mUpdates.back().sequence;
		}
	}

	update.sequence = ++mNextSequence;

	if ( mUpdates.size() >= mMaxUpdates || mQueuedBytes > mMaxBytes ||
		 payloadBytes > mMaxBytes - mQueuedBytes ) {
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

std::vector<TerminalGraphicsUpdate> TerminalGraphicsUpdateQueue::drain() {
	std::vector<TerminalGraphicsUpdate> updates;
	std::lock_guard<std::mutex> lock( mMutex );
	updates.reserve( mUpdates.size() );
	while ( !mUpdates.empty() ) {
		updates.emplace_back( std::move( mUpdates.front() ) );
		mUpdates.pop_front();
	}
	mQueuedBytes = 0;
	return updates;
}

size_t TerminalGraphicsUpdateQueue::queuedBytes() const {
	std::lock_guard<std::mutex> lock( mMutex );
	return mQueuedBytes;
}

bool TerminalGraphicsUpdateQueue::needsResync() const {
	std::lock_guard<std::mutex> lock( mMutex );
	return mNeedsResync;
}

void TerminalGraphicsUpdateQueue::resetResync() {
	std::lock_guard<std::mutex> lock( mMutex );
	mNeedsResync = false;
}

}} // namespace eterm::Terminal
