#include <eepp/scene/actions/move.hpp>
#include <eepp/ui/uinode.hpp>

namespace EE { namespace Scene { namespace Actions {

Move * Move::New( const Vector2f& start, const Vector2f& end, const Time& duration, const Ease::Interpolation& type ) {
	return eeNew( Move, ( start, end, duration, type ) );
}

Move::Move()
{}

Move::Move( const Vector2f & start, const Vector2f & end, const Time& duration, const Ease::Interpolation& type ) {
	mInterpolation.clear().add( start, duration ).add( end ).setType( type );
}

void Move::onStart() {
	if ( NULL != mNode ) {
		mNode->setPosition( mInterpolation.getPosition() );
	}
}

void Move::onUpdate( const Time& time ) {
	if ( NULL != mNode ) {
		mNode->setPosition( mInterpolation.getPosition() );
	}
}

Action * Move::clone() const {
	Move * action = eeNew( Move, () );
	action->setInterpolation( mInterpolation );
	return action;
}

Action * Move::reverse() const {
	Move * action = eeNew( Move, () );
	action->setInterpolation( Interpolation2d( mInterpolation.getReversePoints() ) );
	return action;
}

}}} 
