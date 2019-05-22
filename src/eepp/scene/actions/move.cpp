#include <eepp/scene/actions/move.hpp>
#include <eepp/scene/node.hpp>

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
	onUpdate( Time::Zero );
}

void Move::onUpdate( const Time& ) {
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

MoveCoordinate * MoveCoordinate::New( const Float& start, const Float& end, const Time& duration, const Ease::Interpolation& type, const MoveCoordinate::CoordinateType& coordinateType ) {
	return eeNew( MoveCoordinate, ( start, end, duration, type, coordinateType ) );
}

Action * MoveCoordinate::clone() const {
	MoveCoordinate * action = eeNew( MoveCoordinate, () );
	action->setInterpolation( mInterpolation );
	return action;
}

Action * MoveCoordinate::reverse() const {
	MoveCoordinate * action = eeNew( MoveCoordinate, () );
	action->setInterpolation( Interpolation1d( mInterpolation.getReversePoints() ) );
	return action;
}

MoveCoordinate::MoveCoordinate( const Float& start, const Float& end, const Time& duration, const Ease::Interpolation& type, const MoveCoordinate::CoordinateType& coordinateType ) :
	mCoordinateType( coordinateType )
{
	mInterpolation.clear().add( start, duration ).add( end ).setType( type );
}

void MoveCoordinate::onStart() {
	onUpdate( Time::Zero );
}

void MoveCoordinate::onUpdate( const Time& ) {
	if ( NULL != mNode ) {
		if ( mCoordinateType == CoordinateX )
			mNode->setPosition( mInterpolation.getPosition(), mNode->getPosition().y );
		else if ( mCoordinateType == CoordinateY )
			mNode->setPosition( mNode->getPosition().x, mInterpolation.getPosition() );
	}
}

MoveCoordinate::MoveCoordinate()
{}

}}} 
