#include <eepp/scene/actions/paddingtransition.hpp>
#include <eepp/ui/uiwidget.hpp>
using namespace EE::UI;

namespace EE { namespace Scene { namespace Actions {

PaddingTransition * PaddingTransition::New( const Rectf& start, const Rectf& end, const Time& duration, const Ease::Interpolation& type ) {
	return eeNew( PaddingTransition, ( start, end, duration, type ) );
}

PaddingTransition::PaddingTransition()
{}

PaddingTransition::PaddingTransition( const Rectf& start, const Rectf& end, const Time& duration, const Ease::Interpolation& type ) :
	MarginMove()
{
	mInterpolationLeft.clear().add( start.Left, duration ).add( end.Left ).setType( type );
	mInterpolationRight.clear().add( start.Right, duration ).add( end.Right ).setType( type );
	mInterpolationTop.clear().add( start.Top, duration ).add( end.Top ).setType( type );
	mInterpolationBottom.clear().add( start.Bottom, duration ).add( end.Bottom ).setType( type );
}

void PaddingTransition::onUpdate(const Time &) {
	if ( NULL != mNode && mNode->isWidget() ) {
		static_cast<UIWidget*>( mNode )->setPadding(
											Rectf( mInterpolationLeft.getPosition(),
												  mInterpolationTop.getPosition(),
												  mInterpolationRight.getPosition(),
												  mInterpolationBottom.getPosition()
											) );
	}
}

Action * PaddingTransition::clone() const {
	PaddingTransition * action = eeNew( PaddingTransition, () );
	action->setInterpolationLeft( mInterpolationLeft );
	action->setInterpolationRight( mInterpolationRight );
	action->setInterpolationTop( mInterpolationTop );
	action->setInterpolationBottom( mInterpolationBottom );
	return action;
}

}}}
