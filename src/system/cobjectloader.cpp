#include "cobjectloader.hpp"

namespace EE { namespace System {

cObjectLoader::cObjectLoader( Uint32 ObjType ):
	mObjType(ObjType),
	mLoaded(false),
	mLoading(false),
	mThreaded(false)
{
}

cObjectLoader::~cObjectLoader()
{
}

void cObjectLoader::Load( ObjLoadCallback Cb ) {
	if ( NULL != Cb )
		mLoadCbs.push_back( Cb );

	if ( mLoaded )
		SetLoaded();

	Launch();
}

void cObjectLoader::Launch() {
	if ( mThreaded )
		cThread::Launch();
	else
		Run();
}

void cObjectLoader::Start() {
	mLoading = true;
}

void cObjectLoader::Update() {

}

bool cObjectLoader::IsLoaded() {
	return mLoaded;
}

bool cObjectLoader::IsLoading() {
	return mLoading;
}

bool cObjectLoader::Threaded() const {
	return mThreaded;
}

void cObjectLoader::Threaded( const bool& threaded ) {
	mThreaded = threaded;
}

void cObjectLoader::Run() {
	Start();
}

void cObjectLoader::SetLoaded() {
	mLoaded = true;
	mLoading = false;

	if ( mLoadCbs.size() ) {
		std::list<ObjLoadCallback>::iterator it;

		for ( it = mLoadCbs.begin(); it != mLoadCbs.end(); it++ )
			(*it)( this );

		mLoadCbs.clear();
	}
}

}}
