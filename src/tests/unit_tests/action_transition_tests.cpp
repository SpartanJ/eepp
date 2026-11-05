#include "utest.h"
#include <eepp/math/easing.hpp>
#include <eepp/scene/actions/delay.hpp>
#include <eepp/scene/actions/runnable.hpp>
#include <eepp/system/filesystem.hpp>
#include <eepp/system/sys.hpp>
#include <eepp/ui/css/animationdefinition.hpp>
#include <eepp/ui/css/stylesheetpropertyanimation.hpp>
#include <eepp/ui/css/stylesheetspecification.hpp>
#include <eepp/ui/css/timingfunction.hpp>
#include <eepp/ui/css/transitiondefinition.hpp>
#include <eepp/ui/uiapplication.hpp>
#include <eepp/ui/uiscenenode.hpp>
#include <eepp/ui/uiwidget.hpp>
#include <eepp/window/window.hpp>
#include <limits>

using namespace EE;
using namespace EE::Math;
using namespace EE::Scene;
using namespace EE::Scene::Actions;
using namespace EE::System;
using namespace EE::UI;
using namespace EE::UI::CSS;
using namespace EE::Window;
using namespace EE::Math::easing;

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

UTEST( Animations, stepsFollowEveryJumpPosition ) {
	const auto steps = []( StepPosition position, double t ) {
		return stepsInterpolation( 4, position, t );
	};
	EXPECT_NEAR( steps( StepPosition::JumpEnd, 0.0 ), 0.0, 1e-9 );
	EXPECT_NEAR( steps( StepPosition::JumpEnd, 0.24 ), 0.0, 1e-9 );
	EXPECT_NEAR( steps( StepPosition::JumpEnd, 0.25 ), 0.25, 1e-9 );
	EXPECT_NEAR( steps( StepPosition::JumpEnd, 0.99 ), 0.75, 1e-9 );
	EXPECT_NEAR( steps( StepPosition::JumpEnd, 1.0 ), 1.0, 1e-9 );
	EXPECT_NEAR( steps( StepPosition::JumpStart, 0.0 ), 0.25, 1e-9 );
	EXPECT_NEAR( steps( StepPosition::JumpStart, 0.99 ), 1.0, 1e-9 );
	EXPECT_NEAR( steps( StepPosition::JumpStart, 1.0 ), 1.0, 1e-9 );
	EXPECT_NEAR( steps( StepPosition::JumpNone, 0.0 ), 0.0, 1e-9 );
	EXPECT_NEAR( steps( StepPosition::JumpNone, 0.5 ), 2.0 / 3.0, 1e-9 );
	EXPECT_NEAR( steps( StepPosition::JumpNone, 0.99 ), 1.0, 1e-9 );
	EXPECT_NEAR( steps( StepPosition::JumpBoth, 0.0 ), 0.2, 1e-9 );
	EXPECT_NEAR( steps( StepPosition::JumpBoth, 0.99 ), 0.8, 1e-9 );
	EXPECT_NEAR( steps( StepPosition::JumpBoth, 1.0 ), 1.0, 1e-9 );
}

UTEST( Animations, stepsParsing ) {
	const auto expectSteps = [&]( const char* text, int count, StepPosition position ) {
		const auto parsed = TimingFunction::parse( text );
		EXPECT_TRUE( parsed.interpolation == Ease::Steps );
		ASSERT_EQ( parsed.parameters.size(), 2u );
		EXPECT_EQ( static_cast<int>( parsed.parameters[0] ), count );
		EXPECT_TRUE( static_cast<StepPosition>( parsed.parameters[1] ) == position );
	};
	expectSteps( "steps(8)", 8, StepPosition::JumpEnd );
	expectSteps( " STEPS( 8 , END ) ", 8, StepPosition::JumpEnd );
	expectSteps( "steps(3, jump-end)", 3, StepPosition::JumpEnd );
	expectSteps( "steps(3, start)", 3, StepPosition::JumpStart );
	expectSteps( "steps(3, jump-start)", 3, StepPosition::JumpStart );
	expectSteps( "steps(2, jump-none)", 2, StepPosition::JumpNone );
	expectSteps( "steps(1, jump-both)", 1, StepPosition::JumpBoth );
	expectSteps( "step-start", 1, StepPosition::JumpStart );
	expectSteps( "step-end", 1, StepPosition::JumpEnd );
	for ( const char* invalid :
		  { "steps", "steps()", "steps(0)", "steps(-1)", "steps(1.5)", "steps(a)", "steps(,end)",
			"steps(2, middle)", "steps(2, end, end)", "steps(1, jump-none)", "steps(2," } ) {
		EXPECT_TRUE( TimingFunction::parse( invalid ).interpolation == Ease::None );
	}

	auto* specification = StyleSheetSpecification::instance();
	StyleSheetProperty shorthand( specification->getProperty( PropertyId::Animation ),
								  "spin 2s steps(12, jump-none) infinite" );
	std::vector<const StyleSheetProperty*> properties{ &shorthand };
	AnimationsMap animations = AnimationDefinition::parseAnimationProperties( properties );
	const auto spin = animations.find( "spin" );
	ASSERT_TRUE( spin != animations.end() );
	EXPECT_TRUE( spin->second.getTimingFunction() == Ease::Steps );
	EXPECT_EQ( spin->second.getTimingFunctionParametersInline().size(), 2u );
	EXPECT_EQ( spin->second.getIterations(), -1 );
}

UTEST( Animations, timingFunctionsSpanTheWholeValueRange ) {
	UIApplication app(
		WindowSettings( 200, 200, "eepp - Timing functions", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* widget = app.getUI()->loadLayoutFromString( "<widget />" );
	ASSERT_TRUE( widget != nullptr );
	const auto* rotation = StyleSheetSpecification::instance()->getProperty( "rotation" );

	const auto rotationAt = [&]( const char* timing, Float progress ) {
		const auto function = TimingFunction::parse( timing );
		StyleSheetPropertyAnimation::tweenProperty( widget, progress, rotation, "40", "280",
													function.interpolation, function.parameters, 0,
													false );
		return widget->getRotation();
	};
	EXPECT_NEAR( rotationAt( "cubic-bezier(0, 0, 1, 1)", 0.f ), 40.f, 0.01f );
	EXPECT_NEAR( rotationAt( "cubic-bezier(0, 0, 1, 1)", 0.5f ), 160.f, 0.01f );
	EXPECT_NEAR( rotationAt( "cubic-bezier(0, 0, 1, 1)", 1.f ), 280.f, 0.01f );
	EXPECT_NEAR( rotationAt( "cubic-bezier(0.2, 0.1, 0.2, 0.1)", 1.f ), 280.f, 0.01f );
	EXPECT_NEAR( rotationAt( "cubic-bezier", 0.5f ), 160.f, 0.01f );
	EXPECT_NEAR( rotationAt( "steps(4)", 0.3f ), 100.f, 0.01f );
	EXPECT_NEAR( rotationAt( "steps(4)", 1.f ), 280.f, 0.01f );
}

UTEST( Animations, steppedAnimationsOnlyInvalidateOnNewVisibleSteps ) {
	UIApplication app(
		WindowSettings( 200, 200, "eepp - Stepped animation", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* scene = app.getUI();
	auto* root = scene->loadLayoutFromString( R"xml(
		<vbox id="parent">
			<style>
				@keyframes spin {
					from { rotation: 0; }
					to { rotation: 360; }
				}
				#stepped { animation: spin 800ms steps(8) infinite; }
			</style>
			<widget id="stepped" />
		</vbox>
	)xml" );
	ASSERT_TRUE( root != nullptr );
	auto* stepped = root->find<UIWidget>( "stepped" );
	ASSERT_TRUE( stepped != nullptr );

	const auto frame = Milliseconds( 10 );
	for ( int settle = 0; settle < 5; ++settle )
		scene->update( frame );

	int changes = 0;
	stepped->on( Event::OnAngleChange, [&changes]( const Event* ) { ++changes; } );
	for ( int update = 0; update < 160; ++update ) // Two loops: 16 steps in 160 frames.
		scene->update( frame );
	// A frame landing exactly on the end of a loop also applies the last step before wrapping.
	EXPECT_TRUE( changes >= 15 && changes <= 18 );
	EXPECT_NEAR( std::fmod( stepped->getRotation(), 45.f ), 0.f, 0.01f );

	changes = 0;
	root->setVisible( false );
	for ( int update = 0; update < 100; ++update )
		scene->update( frame );
	EXPECT_EQ( changes, 0 );

	root->setVisible( true );
	for ( int update = 0; update < 20; ++update )
		scene->update( frame );
	EXPECT_TRUE( changes >= 1 && changes <= 3 );
}

UTEST( Animations, stepsSurviveTheLargestCounts ) {
	const int count = std::numeric_limits<int>::max();
	EXPECT_NEAR( stepsInterpolation( count, StepPosition::JumpBoth, 1.0 ), 1.0, 1e-9 );
	EXPECT_NEAR( stepsInterpolation( count, StepPosition::JumpStart, 1.0 ), 1.0, 1e-9 );
	EXPECT_NEAR( stepsInterpolation( count, StepPosition::JumpEnd, 0.5 ), 0.5, 1e-6 );
	EXPECT_NEAR( stepsInterpolation( count, StepPosition::JumpNone, 1.0 ), 1.0, 1e-9 );
}

UTEST( Animations, steppedRelativeValuesFollowLayout ) {
	UIApplication app(
		WindowSettings( 400, 200, "eepp - Stepped relative value", WindowStyle::Default,
						WindowBackend::Default, 32 ),
		UIApplication::Settings( Sys::getProcessPath() + ".." + FileSystem::getOSSlash(), 1 ) );
	auto* scene = app.getUI();
	auto* parent = scene->loadLayoutFromString( R"xml(
		<widget id="parent" layout_width="100dp" layout_height="50dp">
			<style>
				@keyframes grow {
					from { width: 0%; }
					to { width: 100%; }
				}
				#child { animation: grow 10s step-start infinite; }
			</style>
			<widget id="child" layout_height="10dp" />
		</widget>
	)xml" );
	ASSERT_TRUE( parent != nullptr );
	auto* child = parent->find<UIWidget>( "child" );
	ASSERT_TRUE( child != nullptr );

	for ( int update = 0; update < 5; ++update )
		scene->update( Milliseconds( 10 ) );
	EXPECT_NEAR( child->getPixelsSize().getWidth(), parent->getPixelsSize().getWidth(), 0.5f );

	parent->setPixelsSize(
		Sizef( parent->getPixelsSize().getWidth() * 2, parent->getPixelsSize().getHeight() ) );
	for ( int update = 0; update < 5; ++update )
		scene->update( Milliseconds( 10 ) );
	EXPECT_NEAR( child->getPixelsSize().getWidth(), parent->getPixelsSize().getWidth(), 0.5f );
}
