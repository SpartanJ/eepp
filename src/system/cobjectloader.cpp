#include "cobjectloader.hpp"

namespace EE { namespace System {

cObjectLoader::cObjectLoader(): 
	mObjType(0xFFFFFFFF),
	mLoaded(false),
	mThreaded(false)
{
}

cObjectLoader::~cObjectLoader()
{
}

void cObjectLoader::Load() {
	Launch();	
}

void cObjectLoader::Launch() {
	if ( mThreaded )
		cThread::Launch();
	else
		Run();
}

void cObjectLoader::Start() {

}

void cObjectLoader::Update() {
	
}

bool cObjectLoader::IsLoaded() {
	return mLoaded;	
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

}} 
