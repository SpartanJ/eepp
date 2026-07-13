#include <eepp/system/resourceloader.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/system/threadpool.hpp>

namespace EE { namespace System {

ResourceLoader::ResourceLoader( const Uint32& maxThreads ) :
	mLoaded( false ),
	mLoading( false ),
	mThreaded( true ),
	mThreads( maxThreads ),
	mTotalLoaded( 0 ),
	mThread( &ResourceLoader::taskRunner, this ) {
	setThreads();
}

ResourceLoader::~ResourceLoader() {
	mThread.wait();
	clear();
}

void ResourceLoader::setThreads() {
	if ( THREADS_AUTO == mThreads ) {
		mThreads = Sys::getCPUCount();

		if ( 1 == mThreads ) {
			mThreaded = false;
		}
	}
}

bool ResourceLoader::isThreaded() const {
	return mThreaded.load();
}

Uint32 ResourceLoader::getCount() const {
	std::unique_lock<std::mutex> lock( mMutex );
	return mTasks.size();
}

void ResourceLoader::setThreaded( const bool& threaded ) {
	std::unique_lock<std::mutex> lock( mMutex );
	if ( !mLoading ) {
		mThreaded = threaded;
	}
}

void ResourceLoader::add( const ObjectLoaderTask& objectLoaderTask ) {
	std::unique_lock<std::mutex> lock( mMutex );
	if ( !mLoading ) {
		mTasks.emplace_back( objectLoaderTask );
	}
}

bool ResourceLoader::clear() {
	std::unique_lock<std::mutex> lock( mMutex );
	if ( !mLoading ) {
		mLoaded = false;
		mTotalLoaded = 0;
		mTasks.clear();
		mLoadCbs.clear();
		return true;
	}

	return false;
}

void ResourceLoader::load( const ResLoadCallback& callback ) {
	{
		std::unique_lock<std::mutex> lock( mMutex );
		if ( callback )
			mLoadCbs.push_back( callback );
	}

	load();
}

void ResourceLoader::load() {
	bool serialized = false;
	{
		std::unique_lock<std::mutex> lock( mMutex );
		if ( mLoaded || mLoading )
			return;

		mLoading = true;
		if ( mThreaded ) {
			mThread.launch();
		} else {
			serialized = true;
		}
	}

	if ( serialized )
		serializedLoad();
}

bool ResourceLoader::isLoaded() {
	return mLoaded.load();
}

bool ResourceLoader::isLoading() {
	return mLoading.load();
}

void ResourceLoader::setLoaded() {
	mLoaded = true;
	mLoading = false;

	std::vector<ResLoadCallback> callbacks;
	{
		std::unique_lock<std::mutex> lock( mMutex );
		callbacks.swap( mLoadCbs );
	}

	for ( auto& callback : callbacks )
		callback( this );
}

void ResourceLoader::taskRunner() {
	{
		auto pool = ThreadPool::createUnique( eemin( mThreads, (Uint32)mTasks.size() ) );

		for ( auto& task : mTasks )
			pool->run( task, [this]( const auto& ) { mTotalLoaded++; } );
	}

	setLoaded();
}

void ResourceLoader::serializedLoad() {
	for ( auto& task : mTasks ) {
		task();

		mTotalLoaded++;
	}

	setLoaded();
}

Float ResourceLoader::getProgress() {
	Uint32 taskCount;
	{
		std::unique_lock<std::mutex> lock( mMutex );
		taskCount = mTasks.size();
	}

	if ( taskCount == 0 )
		return mLoaded ? 100.f : 0.f;

	return mTotalLoaded / (float)taskCount * 100.f;
}

}} // namespace EE::System
