#include <eepp/scene/actions/rotate.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene { namespace Actions {

Rotate* Rotate::New( const Float& start, const Float& end, const Time& duration,
					 const Ease::Interpolation& type ) {
	return eeNew( Rotate, ( start, end, duration, type ) );
}

Rotate::Rotate() {}

Rotate::Rotate( const Float& start, const Float& end, const Time& duration,
				const Ease::Interpolation& type ) {
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

Action* Rotate::clone() const {
	Rotate* action = eeNew( Rotate, () );
	action->setInterpolation( mInterpolation );
	return action;
}

Action* Rotate::reverse() const {
	Rotate* action = eeNew( Rotate, () );
	action->setInterpolation( Interpolation1d( mInterpolation.getReversePoints() ) );
	return action;
}

}}} // namespace EE::Scene::Actions
