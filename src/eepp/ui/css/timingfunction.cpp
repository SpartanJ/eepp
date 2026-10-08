#include <eepp/core/string.hpp>
#include <eepp/ui/css/timingfunction.hpp>

namespace EE { namespace UI { namespace CSS {

TimingFunction TimingFunction::parse( std::string timingFunction ) {
	return parse( std::string_view{ timingFunction } );
}

TimingFunction TimingFunction::parse( std::string_view timingFunction ) {
	timingFunction = String::trim( timingFunction, " \t\n\r\f\v" );
	SmallVector<char, 64> normalized;
	for ( char character : timingFunction )
		normalized.emplace_back( character >= 'A' && character <= 'Z' ? character + 32
																	  : character );
	timingFunction = std::string_view{ normalized.data(), normalized.size() };

	TimingFunction result;
	const auto open = timingFunction.find( '(' );
	if ( open == std::string_view::npos ) {
		result.interpolation = Ease::fromName( timingFunction, Ease::Interpolation::None );
		return result;
	}
	const auto close = timingFunction.rfind( ')' );
	if ( close == std::string_view::npos || close <= open )
		return result;
	result.interpolation =
		Ease::fromName( timingFunction.substr( 0, open ), Ease::Interpolation::None );

	std::string_view parameters = timingFunction.substr( open + 1, close - open - 1 );
	while ( !parameters.empty() && result.parameters.size() < 4 ) {
		const auto comma = parameters.find( ',' );
		std::string_view parameter = String::trim( parameters.substr( 0, comma ), " \t\n\r\f\v" );

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
