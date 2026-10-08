#include "utest.h"
#include <eepp/scene/actions/delay.hpp>
#include <eepp/scene/actions/runnable.hpp>
#include <eepp/ui/css/animationdefinition.hpp>
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

UTEST( Animations, allocationLightShorthandPreservesSemantics ) {
	auto* specification = StyleSheetSpecification::instance();
	StyleSheetProperty shorthand(
		specification->getProperty( PropertyId::Animation ),
		"fade +150MS CUBIC-BEZIER(0.1, 0.2, 0.3, 0.4) +25MS 3 alternate both paused, "
		"slide 2s linear" );
	std::vector<const StyleSheetProperty*> properties{ &shorthand };

	AnimationsMap animations = AnimationDefinition::parseAnimationProperties( properties );
	auto fade = animations.find( "fade" );
	auto slide = animations.find( "slide" );
	ASSERT_TRUE( fade != animations.end() );
	ASSERT_TRUE( slide != animations.end() );
	EXPECT_EQ( fade->second.getDuration().asMilliseconds(), 150.0 );
	EXPECT_EQ( fade->second.getDelay().asMilliseconds(), 25.0 );
	EXPECT_EQ( fade->second.getIterations(), 3 );
	EXPECT_TRUE( fade->second.getDirection() == AnimationDefinition::Alternate );
	EXPECT_TRUE( fade->second.getFillMode() == AnimationDefinition::Both );
	EXPECT_TRUE( fade->second.isPaused() );
	EXPECT_TRUE( fade->second.getTimingFunction() == Ease::CubizBezier );
	EXPECT_EQ( fade->second.getTimingFunctionParametersInline().size(), 4u );
	EXPECT_EQ( slide->second.getDuration().asSeconds(), 2.0 );
	EXPECT_EQ( slide->second.getDelay().asSeconds(), 0.0 );
}

UTEST( Animations, allocationLightLonghandListsCycleIndependently ) {
	auto* specification = StyleSheetSpecification::instance();
	StyleSheetProperty names( specification->getProperty( PropertyId::AnimationName ),
							  "first,second,third" );
	StyleSheetProperty durations( specification->getProperty( PropertyId::AnimationDuration ),
								  "1s,2s" );
	StyleSheetProperty delays( specification->getProperty( PropertyId::AnimationDelay ),
							   "10ms,20ms" );
	StyleSheetProperty directions( specification->getProperty( PropertyId::AnimationDirection ),
								   "reverse,alternate" );
	StyleSheetProperty timing( specification->getProperty( PropertyId::AnimationTimingFunction ),
							   "linear,cubic-bezier(0.1, 0.2, 0.3, 0.4)" );
	std::vector<const StyleSheetProperty*> properties{ &names, &durations, &delays, &directions,
													   &timing };

	AnimationsMap animations = AnimationDefinition::parseAnimationProperties( properties );
	ASSERT_EQ( animations.size(), 3u );
	EXPECT_EQ( animations.at( "first" ).getDuration().asSeconds(), 1.0 );
	EXPECT_EQ( animations.at( "second" ).getDuration().asSeconds(), 2.0 );
	EXPECT_EQ( animations.at( "third" ).getDuration().asSeconds(), 1.0 );
	EXPECT_EQ( animations.at( "third" ).getDelay().asMilliseconds(), 10.0 );
	EXPECT_TRUE( animations.at( "first" ).getDirection() == AnimationDefinition::Reverse );
	EXPECT_TRUE( animations.at( "second" ).getDirection() == AnimationDefinition::Alternate );
	EXPECT_TRUE( animations.at( "third" ).getDirection() == AnimationDefinition::Reverse );
	EXPECT_TRUE( animations.at( "first" ).getTimingFunction() == Ease::Linear );
	EXPECT_TRUE( animations.at( "second" ).getTimingFunction() == Ease::CubizBezier );
	EXPECT_EQ( animations.at( "second" ).getTimingFunctionParametersInline().size(), 4u );
}
