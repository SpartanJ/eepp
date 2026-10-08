#ifndef EE_UI_CSS_TIMINGFUNCTION_HPP
#define EE_UI_CSS_TIMINGFUNCTION_HPP

#include <eepp/core/small_vector.hpp>
#include <eepp/core/string.hpp>
#include <eepp/math/ease.hpp>

using namespace EE::Math;

namespace EE { namespace UI { namespace CSS {

class EE_API TimingFunction {
  public:
	using Parameters = SmallVector<double, 4>;

	static TimingFunction parse( std::string timingFunction );

	static TimingFunction parse( std::string_view timingFunction );

	static TimingFunction parse( const char* timingFunction ) {
		return parse( std::string_view{ timingFunction } );
	}

	Ease::Interpolation interpolation{ Ease::None };
	Parameters parameters;
};

}}} // namespace EE::UI::CSS

#endif // EE_UI_CSS_TIMINGFUNCTION_HPP
