#include <eepp/scene/actions/runnable.hpp>
#include <eepp/system/lock.hpp>

namespace EE { namespace Scene { namespace Actions {

Runnable* Runnable::New( RunnableFunc callback, const Time& time, bool loop ) {
	return eeNew( Runnable, ( callback, time, loop ) );
}

Runnable::Runnable( Runnable::RunnableFunc callback, const Time& time, bool loop ) :
	Delay( time ), mCallback( std::move( callback ) ), mLoop( loop ) {}

void Runnable::update( const Time& ) {
	Lock l( mCallbackMutex );
	if ( mCallback && Delay::isDone() && !mCalled ) {
		mCallback();
		if ( mLoop ) {
			mClock.restart();
		} else {
			mCalled = true;
		}
	} else if ( !mCallback ) {
		mCalled = true;
	}
}

bool Runnable::isDone() {
	return Delay::isDone() && mCalled;
}

Action* Runnable::clone() const {
	return New( mCallback, mTime );
}

Action* Runnable::reverse() const {
	return NULL; // or a time machine
}

void Runnable::setCallback( RunnableFunc&& callback ) {
	Lock l( mCallbackMutex );
	mCallback = std::move( callback );
}

void Runnable::onStart() {}

}}} // namespace EE::Scene::Actions
