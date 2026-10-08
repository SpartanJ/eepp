#include "utest.h"
#include <eepp/ui/uiscrollcontroller.hpp>

using namespace EE;
using namespace EE::System;
using namespace EE::UI;

UTEST( UIScrollController, InstantScrollRetainsFractionalInputAndClamps ) {
	UIScrollController controller;
	const Vector2f maximum( 100.f, 200.f );

	EXPECT_TRUE( controller.scrollBy( { 4.25f, -20.f }, { 10.5f, 5.5f }, maximum, false ) );
	EXPECT_NEAR( 14.75f, controller.getPosition().x, 0.0001f );
	EXPECT_NEAR( 0.f, controller.getPosition().y, 0.0001f );
	EXPECT_FALSE( controller.isActive() );
	EXPECT_FALSE( controller.scrollBy( { 200.f, -1.f }, { 100.f, 0.f }, maximum, false ) );
}

UTEST( UIScrollController, SmoothScrollAccumulatesTargetsWithoutJumping ) {
	UIScrollController controller;
	const Vector2f maximum( 500.f, 500.f );

	EXPECT_TRUE( controller.scrollBy( { 0.f, 100.f }, Vector2f::Zero, maximum, true ) );
	EXPECT_TRUE( controller.scrollBy( { 0.f, 25.5f }, Vector2f::Zero, maximum, true ) );
	EXPECT_NEAR( 0.f, controller.getPosition().y, 0.0001f );
	EXPECT_NEAR( 125.5f, controller.getTarget().y, 0.0001f );
	EXPECT_TRUE( controller.isActive() );

	controller.update( Milliseconds( 16 ), maximum );
	EXPECT_GT( controller.getPosition().y, 0.f );
	EXPECT_LT( controller.getPosition().y, controller.getTarget().y );
	for ( size_t i = 0; i < 100; ++i )
		controller.update( Milliseconds( 16 ), maximum );
	EXPECT_NEAR( 125.5f, controller.getPosition().y, 0.0001f );
	EXPECT_FALSE( controller.isActive() );
}

UTEST( UIScrollController, SmoothWheelStepsOverlapInsteadOfExtendingGestureDuration ) {
	UIScrollController controller;
	const Vector2f maximum( 500.f, 500.f );
	controller.setWheelScrollDuration( Milliseconds( 100 ) );

	controller.scrollBy( { 0.f, 10.f }, Vector2f::Zero, maximum, true );
	controller.update( Milliseconds( 50 ), maximum );
	EXPECT_NEAR( 7.5f, controller.getPosition().y, 0.0001f );

	controller.scrollBy( { 0.f, 10.f }, controller.getPosition(), maximum, true );
	controller.update( Milliseconds( 50 ), maximum );
	EXPECT_NEAR( 17.5f, controller.getPosition().y, 0.0001f );
	EXPECT_TRUE( controller.isActive() );

	controller.update( Milliseconds( 50 ), maximum );
	EXPECT_NEAR( 20.f, controller.getPosition().y, 0.0001f );
	EXPECT_FALSE( controller.isActive() );
}

UTEST( UIScrollController, PartialWheelStepUsesProportionallyShorterTween ) {
	UIScrollController controller;
	const Vector2f maximum( 100.f, 100.f );
	controller.setWheelScrollDuration( Milliseconds( 100 ) );

	controller.scrollBy( { 0.f, 10.f }, { 0.f, 95.f }, maximum, true );
	controller.update( Milliseconds( 25 ), maximum );
	EXPECT_NEAR( 98.75f, controller.getPosition().y, 0.0001f );
	EXPECT_TRUE( controller.isActive() );

	controller.update( Milliseconds( 25 ), maximum );
	EXPECT_NEAR( 100.f, controller.getPosition().y, 0.0001f );
	EXPECT_FALSE( controller.isActive() );
}

UTEST( UIScrollController, SubTickInputScalesTweenDuration ) {
	UIScrollController controller;
	const Vector2f maximum( 100.f, 100.f );
	controller.setWheelScrollDuration( Milliseconds( 100 ) );

	controller.scrollBy( { 0.f, 2.5f }, Vector2f::Zero, maximum, true, 0.25f );
	controller.update( Milliseconds( 12.5 ), maximum );
	EXPECT_NEAR( 1.875f, controller.getPosition().y, 0.0001f );
	EXPECT_TRUE( controller.isActive() );

	controller.update( Milliseconds( 12.5 ), maximum );
	EXPECT_NEAR( 2.5f, controller.getPosition().y, 0.0001f );
	EXPECT_FALSE( controller.isActive() );
}

static UIScrollController smoothControllerAfter( size_t frames, const Time& elapsed ) {
	UIScrollController controller;
	controller.scrollBy( { 0.f, 300.f }, Vector2f::Zero, { 1000.f, 1000.f }, true );
	for ( size_t i = 0; i < frames; ++i )
		controller.update( elapsed, { 1000.f, 1000.f } );
	return controller;
}

UTEST( UIScrollController, SmoothMotionIsFrameRateIndependent ) {
	const auto atSixtyHz = smoothControllerAfter( 15, Seconds( 1. / 60. ) );
	const auto atOneTwentyHz = smoothControllerAfter( 30, Seconds( 1. / 120. ) );
	EXPECT_NEAR( atSixtyHz.getPosition().y, atOneTwentyHz.getPosition().y, 0.001f );
}

static UIScrollController momentumControllerAfter( size_t frames, const Time& elapsed ) {
	UIScrollController controller;
	const Vector2f maximum( 1000.f, 1000.f );
	controller.beginDrag( { 500.f, 500.f }, maximum );
	controller.dragBy( { 0.f, -20.f }, Milliseconds( 16 ), maximum );
	controller.endDrag();
	for ( size_t i = 0; i < frames; ++i )
		controller.update( elapsed, maximum );
	return controller;
}

UTEST( UIScrollController, DragReleaseProducesFrameRateIndependentMomentum ) {
	const auto atSixtyHz = momentumControllerAfter( 30, Seconds( 1. / 60. ) );
	const auto atOneTwentyHz = momentumControllerAfter( 60, Seconds( 1. / 120. ) );
	EXPECT_LT( atSixtyHz.getPosition().y, 480.f );
	EXPECT_NEAR( atSixtyHz.getPosition().y, atOneTwentyHz.getPosition().y, 0.001f );
	EXPECT_NEAR( atSixtyHz.getVelocity().y, atOneTwentyHz.getVelocity().y, 0.001f );
}

UTEST( UIScrollController, MomentumStopsAtScrollBounds ) {
	UIScrollController controller;
	const Vector2f maximum( 100.f, 100.f );
	controller.beginDrag( { 50.f, 5.f }, maximum );
	controller.dragBy( { 0.f, -20.f }, Milliseconds( 16 ), maximum );
	controller.endDrag();
	controller.update( Milliseconds( 16 ), maximum );

	EXPECT_NEAR( 0.f, controller.getPosition().y, 0.0001f );
	EXPECT_NEAR( 0.f, controller.getVelocity().y, 0.0001f );
	EXPECT_FALSE( controller.isActive() );
}
