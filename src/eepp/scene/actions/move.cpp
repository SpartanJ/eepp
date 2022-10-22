#include <eepp/scene/actions/move.hpp>
#include <eepp/scene/node.hpp>
#include <eepp/ui/uinode.hpp>

using namespace EE::UI;

namespace EE { namespace Scene { namespace Actions {

Move* Move::New( const Vector2f& start, const Vector2f& end, const Time& duration,
				 const Ease::Interpolation& type ) {
	return eeNew( Move, ( start, end, duration, type ) );
}

Move::Move() {}

Move::Move( const Vector2f& start, const Vector2f& end, const Time& duration,
			const Ease::Interpolation& type ) {
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

Action* Move::clone() const {
	Move* action = eeNew( Move, () );
	action->setInterpolation( mInterpolation );
	return action;
}

Action* Move::reverse() const {
	Move* action = eeNew( Move, () );
	action->setInterpolation( Interpolation2d( mInterpolation.getReversePoints() ) );
	return action;
}

MoveCoordinate* MoveCoordinate::New( const Float& start, const Float& end, const Time& duration,
									 const Ease::Interpolation& type,
									 const MoveCoordinate::CoordinateAxis& coordinateAxis,
									 const MoveCoordinate::CoordinateType& coordinateType ) {
	return eeNew( MoveCoordinate, ( start, end, duration, type, coordinateAxis, coordinateType ) );
}

Action* MoveCoordinate::clone() const {
	MoveCoordinate* action = eeNew( MoveCoordinate, () );
	action->setInterpolation( mInterpolation );
	return action;
}

Action* MoveCoordinate::reverse() const {
	MoveCoordinate* action = eeNew( MoveCoordinate, () );
	action->setInterpolation( Interpolation1d( mInterpolation.getReversePoints() ) );
	return action;
}

MoveCoordinate::MoveCoordinate( const Float& start, const Float& end, const Time& duration,
								const Ease::Interpolation& type,
								const MoveCoordinate::CoordinateAxis& coordinateAxis,
								const MoveCoordinate::CoordinateType& coordinateType ) :
	mCoordinateAxis( coordinateAxis ), mCoordinateType( coordinateType ) {
	mInterpolation.clear().add( start, duration ).add( end ).setType( type );
}

void MoveCoordinate::onStart() {
	onUpdate( Time::Zero );
}

void MoveCoordinate::onUpdate( const Time& ) {
	if ( NULL != mNode ) {
		if ( mCoordinateAxis == CoordinateX ) {
			if ( mCoordinateType == Position ) {
				mNode->setPosition( mInterpolation.getPosition(), mNode->getPosition().y );
			} else if ( mNode->isUINode() ) {
				UINode* widget = mNode->asType<UINode>();
				widget->setPixelsPosition( mInterpolation.getPosition(), mNode->getPosition().y );
			}
		} else if ( mCoordinateAxis == CoordinateY ) {
			if ( mCoordinateType == Position ) {
				mNode->setPosition( mNode->getPosition().x, mInterpolation.getPosition() );
			} else if ( mNode->isUINode() ) {
				UINode* widget = mNode->asType<UINode>();
				widget->setPixelsPosition( mNode->getPosition().x, mInterpolation.getPosition() );
			}
		}
	}
}

MoveCoordinate::MoveCoordinate() {}

}}} // namespace EE::Scene::Actions
