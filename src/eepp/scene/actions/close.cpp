#include <eepp/scene/actions/close.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene { namespace Actions {

Close* Close::New( const Time& time ) {
	return eeNew( Close, ( time ) );
}

void Close::update( const Time& ) {
	if ( NULL != mNode && isDone() ) {
		mNode->close();
	}
}

Action* Close::clone() const {
	return New( mTime );
}

Action* Close::reverse() const {
	return NULL; // or a time machine
}

Close::Close( const Time& time ) : Delay( time ) {}

void Close::onStart() {
	update( Seconds( 0 ) );
}

}}} // namespace EE::Scene::Actions
