#include <eepp/scene/actions/scale.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene { namespace Actions {

Scale* Scale::New( const Vector2f& start, const Vector2f& end, const Time& duration,
				   const Ease::Interpolation& type ) {
	return eeNew( Scale, ( start, end, duration, type ) );
}

Scale::Scale() {}

Scale::Scale( const Vector2f& start, const Vector2f& end, const Time& duration,
			  const Ease::Interpolation& type ) {
	mInterpolation.clear().add( start, duration ).add( end ).setType( type );
}

void Scale::onStart() {
	onUpdate( Time::Zero );
}

void Scale::onUpdate( const Time& ) {
	if ( NULL != mNode ) {
		mNode->setScale( mInterpolation.getPosition() );
	}
}

Action* Scale::clone() const {
	Scale* action = eeNew( Scale, () );
	action->setInterpolation( mInterpolation );
	return action;
}

Action* Scale::reverse() const {
	Scale* action = eeNew( Scale, () );
	action->setInterpolation( mInterpolation.getReversePoints() );
	return action;
}

}}} // namespace EE::Scene::Actions
