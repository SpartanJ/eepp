#include <eepp/scene/actions/enable.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene { namespace Actions {

Enable* Enable::New( const Time& time ) {
	return eeNew( Enable, ( true, time ) );
}

Enable* Enable::New( bool enable, const Time& time ) {
	return eeNew( Enable, ( enable, time ) );
}

void Enable::update( const Time& time ) {
	if ( NULL != mNode && isDone() ) {
		mNode->setEnabled( mEnable );
	}
}

Action* Enable::clone() const {
	return New( mEnable, mTime );
}

Action* Enable::reverse() const {
	return NULL; // or a time machine
}

Enable::Enable( bool enable, const Time& time ) : Delay( time ), mEnable( enable ) {}

void Enable::onStart() {
	update( Seconds( 0 ) );
}

}}} // namespace EE::Scene::Actions
