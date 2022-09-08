#include <eepp/system/functionstring.hpp>
#include <eepp/ui/css/timingfunction.hpp>

using namespace EE::System;

namespace EE { namespace UI { namespace CSS {

TimingFunction TimingFunction::parse( std::string timingFunction ) {
	String::trimInPlace( timingFunction );
	String::toLowerInPlace( timingFunction );
	if ( timingFunction.find_first_of( '(' ) == std::string::npos ) {
		return { Ease::fromName( timingFunction ), {} };
	} else {
		TimingFunction tf;
		FunctionString func( FunctionString::parse( timingFunction ) );
		if ( !func.isEmpty() ) {
			tf.interpolation = Ease::fromName( timingFunction, Ease::Interpolation::None );
			const auto& params = func.getParameters();
			for ( const auto& param : params ) {
				double d;
				if ( String::fromString( d, param ) )
					tf.parameters.emplace_back( d );
			}
		}
		return tf;
	}
}

}}} // namespace EE::UI::CSS
