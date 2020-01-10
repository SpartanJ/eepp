#include <eepp/scene/actions/resizewidth.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene { namespace Actions {

ResizeWidth* ResizeWidth::New( const Float& start, const Float& end, const Time& duration,
							   const Ease::Interpolation& type ) {
	return eeNew( ResizeWidth, ( start, end, duration, type ) );
}

ResizeWidth::ResizeWidth() {}

ResizeWidth::ResizeWidth( const Float& start, const Float& end, const Time& duration,
						  const Ease::Interpolation& type ) {
	mInterpolation.clear().add( start, duration ).add( end ).setType( type );
}

void ResizeWidth::onStart() {
	onUpdate( Time::Zero );
}

void ResizeWidth::onUpdate( const Time& ) {
	if ( NULL != mNode ) {
		mNode->setSize( mInterpolation.getPosition(), mNode->getSize().getHeight() );
	}
}

Action* ResizeWidth::clone() const {
	ResizeWidth* action = eeNew( ResizeWidth, () );
	action->setInterpolation( mInterpolation );
	return action;
}

Action* ResizeWidth::reverse() const {
	ResizeWidth* action = eeNew( ResizeWidth, () );
	action->setInterpolation( Interpolation1d( mInterpolation.getReversePoints() ) );
	return action;
}

}}} // namespace EE::Scene::Actions
