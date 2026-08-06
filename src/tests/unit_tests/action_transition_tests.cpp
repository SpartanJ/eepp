#include "utest.h"
#include <eepp/scene/actions/delay.hpp>
#include <eepp/scene/actions/runnable.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/css/transitiondefinition.hpp>

using namespace EE;
using namespace EE::Math;
using namespace EE::Scene;
using namespace EE::Scene::Actions;
using namespace EE::System;
using namespace EE::UI::CSS;

UTEST( ActionPool, reusesCompletedActionSlots ) {
	Delay* first = Delay::New( Milliseconds( 1 ) );
	void* firstAddress = first;
	eeDelete( first );

	Delay* second = Delay::New( Milliseconds( 2 ) );
	EXPECT_EQ( static_cast<void*>( second ), firstAddress );
	eeDelete( second );
}

UTEST( ActionCallbacks, listenerMutationUsesEventSnapshot ) {
	Delay* action = Delay::New( Time::Zero );
	int callbackCount = 0;
	Uint32 firstCallback = 0;
	Uint32 secondCallback = 0;
	firstCallback = action->addEventListener( Action::OnStart, [&]( Action* sender, const auto& ) {
		++callbackCount;
		sender->removeEventListener( secondCallback );
	} );
	secondCallback = action->addEventListener( Action::OnStart, [&]( Action* sender, const auto& ) {
		++callbackCount;
		sender->removeEventListener( firstCallback );
	} );

	action->start();
	EXPECT_EQ( callbackCount, 2 );
	eeDelete( action );
}

UTEST( Transitions, compactShorthandParsesWithoutLosingSemantics ) {
	auto* specification = StyleSheetSpecification::instance();
	StyleSheetProperty shorthand(
		specification->getProperty( PropertyId::Transition ),
		"opacity 150ms cubic-bezier(0.1, 0.2, 0.3, 0.4) 25ms, width 2s linear" );
	std::vector<const StyleSheetProperty*> properties{ &shorthand };

	ComputedTransitions transitions = ComputedTransitions::parse( properties );
	const auto* opacity = transitions.get( String::hash( "opacity" ) );
	const auto* width = transitions.get( String::hash( "width" ) );
	ASSERT_TRUE( nullptr != opacity );
	ASSERT_TRUE( nullptr != width );
	EXPECT_EQ( opacity->duration.asMilliseconds(), 150.0 );
	EXPECT_EQ( opacity->delay.asMilliseconds(), 25.0 );
	EXPECT_TRUE( opacity->timingFunction == Ease::CubizBezier );
	ASSERT_EQ( opacity->timingFunctionParameters.size(), 4u );
	EXPECT_EQ( opacity->timingFunctionParameters[2], 0.3 );
	EXPECT_EQ( width->duration.asSeconds(), 2.0 );
	EXPECT_TRUE( width->timingFunction == Ease::Linear );
}

UTEST( Transitions, longhandListsCycleIndependently ) {
	auto* specification = StyleSheetSpecification::instance();
	StyleSheetProperty property( specification->getProperty( PropertyId::TransitionProperty ),
								 "opacity,width,height" );
	StyleSheetProperty duration( specification->getProperty( PropertyId::TransitionDuration ),
								 "1s,2s" );
	StyleSheetProperty delay( specification->getProperty( PropertyId::TransitionDelay ),
							  "10ms,20ms" );
	StyleSheetProperty timing( specification->getProperty( PropertyId::TransitionTimingFunction ),
							   "linear,quadratic-in,bounce-out" );
	std::vector<const StyleSheetProperty*> properties{ &property, &duration, &delay, &timing };

	ComputedTransitions transitions = ComputedTransitions::parse( properties );
	const auto* opacity = transitions.get( String::hash( "opacity" ) );
	const auto* width = transitions.get( String::hash( "width" ) );
	const auto* height = transitions.get( String::hash( "height" ) );
	ASSERT_TRUE( nullptr != opacity );
	ASSERT_TRUE( nullptr != width );
	ASSERT_TRUE( nullptr != height );
	EXPECT_EQ( opacity->duration.asSeconds(), 1.0 );
	EXPECT_EQ( width->duration.asSeconds(), 2.0 );
	EXPECT_EQ( height->duration.asSeconds(), 1.0 );
	EXPECT_EQ( height->delay.asMilliseconds(), 10.0 );
	EXPECT_TRUE( opacity->timingFunction == Ease::Linear );
	EXPECT_TRUE( width->timingFunction == Ease::QuadraticIn );
	EXPECT_TRUE( height->timingFunction == Ease::BounceOut );

	TransitionsMap compatibility = TransitionDefinition::parseTransitionProperties( properties );
	EXPECT_TRUE( compatibility["height"].getTimingFunction() == Ease::BounceOut );
}

UTEST( Transitions, allIsFallbackAndSpecificPropertyWins ) {
	auto* specification = StyleSheetSpecification::instance();
	StyleSheetProperty shorthand( specification->getProperty( PropertyId::Transition ),
								  "all 1s linear, opacity 2s sine-in" );
	std::vector<const StyleSheetProperty*> properties{ &shorthand };
	ComputedTransitions transitions = ComputedTransitions::parse( properties );

	const auto* width = transitions.get( String::hash( "width" ) );
	const auto* opacity = transitions.get( String::hash( "opacity" ) );
	ASSERT_TRUE( nullptr != width );
	ASSERT_TRUE( nullptr != opacity );
	EXPECT_EQ( width->duration.asSeconds(), 1.0 );
	EXPECT_EQ( opacity->duration.asSeconds(), 2.0 );
	EXPECT_TRUE( opacity->timingFunction == Ease::SineIn );
}

UTEST( Transitions, allocationLightParserPreservesAcceptedSpelling ) {
	auto* specification = StyleSheetSpecification::instance();
	StyleSheetProperty shorthand( specification->getProperty( PropertyId::Transition ),
								  "opacity +150MS CUBIC-BEZIER(0.1, 0.2, 0.3, 0.4) +25MS" );
	std::vector<const StyleSheetProperty*> properties{ &shorthand };

	ComputedTransitions transitions = ComputedTransitions::parse( properties );
	const auto* opacity = transitions.get( String::hash( "opacity" ) );
	ASSERT_TRUE( nullptr != opacity );
	EXPECT_EQ( opacity->duration.asMilliseconds(), 150.0 );
	EXPECT_EQ( opacity->delay.asMilliseconds(), 25.0 );
	EXPECT_TRUE( opacity->timingFunction == Ease::CubizBezier );
	EXPECT_EQ( opacity->timingFunctionParameters.size(), 4u );
	EXPECT_TRUE( TimingFunction::parse( "cubic-bezier(" ).interpolation == Ease::None );
}
