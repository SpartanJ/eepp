#ifndef EE_UI_CSS_TIMINGFUNCTION_HPP
#define EE_UI_CSS_TIMINGFUNCTION_HPP

#include <eepp/core/string.hpp>
#include <eepp/math/ease.hpp>

using namespace EE;
using namespace EE::Math;

namespace EE { namespace UI { namespace CSS {

class EE_API TimingFunction {
  public:
	static TimingFunction parse( std::string timingFunction );

	Ease::Interpolation interpolation{ Ease::None };
	std::vector<double> parameters;
};

}}} // namespace EE::UI::CSS

#endif // EE_UI_CSS_TIMINGFUNCTION_HPP
