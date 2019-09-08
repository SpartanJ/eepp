#ifndef EE_SYSTEM_THREADPOOL_HPP
#define EE_SYSTEM_THREADPOOL_HPP

#include <eepp/system/mutex.hpp>
#include <eepp/system/lock.hpp>
#include <eepp/system/thread.hpp>
#include <eepp/core/noncopyable.hpp>
#include <memory>
#include <functional>
#include <deque>
#include <condition_variable>
#include <mutex>

namespace EE { namespace System {

class EE_API ThreadPool : NonCopyable {
	public:
		static std::unique_ptr<ThreadPool> create(Uint32 numThreads);

		virtual ~ThreadPool();

		void run(const std::function<void()>& func,
				 const std::function<void()>& doneCallback);

		Uint32 numThreads() const;
	private:
		struct Work {
			const std::function<void()> func;
			const std::function<void()> callback;
		};

		ThreadPool();

		void ThreadFunc();

		std::vector<std::unique_ptr<Thread>> mThreads;
		std::deque<std::unique_ptr<Work>> mWork;
		bool mShuttingDown = false;
		mutable std::mutex mMutex;
		std::condition_variable mWorkAvailable;
};

}}

#endif
