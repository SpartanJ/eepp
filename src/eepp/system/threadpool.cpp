#include <eepp/system/threadpool.hpp>

namespace EE { namespace System {

std::shared_ptr<ThreadPool> ThreadPool::createShared( Uint32 numThreads ) {
	std::shared_ptr<ThreadPool> pool( new ThreadPool() );
	create( pool.get(), numThreads );
	return pool;
}

std::unique_ptr<ThreadPool> ThreadPool::createUnique( Uint32 numThreads ) {
	std::unique_ptr<ThreadPool> pool( new ThreadPool() );
	create( pool.get(), numThreads );
	return pool;
}

ThreadPool* ThreadPool::createRaw( Uint32 numThreads ) {
	ThreadPool* pool = eeNew( ThreadPool, () );
	return create( pool, numThreads );
}

ThreadPool* ThreadPool::create( ThreadPool* pool, Uint32 numThreads ) {
	for ( Uint32 i = 0; i < numThreads; ++i ) {
		pool->mThreads.emplace_back( std::make_unique<Thread>( &ThreadPool::threadFunc, pool ) );
		pool->mThreads.back().get()->launch();
	}
	return pool;
}

ThreadPool::ThreadPool() {}

ThreadPool::~ThreadPool() {
	{
		std::unique_lock<std::mutex> lock( mMutex );
		mShuttingDown = true;
	}

	mWorkAvailable.notify_all();

	for ( auto& t : mThreads ) {
		t.get()->wait();
	}
}

void ThreadPool::threadFunc() {
	while ( true ) {
		std::unique_ptr<Work> work;
		{
			std::unique_lock<std::mutex> lock( mMutex );

			mWorkAvailable.wait( lock, [this]() { return !mWork.empty() || mShuttingDown; } );

			if ( mShuttingDown && mWork.empty() ) {
				return;
			}

			work = std::move( mWork.front() );
			mWork.pop_front();
		}

		work->func();

		if ( work->callback != nullptr ) {
			work->callback();
		}
	}
}

void ThreadPool::run( const std::function<void()>& func,
					  const std::function<void()>& doneCallback ) {
	{
		std::unique_lock<std::mutex> lock( mMutex );

		if ( mShuttingDown )
			return;

		mWork.emplace_back( new Work{func, doneCallback} );
	}

	mWorkAvailable.notify_one();
}

Uint32 ThreadPool::numThreads() const {
	std::unique_lock<std::mutex> lock( mMutex );
	return mShuttingDown ? 0 : static_cast<Uint32>( mThreads.size() );
}

}} // namespace EE::System
