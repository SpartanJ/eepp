#include "cresourceloader.hpp"

namespace EE { namespace System {

cResourceLoader::cResourceLoader( const Uint32& MaxThreads ) :
	mLoaded(false),
	mLoading(false),
	mThreaded(true),
	mThreads(MaxThreads)
{
	SetThreads();
}

cResourceLoader::~cResourceLoader() {
	Clear();
}

void cResourceLoader::SetThreads() {
	if ( THREADS_AUTO == mThreads ) {
		eeInt NumCpus = GetNumCPUs() - 1;

		if ( NumCpus > 1 )
			mThreads = NumCpus;
		else
			mThreads = 1;
	}
}

bool cResourceLoader::Threaded() const {
	return mThreaded;
}

void cResourceLoader::Threaded( const bool& threaded ) {
	if ( !mLoading )
		mThreaded = threaded;
}

void cResourceLoader::Add( cObjectLoader * Object ) {
	mObjs.push_front( Object );
}

bool cResourceLoader::Clear( const bool& ClearObjectsLoaded ) {
	if ( !mLoading ) {
		mLoaded = false;
		mLoading = false;

		std::list<cObjectLoader *>::iterator it;

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

void cResourceLoader::Load( ResLoadCallback Cb ) {
	if ( Cb.IsSet() )
		mLoadCbs.push_back( Cb );

	Load();
}

void cResourceLoader::Load() {
	if ( mLoaded )
		return;

	mLoading = true;

	bool AllLoaded = true;

	cObjectLoader * Obj = NULL;
	std::list<cObjectLoader *>::iterator it;
	std::list<cObjectLoader *> ObjsErase;

	Uint32 count = 0;

	for ( it = mObjs.begin(); it != mObjs.end(); it++ ) {
		Obj = (*it);

		if ( NULL != Obj ) {
			Obj->Threaded( mThreaded );

			if ( !Obj->IsLoaded() ) {
				if ( !Obj->IsLoading() )
					Obj->Load();

				if ( Obj->IsLoading() )
					count++;

				Obj->Update();

				if ( !Obj->IsLoaded() )
					AllLoaded = false;
				else
					ObjsErase.push_back( Obj );

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

	if ( AllLoaded )
		SetLoaded();
}

void cResourceLoader::Update() {
	Load();
}

bool cResourceLoader::IsLoaded() {
	return mLoaded;
}

bool cResourceLoader::IsLoading() {
	return mLoading;
}

void cResourceLoader::SetLoaded() {
	mLoaded = true;
	mLoading = false;

	if ( mLoadCbs.size() ) {
		std::list<ResLoadCallback>::iterator it;

		for ( it = mLoadCbs.begin(); it != mLoadCbs.end(); it++ )
			(*it)( this );

		mLoadCbs.clear();
	}
}

eeFloat cResourceLoader::Progress() {
	return ( (eeFloat)mObjsLoaded.size() / (eeFloat)( mObjs.size() + mObjsLoaded.size() ) ) * 100.f;
}

}}

