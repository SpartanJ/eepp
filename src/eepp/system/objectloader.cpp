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

void ObjectLoader::Load() {
	if ( mLoaded ) {
		SetLoaded();
	}

	Launch();
}

void ObjectLoader::Load( ObjLoadCallback Cb ) {
	if ( Cb.IsSet() ) {
		mLoadCbs.push_back( Cb );
	}

	Load();
}

void ObjectLoader::Launch() {
	if ( mThreaded ) {
		Thread::Launch();
	 } else {
		Run();
	}
}

void ObjectLoader::Start() {
	mLoading = true;
}

void ObjectLoader::Update() {

}

bool ObjectLoader::IsLoaded() {
	return mLoaded;
}

bool ObjectLoader::IsLoading() {
	return mLoading;
}

bool ObjectLoader::Threaded() const {
	return mThreaded;
}

void ObjectLoader::Threaded( const bool& threaded ) {
	mThreaded = threaded;
}

void ObjectLoader::Run() {
	Start();
}

void ObjectLoader::SetLoaded() {
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

const Uint32& ObjectLoader::Type() const {
	return mObjType;
}

void ObjectLoader::Reset() {
	mLoaded		= false;
	mLoading	= false;
}

}}
