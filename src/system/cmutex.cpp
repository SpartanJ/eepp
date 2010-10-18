#include "cmutex.hpp"

namespace EE { namespace System {

cMutex::cMutex() {
	mMutex = SDL_CreateMutex();
}

cMutex::~cMutex() {
	SDL_DestroyMutex(mMutex);
}

bool cMutex::Lock() {
	return 0 == SDL_LockMutex(mMutex);
}

bool cMutex::Unlock() {
	return 0 == SDL_UnlockMutex(mMutex);
}

}}
