#include <eepp/ui/actions/rotate.hpp>
#include <eepp/ui/uinode.hpp>

namespace EE { namespace UI { namespace Action {

Rotate * Rotate::New( const Float & start, const Float & end, const Time& duration, const Ease::Interpolation& type ) {
	return eeNew( Rotate, ( start, end, duration, type ) );
}

Rotate::Rotate()
{}

Rotate::Rotate( const Float & start, const Float & end, const Time& duration, const Ease::Interpolation& type )
{
	mInterpolation.clear().add( start, duration ).add( end ).setType( type );
}

void Rotate::onStart() {
	if ( NULL != mNode ) {
		mNode->setRotation( mInterpolation.getPosition() );
	}
}

void Rotate::onUpdate( const Time& time ) {
	if ( NULL != mNode ) {
		mNode->setRotation( mInterpolation.getPosition() );
	}
}

UIAction * Rotate::clone() const {
	Rotate * action = eeNew( Rotate, () );
	action->setInterpolation( mInterpolation );
	return action;
}

UIAction * Rotate::reverse() const {
	return NULL;
}

}}} 
