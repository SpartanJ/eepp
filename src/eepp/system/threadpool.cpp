#include <algorithm>
#include <eepp/system/threadpool.hpp>

namespace EE { namespace System {

std::shared_ptr<ThreadPool> ThreadPool::createShared( Uint32 numThreads, bool terminateOnClose ) {
	std::shared_ptr<ThreadPool> pool( new ThreadPool( numThreads, terminateOnClose ) );
	return pool;
}

std::unique_ptr<ThreadPool> ThreadPool::createUnique( Uint32 numThreads, bool terminateOnClose ) {
	std::unique_ptr<ThreadPool> pool( new ThreadPool( numThreads, terminateOnClose ) );
	return pool;
}

ThreadPool* ThreadPool::createRaw( Uint32 numThreads, bool terminateOnClose ) {
	return eeNew( ThreadPool, ( numThreads, terminateOnClose ) );
}

ThreadPool::ThreadPool( Uint32 numThreads, bool terminateOnClose ) :
	mTerminateOnClose( terminateOnClose ) {
	for ( Uint32 i = 0; i < numThreads; ++i ) {
		mThreads.emplace_back( std::make_unique<Thread>( &ThreadPool::threadFunc, this ) );
		mThreads.back()->launch();
	}
}

ThreadPool::~ThreadPool() {
	{
		std::unique_lock<std::mutex> lock( mMutex );
		mShuttingDown = true;
	}

	mWorkAvailable.notify_all();

	for ( auto& t : mThreads ) {
		if ( terminateOnClose() ) {
			t->terminate();
		} else {
			t->wait();
		}
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
			work->callback( work->id );
		}
	}
}

bool ThreadPool::terminateOnClose() const {
	return mTerminateOnClose;
}

void ThreadPool::setTerminateOnClose( bool terminateOnClose ) {
	mTerminateOnClose = terminateOnClose;
}

bool ThreadPool::existsIdInQueue( const Uint64& id ) {
	std::unique_lock<std::mutex> lock( mMutex );
	return std::any_of( mWork.begin(), mWork.end(),
						[id]( const std::unique_ptr<Work>& work ) { return work->id == id; } );
}

bool ThreadPool::existsTagInQueue( const Uint64& tag ) {
	std::unique_lock<std::mutex> lock( mMutex );
	return std::any_of( mWork.begin(), mWork.end(),
						[tag]( const std::unique_ptr<Work>& work ) { return work->tag == tag; } );
}

bool ThreadPool::removeId( const Uint64& id ) {
	std::unique_lock<std::mutex> lock( mMutex );
	for ( auto it = mWork.begin(); it != mWork.end(); ++it ) {
		if ( it->get()->id == id ) {
			mWork.erase( it );
			return true;
		}
	}
	return false;
}

bool ThreadPool::removeWithTag( const Uint64& tag ) {
	std::vector<Uint64> ids;
	{
		std::unique_lock<std::mutex> lock( mMutex );
		for ( const auto& work : mWork )
			if ( work->tag == tag )
				ids.emplace_back( work->id );
	}
	for ( const auto& id : ids )
		removeId( id );
	return !ids.empty();
}

Uint64 ThreadPool::run( const std::function<void()>& func,
						const std::function<void( const Uint64& )>& doneCallback,
						const Uint64& tag ) {
	Uint64 id = ++mLastWorkId;
	{
		std::unique_lock<std::mutex> lock( mMutex );

		if ( mShuttingDown )
			return id;

		mWork.emplace_back( new Work{ id, func, doneCallback, tag } );
	}

	mWorkAvailable.notify_one();

	return id;
}

Uint32 ThreadPool::numThreads() const {
	std::unique_lock<std::mutex> lock( mMutex );
	return mShuttingDown ? 0 : static_cast<Uint32>( mThreads.size() );
}

}} // namespace EE::System
