#include <eepp/ui/actions/fade.hpp>
#include <eepp/ui/uinode.hpp>

namespace EE { namespace UI { namespace Action {

Fade * Fade::New( const Float & start, const Float & end, const Time& duration, const Ease::Interpolation& type, const bool& alphaChilds ) {
	return eeNew( Fade, ( start, end, duration, type, alphaChilds ) );
}

Fade::Fade() :
	mAffectChilds( true )
{}

Fade::Fade( const Float & start, const Float & end, const Time& duration, const Ease::Interpolation& type, const bool& alphaChilds ) :
	mAffectChilds( alphaChilds )
{
	mInterpolation.clear().add( start, duration ).add( end ).setType( type );
}

void Fade::onStart() {
	if ( NULL != mNode ) {
		mNode->setAlpha( mInterpolation.getPosition() );

		if ( mAffectChilds ) {
			UINode * CurChild = mNode->getFirstChild();

			while ( NULL != CurChild ) {
				CurChild->runAction( clone() );
				CurChild = CurChild->getNextNode();
			}
		}
	}
}

void Fade::onUpdate( const Time& time ) {
	if ( NULL != mNode ) {
		mNode->setAlpha( mInterpolation.getPosition() );
	}
}

UIAction * Fade::clone() const {
	Fade * action = eeNew( Fade, () );
	action->mAffectChilds = mAffectChilds;
	action->setInterpolation( mInterpolation );
	return action;
}

UIAction * Fade::reverse() const {
	Fade * action = eeNew( Fade, () );
	action->mAffectChilds = mAffectChilds;
	action->setInterpolation( Interpolation1d( mInterpolation.getReversePoints() ) );
	return action;
}

}}} 
