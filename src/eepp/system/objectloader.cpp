#include <eepp/system/objectloader.hpp>

namespace EE { namespace System {

ObjectLoader::ObjectLoader( Uint32 ObjType ):
	mObjType(ObjType),
	mLoaded(false),
	mLoading(false),
	mThreaded(false)
{
}

ObjectLoader::~ObjectLoader()
{
}

void ObjectLoader::load() {
	if ( mLoaded ) {
		setLoaded();
	}

	launch();
}

void ObjectLoader::load( ObjLoadCallback Cb ) {
	if ( Cb.IsSet() ) {
		mLoadCbs.push_back( Cb );
	}

	load();
}

void ObjectLoader::launch() {
	if ( mThreaded ) {
		Thread::launch();
	 } else {
		run();
	}
}

void ObjectLoader::start() {
	mLoading = true;
}

void ObjectLoader::update() {

}

bool ObjectLoader::isLoaded() {
	return mLoaded;
}

bool ObjectLoader::isLoading() {
	return mLoading;
}

bool ObjectLoader::isThreaded() const {
	return mThreaded;
}

void ObjectLoader::setThreaded( const bool& threaded ) {
	mThreaded = threaded;
}

void ObjectLoader::run() {
	start();
}

void ObjectLoader::setLoaded() {
	mLoaded = true;
	mLoading = false;

	if ( mLoadCbs.size() ) {
		std::list<ObjLoadCallback>::iterator it;

		for ( it = mLoadCbs.begin(); it != mLoadCbs.end(); it++ ) {
			(*it)( this );
		}

		mLoadCbs.clear();
	}
}

const Uint32& ObjectLoader::type() const {
	return mObjType;
}

void ObjectLoader::reset() {
	mLoaded		= false;
	mLoading	= false;
}

}}
