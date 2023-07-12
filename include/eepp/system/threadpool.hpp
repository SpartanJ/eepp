#ifndef EE_SYSTEM_THREADPOOL_HPP
#define EE_SYSTEM_THREADPOOL_HPP

#include <atomic>
#include <condition_variable>
#include <deque>
#include <eepp/core/noncopyable.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/mutex.hpp>
#include <eepp/system/thread.hpp>
#include <functional>
#include <memory>
#include <mutex>

namespace EE { namespace System {

class EE_API ThreadPool : NonCopyable {
  public:
	static std::shared_ptr<ThreadPool> createShared( Uint32 numThreads,
													 bool terminateOnClose = false );

	static std::unique_ptr<ThreadPool> createUnique( Uint32 numThreads,
													 bool terminateOnClose = false );

	static ThreadPool* createRaw( Uint32 numThreads, bool terminateOnClose = false );

	ThreadPool( Uint32 numThreads, bool terminateOnClose = false );

	virtual ~ThreadPool();

	Uint64 run(
		const std::function<void()>& func,
		const std::function<void( const Uint64& )>& doneCallback = []( const Uint64& ) {},
		const Uint64& tag = 0 );

	Uint32 numThreads() const;

	bool terminateOnClose() const;

	void setTerminateOnClose( bool terminateOnClose );

	bool existsIdInQueue( const Uint64& id );

	bool existsTagInQueue( const Uint64& tag );

	bool removeId( const Uint64& id );

	bool removeWithTag( const Uint64& tag );

  private:
	struct Work {
		Uint64 id{ 0 };
		const std::function<void()> func;
		const std::function<void( const Uint64& )> callback;
		Uint64 tag{ 0 };
	};

	void threadFunc();

	std::vector<std::unique_ptr<Thread>> mThreads;
	std::deque<std::unique_ptr<Work>> mWork;
	std::atomic<Uint64> mLastWorkId;
	bool mShuttingDown = false;
	bool mTerminateOnClose = false;
	mutable std::mutex mMutex;
	std::condition_variable mWorkAvailable;
};

}} // namespace EE::System

#endif
