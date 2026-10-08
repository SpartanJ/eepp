#ifndef ECODE_BOUNDEDEVENTQUEUE_HPP
#define ECODE_BOUNDEDEVENTQUEUE_HPP

#include <cstddef>
#include <deque>
#include <mutex>
#include <utility>

namespace ecode {

// Thread-safe producer queue with a single-consumer scheduling handshake.
// push() returns true exactly when the caller must schedule the consumer.
// Once a drain completes, finishDrain() either transfers responsibility to
// the next drain or atomically makes the next producer responsible for it.
template <typename T> class BoundedEventQueue {
  public:
	bool push( T&& event ) {
		std::lock_guard<std::mutex> lock( mMutex );
		mEvents.emplace_back( std::move( event ) );
		if ( mDrainScheduled )
			return false;
		mDrainScheduled = true;
		return true;
	}

	template <typename Predicate> bool pushOrReplaceLast( T&& event, Predicate&& shouldReplace ) {
		std::lock_guard<std::mutex> lock( mMutex );
		if ( !mEvents.empty() && shouldReplace( mEvents.back(), event ) ) {
			mEvents.back() = std::move( event );
			return false;
		}
		mEvents.emplace_back( std::move( event ) );
		if ( mDrainScheduled )
			return false;
		mDrainScheduled = true;
		return true;
	}

	template <typename Container> void popUpTo( Container& events, std::size_t maxEvents ) {
		std::lock_guard<std::mutex> lock( mMutex );
		while ( !mEvents.empty() && events.size() < maxEvents ) {
			events.emplace_back( std::move( mEvents.front() ) );
			mEvents.pop_front();
		}
	}

	bool finishDrain() {
		std::lock_guard<std::mutex> lock( mMutex );
		if ( !mEvents.empty() )
			return true;
		mDrainScheduled = false;
		return false;
	}

	void clear() {
		std::lock_guard<std::mutex> lock( mMutex );
		mEvents.clear();
		mDrainScheduled = false;
	}

  private:
	std::mutex mMutex;
	std::deque<T> mEvents;
	bool mDrainScheduled{ false };
};

} // namespace ecode

#endif // ECODE_BOUNDEDEVENTQUEUE_HPP
