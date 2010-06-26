#include "cmutex.hpp"

namespace EE { namespace System {

cMutex::cMutex() {
	mMutex = SDL_CreateMutex();
}

cMutex::~cMutex() {
	SDL_DestroyMutex(mMutex);
}

bool cMutex::Lock() {
	int ret = SDL_LockMutex(mMutex);
	if (ret == 0)
		return true;
	return false;
}

bool cMutex::Unlock() {
	int ret = SDL_UnlockMutex(mMutex);
	if (ret == 0)
		return true;
	return false;
}

}}
