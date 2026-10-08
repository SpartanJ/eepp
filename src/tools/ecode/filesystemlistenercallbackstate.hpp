#ifndef ECODE_FILESYSTEMLISTENERCALLBACKSTATE_HPP
#define ECODE_FILESYSTEMLISTENERCALLBACKSTATE_HPP

#include <condition_variable>
#include <cstddef>
#include <mutex>

namespace ecode {

class FileSystemListenerCallbackState {
  public:
	bool beginCallback() {
		std::lock_guard<std::mutex> lock( mMutex );
		if ( mRemoved )
			return false;
		++mActiveCallbacks;
		return true;
	}

	void endCallback() {
		std::lock_guard<std::mutex> lock( mMutex );
		if ( --mActiveCallbacks == 0 )
			mCondition.notify_all();
	}

	void removeAndWait( bool calledFromThisListener ) {
		std::unique_lock<std::mutex> lock( mMutex );
		mRemoved = true;
		if ( !calledFromThisListener )
			mCondition.wait( lock, [this] { return mActiveCallbacks == 0; } );
	}

  private:
	std::mutex mMutex;
	std::condition_variable mCondition;
	std::size_t mActiveCallbacks{ 0 };
	bool mRemoved{ false };
};

} // namespace ecode

#endif // ECODE_FILESYSTEMLISTENERCALLBACKSTATE_HPP
