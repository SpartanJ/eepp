#include <eepp/scene/actions/fade.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene { namespace Actions {

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
			Node * CurChild = mNode->getFirstChild();

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

Action * Fade::clone() const {
	Fade * action = eeNew( Fade, () );
	action->mAffectChilds = mAffectChilds;
	action->setInterpolation( mInterpolation );
	return action;
}

Action * Fade::reverse() const {
	Fade * action = eeNew( Fade, () );
	action->mAffectChilds = mAffectChilds;
	action->setInterpolation( Interpolation1d( mInterpolation.getReversePoints() ) );
	return action;
}

FadeIn * FadeIn::New(const Time & duration, const Ease::Interpolation & type, const bool & alphaChilds) {
	return eeNew( FadeIn, ( duration, type, alphaChilds ) );
}

Action * FadeIn::clone() const {
	FadeIn * action = eeNew( FadeIn, () );
	action->mAffectChilds = mAffectChilds;
	action->mDuration = mDuration;
	action->mType = mType;
	action->setInterpolation( mInterpolation );
	return action;
}

Action * FadeIn::reverse() const {
	FadeIn * action = eeNew( FadeIn, () );
	action->mAffectChilds = mAffectChilds;
	action->mDuration = mDuration;
	action->mType = mType;
	action->setInterpolation( Interpolation1d( mInterpolation.getReversePoints() ) );
	return action;
}

void FadeIn::start() {
	mInterpolation.clear().add( mNode->getAlpha(), mDuration ).add( 255.f ).setType( mType );

	Fade::start();
}

FadeIn::FadeIn() :
	Fade()
{}

FadeIn::FadeIn( const Time & duration, const Ease::Interpolation & type, const bool& alphaChilds ) :
	Fade(),
	mDuration( duration ),
	mType( type )
{
	mAffectChilds = alphaChilds;
}

FadeOut * FadeOut::New(const Time & duration, const Ease::Interpolation & type, const bool & alphaChilds) {
	return eeNew( FadeOut, ( duration, type, alphaChilds ) );
}

Action * FadeOut::clone() const {
	FadeOut * action = eeNew( FadeOut, () );
	action->mAffectChilds = mAffectChilds;
	action->mDuration = mDuration;
	action->mType = mType;
	action->setInterpolation( mInterpolation );
	return action;
}

Action * FadeOut::reverse() const {
	FadeOut * action = eeNew( FadeOut, () );
	action->mAffectChilds = mAffectChilds;
	action->mDuration = mDuration;
	action->mType = mType;
	action->setInterpolation( Interpolation1d( mInterpolation.getReversePoints() ) );
	return action;
}

void FadeOut::start() {
	mInterpolation.clear().add( mNode->getAlpha(), mDuration ).add( 0.f ).setType( mType );

	Fade::start();
}

FadeOut::FadeOut() :
	Fade()
{}

FadeOut::FadeOut( const Time & duration, const Ease::Interpolation & type, const bool& alphaChilds ) :
	Fade(),
	mDuration( duration ),
	mType( type )
{
	mAffectChilds = alphaChilds;
}

}}} 
