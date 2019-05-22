#include <eepp/scene/actions/resizeborderradius.hpp>
#include <eepp/scene/node.hpp>
#include <eepp/ui/uiwidget.hpp>
using namespace EE::UI;

namespace EE { namespace Scene { namespace Actions {

ResizeBorderRadius * ResizeBorderRadius::New( const Float & start, const Float & end, const Time& duration, const Ease::Interpolation& type ) {
	return eeNew( ResizeBorderRadius, ( start, end, duration, type ) );
}

ResizeBorderRadius::ResizeBorderRadius()
{}

ResizeBorderRadius::ResizeBorderRadius( const Float & start, const Float & end, const Time& duration, const Ease::Interpolation& type ) {
	mInterpolation.clear().add( start, duration ).add( end ).setType( type );
}

void ResizeBorderRadius::onStart() {
	onUpdate( Time::Zero );
}

void ResizeBorderRadius::onUpdate( const Time& ) {
	if ( NULL != mNode && mNode->isWidget() ) {
		UIWidget * widget = static_cast<UIWidget*>( mNode );

		widget->setBorderRadius( mInterpolation.getPosition() );
	}
}

Action * ResizeBorderRadius::clone() const {
	ResizeBorderRadius * action = eeNew( ResizeBorderRadius, () );
	action->setInterpolation( mInterpolation );
	return action;
}

Action * ResizeBorderRadius::reverse() const {
	ResizeBorderRadius * action = eeNew( ResizeBorderRadius, () );
	action->setInterpolation( Interpolation1d( mInterpolation.getReversePoints() ) );
	return action;
}

}}}
