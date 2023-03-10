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
	clear();

	mThread.wait();
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
	return mThreaded;
}

Uint32 ResourceLoader::getCount() const {
	return mTasks.size();
}

void ResourceLoader::setThreaded( const bool& threaded ) {
	if ( !mLoading ) {
		mThreaded = threaded;
	}
}

void ResourceLoader::add( const ObjectLoaderTask& objectLoaderTask ) {
	if ( !mLoading ) {
		mTasks.emplace_back( objectLoaderTask );
	}
}

bool ResourceLoader::clear() {
	if ( !mLoading ) {
		mLoaded = false;
		mLoading = false;
		mTotalLoaded = 0;
		mTasks.clear();
		return true;
	}

	return false;
}

void ResourceLoader::load( const ResLoadCallback& callback ) {
	if ( callback )
		mLoadCbs.push_back( callback );

	load();
}

void ResourceLoader::load() {
	if ( mLoaded )
		return;

	if ( mThreaded ) {
		if ( !mLoading ) {
			mLoading = true;
			mThread.launch();
		}
	} else {
		serializedLoad();
	}
}

bool ResourceLoader::isLoaded() {
	return mLoaded;
}

bool ResourceLoader::isLoading() {
	return mLoading;
}

void ResourceLoader::setLoaded() {
	mLoaded = true;
	mLoading = false;

	if ( mLoadCbs.size() ) {
		for ( auto it = mLoadCbs.begin(); it != mLoadCbs.end(); ++it ) {
			( *it )( this );
		}

		mLoadCbs.clear();
	}
}

void ResourceLoader::taskRunner() {
	{
		auto pool = ThreadPool::createUnique( eemin( mThreads, (Uint32)mTasks.size() ) );

		for ( auto& task : mTasks ) {
			pool->run( task, [&]( const auto& ) { mTotalLoaded++; } );
		}
	}

	mLoading = false;
	setLoaded();
}

void ResourceLoader::serializedLoad() {
	mLoading = true;

	for ( auto& task : mTasks ) {
		task();

		mTotalLoaded++;
	}

	mLoading = false;
	setLoaded();
}

Float ResourceLoader::getProgress() {
	return mTotalLoaded / (float)mTasks.size() * 100.f;
}

}} // namespace EE::System
