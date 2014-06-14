#include <eepp/system/resourceloader.hpp>
#include <eepp/system/sys.hpp>

namespace EE { namespace System {

ResourceLoader::ResourceLoader( const Uint32& MaxThreads ) :
	mLoaded(false),
	mLoading(false),
	mThreaded(true),
	mThreads(MaxThreads)
{
	SetThreads();
}

ResourceLoader::~ResourceLoader() {
	Clear();
}

void ResourceLoader::SetThreads() {
	if ( THREADS_AUTO == mThreads ) {
		mThreads = Sys::GetCPUCount();

		if ( 1 == mThreads ) {
			mThreaded = false;
		}
	}
}

bool ResourceLoader::Threaded() const {
	return mThreaded;
}

Uint32 ResourceLoader::Count() const {
	return mObjs.size();
}

void ResourceLoader::Threaded( const bool& threaded ) {
	if ( !mLoading ) {
		mThreaded = threaded;
	}
}

void ResourceLoader::Add( ObjectLoader * Object ) {
	mObjs.push_front( Object );
}

bool ResourceLoader::Clear( const bool& ClearObjectsLoaded ) {
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

void ResourceLoader::Load( ResLoadCallback Cb ) {
	if ( Cb.IsSet() )
		mLoadCbs.push_back( Cb );

	Load();
}

void ResourceLoader::Load() {
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
			Obj->Threaded( mThreaded );

			if ( !Obj->IsLoaded() ) {
				if ( !Obj->IsLoading() ) {
					Obj->Load();
				}

				if ( Obj->IsLoading() ) {
					count++;
				}

				Obj->Update();

				if ( !Obj->IsLoaded() ) {
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
		SetLoaded();
	}
}

void ResourceLoader::Update() {
	Load();
}

void ResourceLoader::Unload() {
	if ( mLoaded ) {
		std::list<ObjectLoader *>::iterator it;

		for ( it = mObjs.begin(); it != mObjs.end(); it++ ) {
			(*it)->Unload();
		}

		mLoaded = false;
	}
}

bool ResourceLoader::IsLoaded() {
	return mLoaded;
}

bool ResourceLoader::IsLoading() {
	return mLoading;
}

void ResourceLoader::SetLoaded() {
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

Float ResourceLoader::Progress() {
	return ( (Float)mObjsLoaded.size() / (Float)( mObjs.size() + mObjsLoaded.size() ) ) * 100.f;
}

}}

