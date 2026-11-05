#include <eepp/core/string.hpp>
#include <eepp/math/easing.hpp>
#include <eepp/ui/css/timingfunction.hpp>

#include <algorithm>

using namespace EE::Math::easing;

namespace EE { namespace UI { namespace CSS {

static constexpr std::string_view Whitespace = " \t\n\r\f\v";

static void setSteps( TimingFunction& function, int steps, StepPosition position ) {
	function.interpolation = Ease::Steps;
	function.parameters.clear();
	function.parameters.emplace_back( steps );
	function.parameters.emplace_back( static_cast<double>( position ) );
}

/** steps( <count>, <position>? ). Anything invalid leaves @p function as Ease::None. */
static void parseSteps( std::string_view parameters, TimingFunction& function ) {
	const auto comma = parameters.find( ',' );
	const std::string_view count = String::trim( parameters.substr( 0, comma ), Whitespace );
	const std::string_view name = comma == std::string_view::npos
									  ? std::string_view{}
									  : String::trim( parameters.substr( comma + 1 ), Whitespace );

	auto position = StepPosition::JumpEnd;
	if ( name == "start" || name == "jump-start" )
		position = StepPosition::JumpStart;
	else if ( name == "jump-none" )
		position = StepPosition::JumpNone;
	else if ( name == "jump-both" )
		position = StepPosition::JumpBoth;
	else if ( name != "end" && name != "jump-end" && comma != std::string_view::npos )
		return;

	// fromString() alone would read 8 out of 8.5.
	int steps = 0;
	if ( count.empty() ||
		 !std::all_of( count.begin(), count.end(),
					   []( char c ) { return c >= '0' && c <= '9'; } ) ||
		 !String::fromString( steps, count ) )
		return;
	if ( steps >= ( position == StepPosition::JumpNone ? 2 : 1 ) )
		setSteps( function, steps, position );
}

TimingFunction TimingFunction::parse( std::string timingFunction ) {
	return parse( std::string_view{ timingFunction } );
}

TimingFunction TimingFunction::parse( std::string_view timingFunction ) {
	timingFunction = String::trim( timingFunction, Whitespace );
	SmallVector<char, 64> normalized;
	for ( char character : timingFunction )
		normalized.emplace_back( character >= 'A' && character <= 'Z' ? character + 32
																	  : character );
	timingFunction = std::string_view{ normalized.data(), normalized.size() };

	TimingFunction result;
	const auto open = timingFunction.find( '(' );
	if ( open == std::string_view::npos ) {
		if ( timingFunction == "step-start" )
			setSteps( result, 1, StepPosition::JumpStart );
		else if ( timingFunction == "step-end" )
			setSteps( result, 1, StepPosition::JumpEnd );
		else if ( timingFunction != "steps" )
			result.interpolation = Ease::fromName( timingFunction, Ease::Interpolation::None );
		return result;
	}
	const auto close = timingFunction.rfind( ')' );
	if ( close == std::string_view::npos || close <= open )
		return result;
	std::string_view parameters = timingFunction.substr( open + 1, close - open - 1 );
	if ( timingFunction.substr( 0, open ) == "steps" ) {
		parseSteps( parameters, result );
		return result;
	}
	result.interpolation =
		Ease::fromName( timingFunction.substr( 0, open ), Ease::Interpolation::None );

	while ( !parameters.empty() && result.parameters.size() < 4 ) {
		const auto comma = parameters.find( ',' );
		std::string_view parameter = String::trim( parameters.substr( 0, comma ), Whitespace );

		double value = 0;
		if ( String::fromString( value, parameter ) )
			result.parameters.emplace_back( value );
		if ( comma == std::string_view::npos )
			break;
		parameters.remove_prefix( comma + 1 );
	}
	return result;
}

}}} // namespace EE::UI::CSS
