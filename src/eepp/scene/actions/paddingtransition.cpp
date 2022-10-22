#include <eepp/scene/actions/paddingtransition.hpp>
#include <eepp/ui/uiwidget.hpp>
using namespace EE::UI;

namespace EE { namespace Scene { namespace Actions {

PaddingTransition* PaddingTransition::New( const Rectf& start, const Rectf& end,
										   const Time& duration, const Ease::Interpolation& type,
										   const Uint32& interpolateFlag ) {
	return eeNew( PaddingTransition, ( start, end, duration, type, interpolateFlag ) );
}

PaddingTransition::PaddingTransition() {}

PaddingTransition::PaddingTransition( const Rectf& start, const Rectf& end, const Time& duration,
									  const Ease::Interpolation& type,
									  const Uint32& interpolateFlag ) :
	MarginMove() {
	mFlags = interpolateFlag;

	if ( mFlags & InterpolateFlag::Left )
		mInterpolationLeft.clear().add( start.Left, duration ).add( end.Left ).setType( type );

	if ( mFlags & InterpolateFlag::Right )
		mInterpolationRight.clear().add( start.Right, duration ).add( end.Right ).setType( type );

	if ( mFlags & InterpolateFlag::Top )
		mInterpolationTop.clear().add( start.Top, duration ).add( end.Top ).setType( type );

	if ( mFlags & InterpolateFlag::Bottom )
		mInterpolationBottom.clear()
			.add( start.Bottom, duration )
			.add( end.Bottom )
			.setType( type );
}

void PaddingTransition::onUpdate( const Time& ) {
	if ( NULL != mNode && mNode->isWidget() ) {
		UIWidget* widget = static_cast<UIWidget*>( mNode );

		widget->setPadding(
			Rectf( ( mFlags & InterpolateFlag::Left ) ? mInterpolationLeft.getPosition()
													  : widget->getPadding().Left,
				   ( mFlags & InterpolateFlag::Top ) ? mInterpolationTop.getPosition()
													 : widget->getPadding().Top,
				   ( mFlags & InterpolateFlag::Right ) ? mInterpolationRight.getPosition()
													   : widget->getPadding().Right,
				   ( mFlags & InterpolateFlag::Bottom ) ? mInterpolationBottom.getPosition()
														: widget->getPadding().Bottom ) );
	}
}

Action* PaddingTransition::clone() const {
	PaddingTransition* action = eeNew( PaddingTransition, () );
	action->mFlags = mFlags;
	action->setInterpolationLeft( mInterpolationLeft );
	action->setInterpolationRight( mInterpolationRight );
	action->setInterpolationTop( mInterpolationTop );
	action->setInterpolationBottom( mInterpolationBottom );
	return action;
}

}}} // namespace EE::Scene::Actions
