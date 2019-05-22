#include <eepp/scene/actions/resize.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene { namespace Actions {

Resize * Resize::New( const Sizef& start, const Sizef& end, const Time& duration, const Ease::Interpolation& type ) {
	return eeNew( Resize, ( start, end, duration, type ) );
}

Resize::Resize()
{}

Resize::Resize( const Sizef & start, const Sizef & end, const Time& duration, const Ease::Interpolation& type ) {
	mInterpolation.clear().add( start, duration ).add( end ).setType( type );
}

void Resize::onStart() {
	onUpdate( Time::Zero );
}

void Resize::onUpdate( const Time& ) {
	if ( NULL != mNode ) {
		mNode->setSize( mInterpolation.getPosition() );
	}
}

Action * Resize::clone() const {
	Resize * action = eeNew( Resize, () );
	action->setInterpolation( mInterpolation );
	return action;
}

Action * Resize::reverse() const {
	Resize * action = eeNew( Resize, () );
	action->setInterpolation( mInterpolation.getReversePoints() );
	return action;
}

}}}
