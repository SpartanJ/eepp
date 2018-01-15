#include <eepp/system/resourceloader.hpp>
#include <eepp/system/sys.hpp>

namespace EE { namespace System {

ResourceLoader::ResourceLoader( const Uint32& MaxThreads ) :
	mLoaded(false),
	mLoading(false),
	mThreaded(true),
	mThreads(MaxThreads)
{
	setThreads();
}

ResourceLoader::~ResourceLoader() {
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
	return mThreaded;
}

Uint32 ResourceLoader::getCount() const {
	return mObjs.size();
}

void ResourceLoader::setThreaded( const bool& threaded ) {
	if ( !mLoading ) {
		mThreaded = threaded;
	}
}

void ResourceLoader::add( ObjectLoader * Object ) {
	mObjs.push_front( Object );
}

bool ResourceLoader::clear( const bool& ClearObjectsLoaded ) {
	if ( !mLoading ) {
		mLoaded = false;
		mLoading = false;

		std::list<ObjectLoader *>::iterator it;

		for ( it = mObjs.begin(); it != mObjs.end(); it++ )
			eeSAFE_DELETE( *it );

		mObjs.clear();

		if ( ClearObjectsLoaded ) {
			for ( it = mObjsLoaded.begin(); it != mObjsLoaded.end(); it++ )
				eeSAFE_DELETE( *it );

			mObjsLoaded.clear();
		}

		return true;
	}

	return false;
}

void ResourceLoader::load( ResLoadCallback Cb ) {
	if ( Cb.IsSet() )
		mLoadCbs.push_back( Cb );

	load();
}

void ResourceLoader::load() {
	if ( mLoaded )
		return;

	mLoading = true;

	bool AllLoaded = true;

	ObjectLoader * Obj = NULL;
	std::list<ObjectLoader *>::iterator it;
	std::list<ObjectLoader *> ObjsErase;

	Uint32 count = 0;

	for ( it = mObjs.begin(); it != mObjs.end(); it++ ) {
		Obj = (*it);

		if ( NULL != Obj ) {
			Obj->setThreaded( mThreaded );

			if ( !Obj->isLoaded() ) {
				if ( !Obj->isLoading() ) {
					Obj->load();
				}

				if ( Obj->isLoading() ) {
					count++;
				}

				Obj->update();

				if ( !Obj->isLoaded() ) {
					AllLoaded = false;
				} else {
					ObjsErase.push_back( Obj );
				}

				if ( mThreaded && mThreads == count ) {
					AllLoaded = false;
					break;
				}
			}
		}
	}

	for ( it = ObjsErase.begin(); it != ObjsErase.end(); it++ ) {
		Obj = (*it);
		mObjs.remove( Obj );
		mObjsLoaded.push_back( Obj );
	}

	if ( AllLoaded ) {
		setLoaded();
	}
}

void ResourceLoader::update() {
	load();
}

void ResourceLoader::unload() {
	if ( mLoaded ) {
		std::list<ObjectLoader *>::iterator it;

		for ( it = mObjs.begin(); it != mObjs.end(); it++ ) {
			(*it)->unload();
		}

		mLoaded = false;
	}
}

bool ResourceLoader::isLoaded() {
	return mLoaded;
}

bool ResourceLoader::isLoading() {
	return mLoading;
}

void ResourceLoader::setLoaded() {
	mLoaded		= true;
	mLoading	= false;

	if ( mLoadCbs.size() ) {
		std::list<ResLoadCallback>::iterator it;

		for ( it = mLoadCbs.begin(); it != mLoadCbs.end(); it++ ) {
			(*it)( this );
		}

		mLoadCbs.clear();
	}
}

Float ResourceLoader::getProgress() {
	return ( (Float)mObjsLoaded.size() / (Float)( mObjs.size() + mObjsLoaded.size() ) ) * 100.f;
}

}}

