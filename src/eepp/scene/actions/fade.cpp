#include <eepp/scene/actions/fade.hpp>
#include <eepp/scene/node.hpp>

namespace EE { namespace Scene { namespace Actions {

Fade* Fade::New( const Float& start, const Float& end, const Time& duration,
				 const Ease::Interpolation& type, const bool& alphaChildren ) {
	return eeNew( Fade, ( start, end, duration, type, alphaChildren ) );
}

Fade::Fade() : mAffectChildren( true ) {}

Fade::Fade( const Float& start, const Float& end, const Time& duration,
			const Ease::Interpolation& type, const bool& alphaChildren ) :
	mAffectChildren( alphaChildren ) {
	mInterpolation.clear().add( start, duration ).add( end ).setType( type );
}

void Fade::onStart() {
	if ( NULL != mNode ) {
		mNode->setAlpha( mInterpolation.getPosition() );

		if ( mAffectChildren ) {
			Node* CurChild = mNode->getFirstChild();

			while ( NULL != CurChild ) {
				CurChild->runAction( clone() );
				CurChild = CurChild->getNextNode();
			}
		}
	}
}

void Fade::onUpdate( const Time& ) {
	if ( NULL != mNode ) {
		mNode->setAlpha( mInterpolation.getPosition() );
	}
}

Action* Fade::clone() const {
	Fade* action = eeNew( Fade, () );
	action->mAffectChildren = mAffectChildren;
	action->setInterpolation( mInterpolation );
	return action;
}

Action* Fade::reverse() const {
	Fade* action = eeNew( Fade, () );
	action->mAffectChildren = mAffectChildren;
	action->setInterpolation( Interpolation1d( mInterpolation.getReversePoints() ) );
	return action;
}

FadeIn* FadeIn::New( const Time& duration, const Ease::Interpolation& type,
					 const bool& alphaChildren ) {
	return eeNew( FadeIn, ( duration, type, alphaChildren ) );
}

Action* FadeIn::clone() const {
	FadeIn* action = eeNew( FadeIn, () );
	action->mAffectChildren = mAffectChildren;
	action->mDuration = mDuration;
	action->mType = mType;
	action->setInterpolation( mInterpolation );
	return action;
}

Action* FadeIn::reverse() const {
	FadeIn* action = eeNew( FadeIn, () );
	action->mAffectChildren = mAffectChildren;
	action->mDuration = mDuration;
	action->mType = mType;
	action->setInterpolation( Interpolation1d( mInterpolation.getReversePoints() ) );
	return action;
}

void FadeIn::start() {
	mInterpolation.clear().add( mNode->getAlpha(), mDuration ).add( 255.f ).setType( mType );

	Fade::start();
}

FadeIn::FadeIn() : Fade() {}

FadeIn::FadeIn( const Time& duration, const Ease::Interpolation& type, const bool& alphaChildren ) :
	Fade(), mDuration( duration ), mType( type ) {
	mAffectChildren = alphaChildren;
}

FadeOut* FadeOut::New( const Time& duration, const Ease::Interpolation& type,
					   const bool& alphaChildren ) {
	return eeNew( FadeOut, ( duration, type, alphaChildren ) );
}

Action* FadeOut::clone() const {
	FadeOut* action = eeNew( FadeOut, () );
	action->mAffectChildren = mAffectChildren;
	action->mDuration = mDuration;
	action->mType = mType;
	action->setInterpolation( mInterpolation );
	return action;
}

Action* FadeOut::reverse() const {
	FadeOut* action = eeNew( FadeOut, () );
	action->mAffectChildren = mAffectChildren;
	action->mDuration = mDuration;
	action->mType = mType;
	action->setInterpolation( Interpolation1d( mInterpolation.getReversePoints() ) );
	return action;
}

void FadeOut::start() {
	mInterpolation.clear().add( mNode->getAlpha(), mDuration ).add( 0.f ).setType( mType );

	Fade::start();
}

FadeOut::FadeOut() : Fade() {}

FadeOut::FadeOut( const Time& duration, const Ease::Interpolation& type,
				  const bool& alphaChildren ) :
	Fade(), mDuration( duration ), mType( type ) {
	mAffectChildren = alphaChildren;
}

}}} // namespace EE::Scene::Actions
