#include "cthread.hpp"

namespace EE { namespace System {

cThread::cThread() : mIsActive(false), mFunction(NULL), mUserData(NULL) {}
cThread::cThread(cThread::FuncType Function, void* UserData) : mIsActive(false), mFunction(Function), mUserData(UserData) {}

cThread::~cThread() {
	Wait();
}

void cThread::Launch() {
	mIsActive = true;
	mThreadPtr = SDL_CreateThread( &cThread::ThreadFunc, this ); // Create the thread
	
	if (mThreadPtr == NULL) {
		std::cerr << "Failed to create thread" << std::endl;
		mIsActive = false;
	}
}

void cThread::Wait() {
	if (mIsActive) { // Wait for the thread to finish, no timeout
		SDL_WaitThread(mThreadPtr, NULL);	
		
		mIsActive = false; // Reset the thread state
	}
}

void cThread::Terminate() {
	if (mIsActive) {
		SDL_KillThread(mThreadPtr);
		mIsActive = false;
	}
}

void cThread::Run() {
	if (mFunction)
		mFunction(mUserData);
}

int cThread::ThreadFunc(void* UserData) {
	// The cThread instance is stored in the user data
	cThread* ThreadToRun = reinterpret_cast<cThread*>(UserData);
	
	// Forward to the instance
	ThreadToRun->Run();
	
	return 0;
}

}}
