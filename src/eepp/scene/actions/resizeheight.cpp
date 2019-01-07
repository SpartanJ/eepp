#include <eepp/scene/actions/resizeheight.hpp>
#include <eepp/scene/node.hpp>
#include <eepp/ui/uiwidget.hpp>
using namespace EE::UI;

namespace EE { namespace Scene { namespace Actions {

ResizeHeight * ResizeHeight::New( const Float & start, const Float & end, const Time& duration, const Ease::Interpolation& type ) {
	return eeNew( ResizeHeight, ( start, end, duration, type ) );
}

ResizeHeight::ResizeHeight()
{}

ResizeHeight::ResizeHeight( const Float & start, const Float & end, const Time& duration, const Ease::Interpolation& type ) {
	mInterpolation.clear().add( start, duration ).add( end ).setType( type );
}

void ResizeHeight::onStart() {
	onUpdate( Time::Zero );
}

void ResizeHeight::onUpdate( const Time& ) {
	if ( NULL != mNode ) {
		mNode->setSize( mNode->getSize().getWidth(), mInterpolation.getPosition() );

		/** TODO: Remove this when onSizeChange notifyLayoutAttrChange calls is enabled */
		if ( mNode->isWidget() ) {
			static_cast<UIWidget*>( mNode )->notifyLayoutAttrChange();
		}
	}
}

Action * ResizeHeight::clone() const {
	ResizeHeight * action = eeNew( ResizeHeight, () );
	action->setInterpolation( mInterpolation );
	return action;
}

Action * ResizeHeight::reverse() const {
	ResizeHeight * action = eeNew( ResizeHeight, () );
	action->setInterpolation( Interpolation1d( mInterpolation.getReversePoints() ) );
	return action;
}

}}}
