#include <eepp/scene/actions/visible.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene { namespace Actions {

Visible * Visible::New( bool visible, const Time & time) {
	return eeNew( Visible, ( visible, time ) );
}

void Visible::update(const Time & time) {
	if ( NULL != mNode && isDone() ) {
		mNode->setVisible( mVisible );
	}
}

Action * Visible::clone() const {
	return New( mVisible, mTime );
}

Action * Visible::reverse() const {
	return NULL; // or a time machine
}

Visible::Visible( bool visible, const Time & time) :
	Delay( time ),
	mVisible( visible )
{}

void Visible::onStart() {
	update(Seconds(0));
}

}}}
